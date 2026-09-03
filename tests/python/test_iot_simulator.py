#!/usr/bin/env python3
"""unittest coverage for tools/iot_simulator.py; no pytest dependency."""

import hashlib
import hmac
import struct
import sys
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

import iot_simulator as sim  # noqa: E402


class WireFormatTests(unittest.TestCase):
    def test_crc16_ccitt_known_vector(self):
        self.assertEqual(sim.crc16_ccitt(b"123456789"), 0x29B1)

    def test_wire_offsets_and_round_trip(self):
        packet = sim.Packet(
            msg_type=sim.MsgType.EVENT,
            src=0x22,
            dst=0x01,
            boot_id=0x1234,
            seq=0x01020304,
            flags=0x03,
            payload=sim.event_payload(0x06),
        )
        raw = sim.encode(packet)
        self.assertEqual(raw[:2], b"\xA5\x5A")
        self.assertEqual(raw[2], 0x41)
        self.assertEqual(raw[3:6], bytes((0x01, 0x22, 0x01)))
        self.assertEqual(raw[6:8], b"\x12\x34")
        self.assertEqual(raw[8:12], b"\x01\x02\x03\x04")
        self.assertEqual(raw[12], 0x03)
        self.assertEqual(raw[13], len(packet.payload))
        self.assertEqual(sim.decode(raw), packet)

    def test_crc_invalid_is_rejected(self):
        raw = bytearray(sim.encode(sim.Packet(payload=sim.event_payload(1))))
        raw[-2] ^= 0x80
        with self.assertRaises(sim.ProtocolError) as context:
            sim.decode(raw)
        self.assertEqual(context.exception.code, "CRC_INVALID")

    def test_tlv_truncated_is_rejected(self):
        raw = sim.encode(sim.Packet(payload=b"\x01\x03\xAA"))
        with self.assertRaises(sim.ProtocolError) as context:
            sim.decode(raw)
        self.assertEqual(context.exception.code, "TLV_TRUNCATED")

    def test_payload_limit_matches_v4(self):
        packet = sim.Packet(payload=b"\x00" * 65)
        with self.assertRaises(ValueError):
            sim.encode(packet)


class AuthenticationTests(unittest.TestCase):
    def setUp(self):
        self.key = b"unit-test-only-key"
        self.packet = sim.Packet(seq=4, payload=sim.event_payload(0x01))

    def test_hmac_known_vector(self):
        key = b"known-vector-key"
        packet = sim.Packet(
            msg_type=sim.MsgType.EVENT,
            src=0x02,
            dst=0x01,
            boot_id=0x1234,
            seq=0x01020304,
            flags=0x03,
            payload=sim.event_payload(0x06),
        )
        header = struct.pack(
            ">BBBBHIB",
            packet.version,
            int(packet.msg_type),
            packet.src,
            packet.dst,
            packet.boot_id,
            packet.seq,
            packet.flags & ~sim.AUTH_FLAG,
        )
        expected = bytes.fromhex("7b5f5cee")
        self.assertEqual(hmac.new(key, header + packet.payload, hashlib.sha256).digest()[:4], expected)
        signed = sim.sign_packet(packet, key)
        auth_values = [value for tag, value in sim.iter_tlvs(signed.payload) if tag == sim.AUTH_TLV]
        self.assertEqual(auth_values, [expected])


    def test_hmac_valid(self):
        signed = sim.sign_packet(self.packet, self.key)
        result = sim.check_auth(sim.decode(sim.encode(signed)), self.key, sim.AuthPolicy.REQUIRED)
        self.assertEqual(result, sim.AuthResult(True, "hmac_valid"))
        self.assertEqual(list(sim.iter_tlvs(signed.payload))[-1][0], sim.AUTH_TLV)

    def test_hmac_incorrect_and_absent_required(self):
        signed = sim.sign_packet(self.packet, self.key)
        corrupt_payload = signed.payload[:-1] + bytes((signed.payload[-1] ^ 1,))
        corrupt = sim.Packet(msg_type=signed.msg_type, src=signed.src, dst=signed.dst,
                             boot_id=signed.boot_id, seq=signed.seq, flags=signed.flags,
                             payload=corrupt_payload, version=signed.version)
        incorrect = sim.check_auth(sim.decode(sim.encode(corrupt)), self.key, sim.AuthPolicy.REQUIRED)
        absent = sim.check_auth(self.packet, self.key, sim.AuthPolicy.REQUIRED)
        self.assertEqual(incorrect.reason, "HMAC_INVALID")
        self.assertEqual(absent.reason, "HMAC_ABSENT")

    def test_optional_and_disabled_policy(self):
        self.assertTrue(sim.check_auth(self.packet, self.key, sim.AuthPolicy.OPTIONAL).accepted)
        self.assertTrue(sim.check_auth(self.packet, self.key, sim.AuthPolicy.DISABLED).accepted)

    def test_hmac_is_not_accepted_with_flag_mismatch(self):
        signed = sim.sign_packet(self.packet, self.key)
        unmarked = sim.Packet(msg_type=signed.msg_type, src=signed.src, dst=signed.dst,
                              boot_id=signed.boot_id, seq=signed.seq, flags=0,
                              payload=signed.payload, version=signed.version)
        result = sim.check_auth(unmarked, self.key, sim.AuthPolicy.OPTIONAL)
        self.assertEqual(result.reason, "HMAC_FLAG_MISMATCH")


class IngressAndRetryTests(unittest.TestCase):
    def test_duplicate_is_acked_but_not_delivered_twice(self):
        receiver = sim.Receiver()
        packet = sim.Packet(seq=8, flags=0x03, payload=sim.event_payload(1))
        first = receiver.receive(sim.encode(packet))
        duplicate = receiver.receive(sim.encode(packet))
        self.assertTrue(first.accepted)
        self.assertTrue(first.ack_sent)
        self.assertEqual(duplicate.reason, "duplicate")
        self.assertTrue(duplicate.ack_sent)
        self.assertEqual(len(receiver.delivered), 1)

    def test_out_of_order_inside_window_is_accepted(self):
        receiver = sim.Receiver()
        receiver.receive(sim.encode(sim.Packet(seq=10, flags=sim.RELIABLE_FLAG)))
        result = receiver.receive(sim.encode(sim.Packet(seq=8, flags=sim.RELIABLE_FLAG)))
        self.assertEqual(result.reason, "accepted_out_of_order")
        self.assertTrue(result.accepted)

    def test_replay_outside_window_is_rejected(self):
        receiver = sim.Receiver()
        receiver.receive(sim.encode(sim.Packet(seq=10, flags=sim.RELIABLE_FLAG)))
        result = receiver.receive(sim.encode(sim.Packet(seq=1, flags=sim.RELIABLE_FLAG)))
        self.assertEqual(result.reason, "replay_outside_window")
        self.assertFalse(result.accepted)

    def test_new_boot_id_resets_window(self):
        receiver = sim.Receiver()
        receiver.receive(sim.encode(sim.Packet(boot_id=50, seq=20, flags=sim.RELIABLE_FLAG)))
        result = receiver.receive(sim.encode(sim.Packet(boot_id=51, seq=1, flags=sim.RELIABLE_FLAG)))
        self.assertEqual(result.reason, "accepted_new_boot")
        self.assertTrue(result.accepted)

    def test_loss_then_reliable_retry(self):
        receiver = sim.Receiver()
        result = sim.transmit_with_retries(
            sim.Packet(seq=9, flags=0x03, payload=sim.event_payload(0x06)),
            receiver,
            drop_attempts=(1,),
        )
        self.assertTrue(result["ack"])
        self.assertEqual(result["attempts"], 2)
        self.assertEqual(len(receiver.delivered), 1)

    def test_two_sensors_are_independent(self):
        receiver = sim.Receiver()
        receiver.receive(sim.encode(sim.Packet(src=2, boot_id=10, payload=sim.event_payload(1))))
        receiver.receive(sim.encode(sim.Packet(src=3, boot_id=20, payload=sim.event_payload(6))))
        self.assertEqual({packet.src for packet in receiver.delivered}, {2, 3})


class ScenarioTests(unittest.TestCase):
    def test_all_documented_scenarios_pass(self):
        results = sim.run_scenarios()
        self.assertEqual(len(results), len(sim.SCENARIOS))
        failures = [(name, detail) for name, passed, detail in results if not passed]
        self.assertEqual(failures, [])


if __name__ == "__main__":
    unittest.main()
