#!/usr/bin/env python3
"""Reproducible Python-only simulator for the IoT wire protocol V4.1.

This models the wire codec and a small receiver/retry path.  It does not import,
compile, or pretend to execute the ESP/Arduino IoTNode C++ implementation.
Only Python's standard library is used.

Run from the repository root:
    python3 tools/iot_simulator.py --scenario all

The HMAC key used by the scenarios is deliberately a non-secret test value.
"""

from __future__ import print_function

import argparse
import hashlib
import hmac
import struct
import sys
from dataclasses import dataclass
from enum import Enum, IntEnum

MAGIC = b"\xA5\x5A"
PROTOCOL_MAJOR = 4
PROTOCOL_MINOR = 1
PROTOCOL_VERSION = 0x41
HEADER_SIZE = 14
CRC_SIZE = 2
MAX_PAYLOAD = 64
MAX_PACKET = HEADER_SIZE + MAX_PAYLOAD + CRC_SIZE
ACK_REQUIRED_FLAG = 0x01
RELIABLE_FLAG = 0x02
AUTH_FLAG = 0x10
AUTH_TLV = 0xF0
AUTH_HMAC_SIZE = 4
REPLAY_WINDOW = 8
SIMULATOR_KEY = b"wifi-pir-python-test-key"


class MsgType(IntEnum):
    EVENT = 0x01
    DATA = 0x02
    ACK = 0x10
    HEARTBEAT = 0x11
    STATE_REPORT = 0x13
    STATE_REQUEST = 0x14
    HELLO = 0x20
    CONFIG = 0x30
    ERROR_MSG = 0xE0


class AuthPolicy(Enum):
    DISABLED = "disabled"
    OPTIONAL = "optional"
    REQUIRED = "required"


class ProtocolError(ValueError):
    """A decode error with a stable, human-readable reason."""

    def __init__(self, code, message):
        super(ProtocolError, self).__init__(message)
        self.code = code


@dataclass(frozen=True)
class Packet:
    """Decoded/encodable V4 packet; payload is the raw TLV byte sequence."""

    msg_type: int = MsgType.EVENT
    src: int = 2
    dst: int = 1
    boot_id: int = 1
    seq: int = 1
    flags: int = 0
    payload: bytes = b""
    version: int = PROTOCOL_VERSION


def crc16_ccitt(data):
    """CRC16-CCITT used by IoTProtocol.cpp (poly 0x1021, init 0xFFFF)."""

    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def validate_tlv(payload):
    """Validate that payload consists entirely of TAG/LENGTH/VALUE records."""

    offset = 0
    while offset < len(payload):
        if offset + 2 > len(payload):
            raise ProtocolError("TLV_TRUNCATED", "TLV header is truncated")
        value_length = payload[offset + 1]
        end = offset + 2 + value_length
        if end > len(payload):
            raise ProtocolError("TLV_TRUNCATED", "TLV value is truncated")
        offset = end


def iter_tlvs(payload):
    """Yield (tag, value) after strict TLV validation."""

    validate_tlv(payload)
    offset = 0
    while offset < len(payload):
        tag = payload[offset]
        length = payload[offset + 1]
        yield tag, payload[offset + 2:offset + 2 + length]
        offset += 2 + length


def tlv(tag, value):
    value = bytes(value)
    if not 0 <= tag <= 0xFF or len(value) > 0xFF:
        raise ValueError("TLV tag/value is outside the wire range")
    return bytes((tag, len(value))) + value


def event_payload(event_code, active=True):
    """Build the same two basic event TLVs used by the V4 event path."""

    return tlv(0x01, bytes((event_code,))) + tlv(0x02, bytes((1 if active else 0,)))


def encode(packet):
    """Serialize Packet to the exact V4.1 wire format."""

    if len(packet.payload) > MAX_PAYLOAD:
        raise ValueError("payload exceeds V4 maximum of 64 bytes")
    if not 0 <= packet.version <= 0xFF:
        raise ValueError("version must fit in one byte")
    for name, value, maximum in (
        ("msg_type", packet.msg_type, 0xFF),
        ("src", packet.src, 0xFF),
        ("dst", packet.dst, 0xFF),
        ("boot_id", packet.boot_id, 0xFFFF),
        ("seq", packet.seq, 0xFFFFFFFF),
        ("flags", packet.flags, 0xFF),
    ):
        if not 0 <= int(value) <= maximum:
            raise ValueError("{} is outside its wire range".format(name))

    header = struct.pack(
        ">2sBBBBHIBB",
        MAGIC,
        packet.version,
        int(packet.msg_type),
        packet.src,
        packet.dst,
        packet.boot_id,
        packet.seq,
        packet.flags,
        len(packet.payload),
    )
    body = header + packet.payload
    return body + struct.pack(">H", crc16_ccitt(body))


def decode(datagram):
    """Deserialize and strictly validate a V4.1 frame."""

    datagram = bytes(datagram)
    if len(datagram) < HEADER_SIZE + CRC_SIZE:
        raise ProtocolError("LENGTH_INVALID", "frame is shorter than header plus CRC")
    if datagram[:2] != MAGIC:
        raise ProtocolError("MAGIC_INVALID", "MAGIC does not match A5 5A")
    version = datagram[2]
    if version >> 4 != PROTOCOL_MAJOR:
        raise ProtocolError("VERSION_INVALID", "protocol major is not 4")

    payload_length = datagram[13]
    if payload_length > MAX_PAYLOAD:
        raise ProtocolError("PAYLOAD_TOO_LARGE", "payload exceeds 64 bytes")
    expected_length = HEADER_SIZE + payload_length + CRC_SIZE
    if len(datagram) != expected_length:
        raise ProtocolError("LENGTH_INVALID", "frame length is not header + payload + CRC")

    received_crc = struct.unpack(">H", datagram[-CRC_SIZE:])[0]
    calculated_crc = crc16_ccitt(datagram[:-CRC_SIZE])
    if received_crc != calculated_crc:
        raise ProtocolError("CRC_INVALID", "CRC16-CCITT does not match")

    payload = datagram[HEADER_SIZE:HEADER_SIZE + payload_length]
    validate_tlv(payload)
    fields = struct.unpack(">2sBBBBHIBB", datagram[:HEADER_SIZE])
    return Packet(
        msg_type=fields[2],
        src=fields[3],
        dst=fields[4],
        boot_id=fields[5],
        seq=fields[6],
        flags=fields[7],
        payload=payload,
        version=fields[1],
    )


def _auth_input(packet, payload_without_auth):
    """Build IoTAuth.cpp's HMAC input (11 header fields + payload)."""

    header_fields = struct.pack(
        ">BBBBHIB",
        packet.version,
        int(packet.msg_type),
        packet.src,
        packet.dst,
        packet.boot_id,
        packet.seq,
        packet.flags & ~AUTH_FLAG,
    )
    return header_fields + payload_without_auth


def _auth_parts(packet):
    parts = list(iter_tlvs(packet.payload))
    auth_indexes = [index for index, (tag, _) in enumerate(parts) if tag == AUTH_TLV]
    if len(auth_indexes) > 1:
        raise ProtocolError("HMAC_INVALID", "more than one HMAC TLV is present")
    if auth_indexes and auth_indexes[0] != len(parts) - 1:
        raise ProtocolError("HMAC_INVALID", "HMAC TLV must be the final TLV")
    if auth_indexes and len(parts[auth_indexes[0]][1]) != AUTH_HMAC_SIZE:
        raise ProtocolError("HMAC_INVALID", "HMAC TLV must contain four bytes")
    if auth_indexes:
        auth_index = auth_indexes[0]
        without_auth = b"".join(tlv(tag, value) for tag, value in parts[:auth_index])
        return without_auth, parts[auth_index][1]
    return packet.payload, None


def sign_packet(packet, key=SIMULATOR_KEY):
    """Return a signed copy, matching IoTAuth::signPacket idempotence."""

    without_auth, existing = _auth_parts(packet)
    digest = hmac.new(key, _auth_input(packet, without_auth), hashlib.sha256).digest()[:AUTH_HMAC_SIZE]
    signed_payload = without_auth + tlv(AUTH_TLV, digest)
    if len(signed_payload) > MAX_PAYLOAD:
        raise ValueError("payload plus HMAC TLV exceeds 64 bytes")
    return Packet(
        msg_type=packet.msg_type,
        src=packet.src,
        dst=packet.dst,
        boot_id=packet.boot_id,
        seq=packet.seq,
        flags=packet.flags | AUTH_FLAG,
        payload=signed_payload,
        version=packet.version,
    )


@dataclass(frozen=True)
class AuthResult:
    accepted: bool
    reason: str


def check_auth(packet, key=SIMULATOR_KEY, policy=AuthPolicy.REQUIRED):
    """Apply the V4 DISABLED/OPTIONAL/REQUIRED ingress policy."""

    if policy is AuthPolicy.DISABLED:
        return AuthResult(True, "auth_disabled")

    try:
        without_auth, received = _auth_parts(packet)
    except ProtocolError as error:
        return AuthResult(False, error.code)

    marked = bool(packet.flags & AUTH_FLAG)
    if not marked:
        if received is not None:
            return AuthResult(False, "HMAC_FLAG_MISMATCH")
        if policy is AuthPolicy.REQUIRED:
            return AuthResult(False, "HMAC_ABSENT")
        return AuthResult(True, "auth_optional_absent")
    if received is None:
        return AuthResult(False, "HMAC_ABSENT")

    expected = hmac.new(key, _auth_input(packet, without_auth), hashlib.sha256).digest()[:AUTH_HMAC_SIZE]
    if not hmac.compare_digest(received, expected):
        return AuthResult(False, "HMAC_INVALID")
    return AuthResult(True, "hmac_valid")


def _serial_newer(candidate, current, bits):
    modulus = 1 << bits
    half = modulus >> 1
    distance = (candidate - current) % modulus
    return 0 < distance < half


class ReplayWindow(object):
    """BOOT_ID+SEQ sliding window matching IoTNode's eight-packet window."""

    def __init__(self, window=REPLAY_WINDOW):
        self.window = window
        self.sessions = {}

    def observe(self, packet):
        if packet.boot_id == 0:
            return "BOOT_ID_INVALID"
        key = packet.src
        current = self.sessions.get(key)
        if current is None:
            self.sessions[key] = [packet.boot_id, packet.seq, 1]
            return "accepted_new_boot"

        boot_id, highest, bitmap = current
        if packet.boot_id != boot_id:
            if _serial_newer(packet.boot_id, boot_id, 16):
                self.sessions[key] = [packet.boot_id, packet.seq, 1]
                return "accepted_new_boot"
            return "replay_old_boot"

        if packet.seq > highest:
            shift = packet.seq - highest
            bitmap = 1 if shift >= self.window else ((bitmap << shift) | 1) & ((1 << self.window) - 1)
            self.sessions[key] = [boot_id, packet.seq, bitmap]
            return "accepted_in_order"

        distance = highest - packet.seq
        if distance >= self.window:
            return "replay_outside_window"
        bit = 1 << distance
        if bitmap & bit:
            return "duplicate"
        self.sessions[key][2] = bitmap | bit
        return "accepted_out_of_order"


@dataclass(frozen=True)
class ReceiveResult:
    accepted: bool
    reason: str
    ack_sent: bool = False


class Receiver(object):
    """Minimal observable ingress model; it never invokes the C++ implementation."""

    def __init__(self, auth_policy=AuthPolicy.DISABLED, key=SIMULATOR_KEY):
        self.auth_policy = auth_policy
        self.key = key
        self.replay = ReplayWindow()
        self.delivered = []

    def receive(self, datagram):
        try:
            packet = decode(datagram)
        except ProtocolError as error:
            return ReceiveResult(False, error.code)

        auth = check_auth(packet, self.key, self.auth_policy)
        if not auth.accepted:
            return ReceiveResult(False, auth.reason)

        if packet.flags & RELIABLE_FLAG:
            replay_reason = self.replay.observe(packet)
        else:
            replay_reason = "accepted_unreliable"
        ack_sent = bool(packet.flags & ACK_REQUIRED_FLAG) and replay_reason in (
            "accepted_new_boot", "accepted_in_order", "accepted_out_of_order", "accepted_unreliable", "duplicate"
        )
        if replay_reason in ("accepted_new_boot", "accepted_in_order", "accepted_out_of_order", "accepted_unreliable"):
            self.delivered.append(packet)
            return ReceiveResult(True, replay_reason, ack_sent)
        return ReceiveResult(False, replay_reason, ack_sent)


def transmit_with_retries(packet, receiver, drop_attempts=(), max_attempts=5):
    """Transmit deterministically, dropping selected 1-based attempts."""

    drop_attempts = set(drop_attempts)
    attempts = 0
    for attempt in range(1, max_attempts + 1):
        attempts = attempt
        if attempt in drop_attempts:
            continue
        result = receiver.receive(encode(packet))
        if result.ack_sent:
            return {"delivered": bool(receiver.delivered), "attempts": attempts, "ack": True, "last": result}
    return {"delivered": False, "attempts": attempts, "ack": False, "last": None}


def _require(condition, message):
    if not condition:
        raise AssertionError(message)


def scenario_valid_packet():
    packet = Packet(msg_type=MsgType.EVENT, src=2, dst=1, boot_id=0x1234, seq=7, payload=event_payload(0x06))
    decoded = decode(encode(packet))
    _require(decoded == packet, "valid packet did not round-trip")
    return "round-trip EVENT with two TLVs and CRC16"


def scenario_crc_invalid():
    raw = bytearray(encode(Packet(payload=event_payload(0x01))))
    raw[-1] ^= 0x01
    try:
        decode(raw)
    except ProtocolError as error:
        _require(error.code == "CRC_INVALID", "wrong error for corrupt CRC: {}".format(error.code))
    else:
        raise AssertionError("corrupt CRC was accepted")
    return "CRC corruption rejected"


def scenario_tlv_truncated():
    raw = encode(Packet(payload=bytes((0x01, 0x02, 0x01))))
    try:
        decode(raw)
    except ProtocolError as error:
        _require(error.code == "TLV_TRUNCATED", "wrong error for truncated TLV: {}".format(error.code))
    else:
        raise AssertionError("truncated TLV was accepted")
    return "structurally truncated TLV rejected after CRC validation"


def scenario_hmac_policies():
    base = Packet(seq=10, payload=event_payload(0x06))
    signed = sign_packet(base)
    _require(check_auth(decode(encode(signed)), policy=AuthPolicy.REQUIRED).accepted, "valid HMAC rejected")

    bad_payload = signed.payload[:-1] + bytes((signed.payload[-1] ^ 0xFF,))
    bad = Packet(msg_type=signed.msg_type, src=signed.src, dst=signed.dst, boot_id=signed.boot_id,
                 seq=signed.seq, flags=signed.flags, payload=bad_payload, version=signed.version)
    _require(check_auth(decode(encode(bad)), policy=AuthPolicy.REQUIRED).reason == "HMAC_INVALID",
             "incorrect HMAC accepted")
    _require(check_auth(decode(encode(base)), policy=AuthPolicy.REQUIRED).reason == "HMAC_ABSENT",
             "absent HMAC accepted under required policy")
    return "HMAC valid, incorrect and absent/required cases"


def scenario_duplicate():
    receiver = Receiver()
    packet = Packet(seq=20, flags=0x03, payload=event_payload(0x01))
    first = receiver.receive(encode(packet))
    second = receiver.receive(encode(packet))
    _require(first.accepted and first.ack_sent, "first reliable packet was not accepted/ACKed")
    _require(second.reason == "duplicate" and second.ack_sent, "duplicate was not ACKed without a second effect")
    _require(len(receiver.delivered) == 1, "duplicate caused a second delivery")
    return "duplicate ACKed, delivered once"


def scenario_out_of_order():
    receiver = Receiver()
    first = receiver.receive(encode(Packet(seq=10, flags=RELIABLE_FLAG)))
    late = receiver.receive(encode(Packet(seq=8, flags=RELIABLE_FLAG)))
    _require(first.accepted and late.accepted and late.reason == "accepted_out_of_order",
             "SEQ within the eight-packet window was not accepted")
    return "out-of-order SEQ accepted inside window"


def scenario_replay_outside_window():
    receiver = Receiver()
    receiver.receive(encode(Packet(seq=10, flags=RELIABLE_FLAG)))
    replay = receiver.receive(encode(Packet(seq=1, flags=RELIABLE_FLAG)))
    _require(replay.reason == "replay_outside_window", "old SEQ was not rejected outside window")
    return "replay outside window rejected"


def scenario_new_boot_id():
    receiver = Receiver()
    old_boot = receiver.receive(encode(Packet(boot_id=100, seq=99, flags=RELIABLE_FLAG)))
    new_boot = receiver.receive(encode(Packet(boot_id=101, seq=1, flags=RELIABLE_FLAG)))
    _require(old_boot.accepted and new_boot.accepted and new_boot.reason == "accepted_new_boot",
             "new BOOT_ID did not reset the sequence window")
    return "new BOOT_ID accepted with SEQ reset"


def scenario_loss_retry():
    receiver = Receiver()
    result = transmit_with_retries(Packet(seq=30, flags=0x03, payload=event_payload(0x06)), receiver, drop_attempts=(1,))
    _require(result["ack"] and result["attempts"] == 2 and len(receiver.delivered) == 1,
             "loss/retry did not deliver exactly once on attempt two")
    return "first attempt lost, retry ACKed on attempt two"


def scenario_two_sensors():
    receiver = Receiver()
    sensor_a = receiver.receive(encode(Packet(src=2, boot_id=11, seq=1, payload=event_payload(0x01))))
    sensor_b = receiver.receive(encode(Packet(src=3, boot_id=22, seq=1, payload=event_payload(0x06))))
    sources = {packet.src for packet in receiver.delivered}
    _require(sensor_a.accepted and sensor_b.accepted and sources == {2, 3},
             "simultaneous sensor events were not independently delivered")
    return "two sensor sources delivered independently"


SCENARIOS = (
    ("valid-packet", scenario_valid_packet),
    ("crc-invalid", scenario_crc_invalid),
    ("tlv-truncated", scenario_tlv_truncated),
    ("hmac-policies", scenario_hmac_policies),
    ("duplicate", scenario_duplicate),
    ("seq-out-of-order", scenario_out_of_order),
    ("replay-outside-window", scenario_replay_outside_window),
    ("boot-id-new", scenario_new_boot_id),
    ("loss-retry", scenario_loss_retry),
    ("two-sensors", scenario_two_sensors),
)


def run_scenarios(selected="all"):
    selected_names = {name for name, _ in SCENARIOS} if selected == "all" else {selected}
    results = []
    for name, function in SCENARIOS:
        if name not in selected_names:
            continue
        try:
            detail = function()
        except Exception as error:
            results.append((name, False, str(error)))
        else:
            results.append((name, True, detail))
    if not results:
        raise ValueError("unknown scenario: {}".format(selected))
    return results


def main(argv=None):
    parser = argparse.ArgumentParser(description="Reproducible Python-only IoT V4.1 simulator")
    parser.add_argument("--scenario", default="all", help="scenario name or all (default: all)")
    parser.add_argument("--list", action="store_true", help="list available scenarios")
    args = parser.parse_args(argv)
    if args.list:
        for name, _ in SCENARIOS:
            print(name)
        return 0

    print("IoT V4.1 simulator (stdlib only; no C++/hardware execution)")
    print("HMAC key: fixed simulator-only value, not a project secret")
    results = run_scenarios(args.scenario)
    for name, passed, detail in results:
        print("[{}] {:24s} {}".format("PASS" if passed else "FAIL", name, detail))
    failed = sum(1 for _, passed, _ in results if not passed)
    print("Summary: {}/{} scenarios passed".format(len(results) - failed, len(results)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
