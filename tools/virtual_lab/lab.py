#!/usr/bin/env python3
"""Deterministic functional laboratory for the wifi_PIR application.

This is intentionally a host-side model, not an electrical simulator and not a
C++ firmware executor. It reuses the repository wire codec/HMAC implementation
from ``tools.iot_simulator`` and adds observable models for the PIR+timbre
emitter, UDP faults, central alarm logic, buzzer, MQTT and Home Assistant.

Run from the repository root with::

    python3 -m tools.virtual_lab --scenario all

All time is virtual. No sleep, sockets, broker or external dependency is used.
"""

from __future__ import annotations

import heapq
import json
import random
from collections import deque
from dataclasses import dataclass, field
from typing import Any, Callable, Deque, Dict, Iterable, List, Optional, Tuple

from tools.iot_simulator import (
    ACK_REQUIRED_FLAG,
    AUTH_FLAG,
    AuthPolicy,
    Packet,
    ProtocolError,
    RELIABLE_FLAG,
    ReplayWindow,
    SIMULATOR_KEY,
    decode,
    encode,
    event_payload,
    check_auth,
    sign_packet,
    iter_tlvs,
    tlv,
)

# Application profile values mirror lib/AlarmProfile/AlarmProfile.h.
EVENT_MOTION = 0x01
EVENT_TIMBRE = 0x06
DEVICE_CENTRAL = 0x01
DEVICE_PIR = 0x02
STATE_MOTION = 0xA0
STATE_BUTTON = 0xA3
EVENT_TYPE_TAG = 0x01
EVENT_VALUE_TAG = 0x02
DEVICE_TYPE_TAG = 0x61
DEVICE_NAME_TAG = 0x60
UPTIME_TAG = 0x80

TOPIC_ESTADO = "casa/iot/central/estado"
TOPIC_UPTIME = "casa/iot/central/uptime"
TOPIC_IP = "casa/iot/central/ip"
TOPIC_MODO = "casa/iot/alarma/modo/set"
TOPIC_MODO_STATE = "casa/iot/alarma/modo/state"
TOPIC_BOCINA_CMD = "casa/iot/alarma/bocina/set"
TOPIC_BOCINA_STATE = "casa/iot/alarma/bocina/state"
TOPIC_V3_EVENTO = "casa/alarma/evento"
TOPIC_V3_TIMBRE = "casa/alarma/timbre"
TOPIC_V3_ESTADO = "casa/alarma/estado"
TOPIC_V3_BOCINA_CMD = "casa/alarma/bocina/set"
TOPIC_V3_BOCINA_STATE = "casa/alarma/bocina/state"
TOPIC_V3_MODO = "casa/alarma/modo/set"
TOPIC_V3_MODO_STATE = "casa/alarma/modo/state"
TOPIC_V3_UPTIME = "casa/alarma/uptime"
TOPIC_V3_IP = "casa/alarma/ip"


def _first_tlv(payload: bytes, tag: int, default: Optional[bytes] = None) -> Optional[bytes]:
    for current_tag, value in iter_tlvs(payload):
        if current_tag == tag:
            return value
    return default


def _u8(payload: bytes, tag: int, default: int = 0) -> int:
    value = _first_tlv(payload, tag)
    return value[0] if value and len(value) == 1 else default


def _u32(payload: bytes, tag: int, default: int = 0) -> int:
    value = _first_tlv(payload, tag)
    return int.from_bytes(value, "big") if value and len(value) == 4 else default


def _string(payload: bytes, tag: int, default: str = "") -> str:
    value = _first_tlv(payload, tag)
    return value.decode("utf-8", errors="replace") if value else default


class VirtualClock:
    """Monotonic deterministic scheduler."""

    def __init__(self):
        self.now_ms = 0
        self._sequence = 0
        self._queue: List[Tuple[int, int, Callable[[], None]]] = []

    def call_later(self, delay_ms: int, callback: Callable[[], None]) -> None:
        self._sequence += 1
        heapq.heappush(self._queue, (self.now_ms + max(0, delay_ms), self._sequence, callback))

    def run_until(self, target_ms: int) -> None:
        while self._queue and self._queue[0][0] <= target_ms:
            timestamp, _, callback = heapq.heappop(self._queue)
            self.now_ms = timestamp
            callback()
        self.now_ms = max(self.now_ms, target_ms)


class Trace:
    def __init__(self, clock: VirtualClock):
        self.clock = clock
        self.records: List[Dict[str, Any]] = []

    def add(self, kind: str, **data: Any) -> None:
        record = {"t_ms": self.clock.now_ms, "kind": kind}
        record.update(data)
        self.records.append(record)

    def count(self, kind: str, **match: Any) -> int:
        return sum(
            1 for record in self.records
            if record["kind"] == kind and all(record.get(k) == v for k, v in match.items())
        )


class VirtualNetwork:
    """In-memory UDP-like datagram bus with deterministic fault injection."""

    def __init__(self, clock: VirtualClock, trace: Trace, seed: int = 7, delay_ms: int = 5):
        self.clock = clock
        self.trace = trace
        self.random = random.Random(seed)
        self.delay_ms = delay_ms
        self.endpoints: Dict[int, Callable[[bytes], None]] = {}
        self.attempts: Dict[Tuple[int, int, int, int], int] = {}
        self.drop_rules: Dict[Tuple[int, int, Optional[int]], int] = {}
        self.duplicate_rules: Dict[Tuple[int, int, Optional[int]], int] = {}
        self.corrupt_rules: Dict[Tuple[int, int, Optional[int]], int] = {}
        self.blocked: set[Tuple[int, int]] = set()
        self.captured: List[Dict[str, Any]] = []

    def register(self, device_id: int, receiver: Callable[[bytes], None]) -> None:
        self.endpoints[device_id] = receiver

    def drop_first(self, sender: int, target: int, msg_type: Optional[int] = None, count: int = 1) -> None:
        self.drop_rules[(sender, target, msg_type)] = count

    def duplicate_first(self, sender: int, target: int, msg_type: Optional[int] = None, count: int = 1) -> None:
        self.duplicate_rules[(sender, target, msg_type)] = count

    def corrupt_first(self, sender: int, target: int, msg_type: Optional[int] = None, count: int = 1) -> None:
        self.corrupt_rules[(sender, target, msg_type)] = count

    def set_blocked(self, sender: int, target: int, blocked: bool = True) -> None:
        if blocked:
            self.blocked.add((sender, target))
        else:
            self.blocked.discard((sender, target))

    @staticmethod
    def _consume(rules: Dict[Tuple[int, int, Optional[int]], int], sender: int, target: int, msg_type: int) -> bool:
        for key in ((sender, target, msg_type), (sender, target, None)):
            remaining = rules.get(key, 0)
            if remaining:
                rules[key] = remaining - 1
                return True
        return False

    def send(self, sender: int, target: int, raw: bytes) -> None:
        try:
            packet = decode(raw)
            msg_type = int(packet.msg_type)
            seq = packet.seq
        except ProtocolError:
            msg_type = -1
            seq = -1

        key = (sender, target, msg_type, seq)
        self.attempts[key] = self.attempts.get(key, 0) + 1
        attempt = self.attempts[key]
        self.captured.append({"t_ms": self.clock.now_ms, "sender": sender, "target": target,
                              "msg_type": msg_type, "seq": seq, "attempt": attempt,
                              "raw_hex": bytes(raw).hex()})
        self.trace.add("packet_tx", sender=sender, target=target, msg_type=msg_type,
                       seq=seq, attempt=attempt)

        if (sender, target) in self.blocked:
            self.trace.add("packet_drop", reason="link_blocked", sender=sender, target=target,
                           msg_type=msg_type, seq=seq, attempt=attempt)
            return
        if target not in self.endpoints:
            self.trace.add("packet_drop", reason="unknown_target", sender=sender, target=target,
                           msg_type=msg_type, seq=seq, attempt=attempt)
            return
        if self._consume(self.drop_rules, sender, target, msg_type):
            self.trace.add("packet_drop", reason="fault_plan", sender=sender, target=target,
                           msg_type=msg_type, seq=seq, attempt=attempt)
            return

        deliveries = 2 if self._consume(self.duplicate_rules, sender, target, msg_type) else 1
        payload = bytearray(raw)
        if self._consume(self.corrupt_rules, sender, target, msg_type) and payload:
            payload[-1] ^= 0x01
            self.trace.add("packet_corrupt", sender=sender, target=target, msg_type=msg_type, seq=seq)
        for copy_index in range(deliveries):
            self.clock.call_later(
                self.delay_ms + copy_index,
                lambda data=bytes(payload), destination=target: self._deliver(destination, data),
            )
        if deliveries == 2:
            self.trace.add("packet_duplicate", sender=sender, target=target, msg_type=msg_type, seq=seq)

    def _deliver(self, target: int, raw: bytes) -> None:
        self.trace.add("packet_rx", target=target)
        self.endpoints[target](raw)


class MemoryMqttBroker:
    """Small retained/LWT broker model with exact-topic and # subscriptions."""

    def __init__(self, trace: Trace):
        self.trace = trace
        self.available = True
        self.clients: Dict[str, Callable[[str, str], None]] = {}
        self.subscriptions: Dict[str, List[str]] = {}
        self.retained: Dict[str, str] = {}
        self.messages: List[Dict[str, Any]] = []
        self.lwt: Dict[str, Tuple[str, str, bool]] = {}

    @staticmethod
    def matches(filter_topic: str, topic: str) -> bool:
        if filter_topic == topic:
            return True
        if filter_topic.endswith("/#"):
            return topic.startswith(filter_topic[:-1])
        return False

    def connect(self, client: str, callback: Callable[[str, str], None],
                lwt: Optional[Tuple[str, str, bool]] = None) -> bool:
        if not self.available:
            self.trace.add("mqtt_connect_failed", client=client)
            return False
        self.clients[client] = callback
        self.subscriptions.setdefault(client, [])
        if lwt:
            self.lwt[client] = lwt
        self.trace.add("mqtt_connected", client=client)
        return True

    def disconnect(self, client: str, publish_lwt: bool = True) -> None:
        if publish_lwt and client in self.lwt and self.available:
            topic, payload, retained = self.lwt[client]
            self.publish(client, topic, payload, retained)
        self.clients.pop(client, None)
        self.subscriptions.pop(client, None)
        self.lwt.pop(client, None)
        self.trace.add("mqtt_disconnected", client=client)

    def set_available(self, available: bool) -> None:
        if self.available == available:
            return
        self.available = available
        self.trace.add("mqtt_availability", available=available)
        if not available:
            for client in list(self.clients):
                self.disconnect(client, publish_lwt=True)

    def subscribe(self, client: str, filter_topic: str) -> bool:
        if client not in self.clients:
            return False
        self.subscriptions.setdefault(client, []).append(filter_topic)
        for topic, payload in self.retained.items():
            if self.matches(filter_topic, topic):
                self.clients[client](topic, payload)
        self.trace.add("mqtt_subscribe", client=client, topic=filter_topic)
        return True

    def publish(self, client: str, topic: str, payload: str, retained: bool = False) -> bool:
        if not self.available or client not in self.clients:
            self.trace.add("mqtt_publish_failed", client=client, topic=topic)
            return False
        if retained:
            self.retained[topic] = payload
        message = {"t_ms": self.trace.clock.now_ms, "client": client, "topic": topic,
                   "payload": payload, "retained": retained}
        self.messages.append(message)
        self.trace.add("mqtt_publish", client=client, topic=topic, retained=retained)
        for subscriber, filters in list(self.subscriptions.items()):
            callback = self.clients.get(subscriber)
            if callback and any(self.matches(filter_topic, topic) for filter_topic in filters):
                callback(topic, payload)
        return True

    def inject(self, topic: str, payload: str) -> None:
        """Inject an external command as if Home Assistant published it."""
        if not self.available:
            return
        for subscriber, filters in list(self.subscriptions.items()):
            callback = self.clients.get(subscriber)
            if callback and any(self.matches(filter_topic, topic) for filter_topic in filters):
                callback(topic, payload)
        self.trace.add("mqtt_inject", topic=topic)


class VirtualHA:
    def __init__(self, broker: MemoryMqttBroker):
        self.broker = broker
        self.client = "homeassistant"
        self.started = False
        self.entities: Dict[str, Dict[str, Any]] = {}

    def start(self) -> None:
        self.started = True
        self._connect_if_available()
        self.broker.trace.clock.call_later(500, self._retry_connection)

    def _connect_if_available(self) -> None:
        if not self.broker.available or self.client in self.broker.clients:
            return
        if self.broker.connect(self.client, self._on_message):
            self.broker.subscribe(self.client, "homeassistant/#")
            self.broker.subscribe(self.client, "casa/#")

    def _retry_connection(self) -> None:
        if self.started:
            self._connect_if_available()
            self.broker.trace.clock.call_later(500, self._retry_connection)

    def _on_message(self, topic: str, payload: str) -> None:
        if topic.startswith("homeassistant/") and topic.endswith("/config"):
            try:
                config = json.loads(payload)
            except json.JSONDecodeError:
                return
            unique_id = config.get("unique_id")
            if unique_id:
                self.entities[unique_id] = {"config": config, "state": None, "availability": None}
            return
        for entity in self.entities.values():
            config = entity["config"]
            if topic == config.get("state_topic"):
                entity["state"] = payload
            if topic == config.get("availability_topic"):
                entity["availability"] = payload

    def state(self, unique_id: str) -> Optional[str]:
        return self.entities.get(unique_id, {}).get("state")

    def command(self, unique_id: str, payload: str) -> None:
        entity = self.entities.get(unique_id)
        if entity and entity["config"].get("command_topic"):
            self.broker.inject(entity["config"]["command_topic"], payload)


@dataclass
class LabConfig:
    auth_required: bool = False
    network_delay_ms: int = 5
    ack_timeout_ms: int = 120
    max_attempts: int = 5
    pir_debounce_ms: int = 200
    timbre_debounce_ms: int = 800
    heartbeat_ms: int = 3000
    stale_ms: int = 6000
    offline_ms: int = 12000
    mqtt_initial_ms: int = 1000
    mqtt_probe_ms: int = 3000
    mqtt_reconnect_ms: int = 1000
    buzzer_motion_ms: int = 1000
    buzzer_timbre_ms: int = 500


class VirtualBuzzer:
    def __init__(self, clock: VirtualClock, trace: Trace):
        self.clock = clock
        self.trace = trace
        self.on = False
        self.deadline_ms = 0
        self.transitions: List[Dict[str, Any]] = []

    def timed_on(self, duration_ms: int, reason: str) -> None:
        was_on = self.on
        self.on = True
        self.deadline_ms = max(self.deadline_ms, self.clock.now_ms + duration_ms)
        if not was_on:
            self.transitions.append({"kind": "on", "t_ms": self.clock.now_ms, "reason": reason})
            self.trace.add("buzzer_on", reason=reason, duration_ms=duration_ms)
        else:
            self.trace.add("buzzer_extend", reason=reason, duration_ms=duration_ms)
        deadline = self.deadline_ms
        self.clock.call_later(duration_ms, lambda: self._off_if_due(deadline))

    def _off_if_due(self, deadline: int) -> None:
        if self.on and self.deadline_ms <= deadline and self.clock.now_ms >= self.deadline_ms:
            self.on = False
            self.transitions.append({"kind": "off", "t_ms": self.clock.now_ms})
            self.trace.add("buzzer_off")

    def off(self, reason: str = "command") -> None:
        if self.on:
            self.on = False
            self.deadline_ms = self.clock.now_ms
            self.transitions.append({"kind": "off", "t_ms": self.clock.now_ms, "reason": reason})
            self.trace.add("buzzer_off", reason=reason)


class VirtualEmitter:
    def __init__(self, lab: "VirtualLab", device_id: int = 0x02, boot_id: int = 0x1001,
                 name: str = "PIR Entrada"):
        self.lab = lab
        self.device_id = device_id
        self.boot_id = boot_id
        self.name = name
        self.seq = 0
        self.pir_state = False
        self.timbre_state = False
        self.last_pir_event = -10**9
        self.last_timbre_event = -10**9
        self.pending: Deque[Packet] = deque()
        self.inflight: Optional[Dict[str, Any]] = None
        self.hello_sent = False
        self.sent_packets: List[Packet] = []
        self.received_acks = 0
        self.timeouts = 0

    @property
    def network(self) -> VirtualNetwork:
        return self.lab.network

    def start(self) -> None:
        self.network.register(self.device_id, self.receive)
        self._queue_reliable(self._hello_packet())
        self.lab.clock.call_later(self.lab.config.heartbeat_ms, self._heartbeat)

    def _next_seq(self) -> int:
        self.seq += 1
        return self.seq

    def _prepare(self, packet: Packet) -> Packet:
        if self.lab.config.auth_required:
            return sign_packet(packet, self.lab.key)
        return packet

    def _hello_packet(self) -> Packet:
        payload = tlv(DEVICE_TYPE_TAG, bytes((DEVICE_PIR,))) + tlv(DEVICE_NAME_TAG, self.name.encode())
        return Packet(msg_type=0x20, src=self.device_id, dst=DEVICE_CENTRAL,
                      boot_id=self.boot_id, seq=self._next_seq(),
                      flags=ACK_REQUIRED_FLAG | RELIABLE_FLAG, payload=payload)

    def _queue_reliable(self, packet: Packet) -> None:
        self.pending.append(packet)
        self._pump()

    def _pump(self) -> None:
        if self.inflight or not self.pending:
            return
        self.inflight = {"packet": self._prepare(self.pending.popleft()), "attempt": 0}
        self._transmit_inflight()

    def _transmit_inflight(self) -> None:
        if not self.inflight:
            return
        self.inflight["attempt"] += 1
        packet = self.inflight["packet"]
        attempt = self.inflight["attempt"]
        self.sent_packets.append(packet)
        self.network.send(self.device_id, DEVICE_CENTRAL, encode(packet))
        token = (packet.seq, attempt)
        self.lab.clock.call_later(self.lab.config.ack_timeout_ms,
                                  lambda: self._retry_if_pending(token))
        self.lab.trace.add("reliable_tx", sender=self.device_id, seq=packet.seq, attempt=attempt)

    def _retry_if_pending(self, token: Tuple[int, int]) -> None:
        if not self.inflight:
            return
        packet = self.inflight["packet"]
        if (packet.seq, self.inflight["attempt"]) != token:
            return
        if self.inflight["attempt"] >= self.lab.config.max_attempts:
            self.timeouts += 1
            self.lab.trace.add("reliable_timeout", sender=self.device_id, seq=packet.seq)
            self.inflight = None
            self._pump()
            return
        self.lab.trace.add("reliable_retry", sender=self.device_id, seq=packet.seq)
        self._transmit_inflight()

    def receive(self, raw: bytes) -> None:
        try:
            packet = decode(raw)
        except ProtocolError as error:
            self.lab.trace.add("emitter_reject", reason=error.code)
            return
        if self.lab.config.auth_required and not check_auth(packet, self.lab.key, AuthPolicy.REQUIRED).accepted:
            self.lab.trace.add("emitter_reject", reason="auth")
            return
        if packet.msg_type == 0x10 and self.inflight:
            expected = self.inflight["packet"]
            if packet.src == DEVICE_CENTRAL and packet.seq == expected.seq:
                self.received_acks += 1
                self.lab.trace.add("ack_received", sender=self.device_id, seq=packet.seq)
                self.inflight = None
                self._pump()

    def send_event(self, event_code: int) -> None:
        packet = Packet(msg_type=0x01, src=self.device_id, dst=DEVICE_CENTRAL,
                        boot_id=self.boot_id, seq=self._next_seq(),
                        flags=ACK_REQUIRED_FLAG | RELIABLE_FLAG,
                        payload=event_payload(event_code))
        self._queue_reliable(packet)
        self.lab.trace.add("emitter_event", event_code=event_code)

    def pir_high(self) -> None:
        if self.pir_state:
            return
        self.pir_state = True
        if self.lab.clock.now_ms - self.last_pir_event >= self.lab.config.pir_debounce_ms:
            self.last_pir_event = self.lab.clock.now_ms
            self.send_event(EVENT_MOTION)

    def pir_low(self) -> None:
        self.pir_state = False

    def press_timbre(self) -> None:
        if self.timbre_state:
            return
        self.timbre_state = True
        if self.lab.clock.now_ms - self.last_timbre_event >= self.lab.config.timbre_debounce_ms:
            self.last_timbre_event = self.lab.clock.now_ms
            self.send_event(EVENT_TIMBRE)

    def release_timbre(self) -> None:
        self.timbre_state = False

    def _heartbeat(self) -> None:
        packet = Packet(msg_type=0x11, src=self.device_id, dst=DEVICE_CENTRAL,
                        boot_id=self.boot_id, seq=self._next_seq(), flags=0x08,
                        payload=tlv(UPTIME_TAG, self.lab.clock.now_ms.to_bytes(4, "big")))
        self.network.send(self.device_id, DEVICE_CENTRAL, encode(self._prepare(packet)))
        self.lab.clock.call_later(self.lab.config.heartbeat_ms, self._heartbeat)


class VirtualCentral:
    def __init__(self, lab: "VirtualLab", boot_id: int = 0x2001):
        self.lab = lab
        self.boot_id = boot_id
        self.mode = "LOCAL"
        self.alarm_mode = "armado"
        self.mqtt_connected = False
        self.mqtt_failures = 0
        self.registry: Dict[int, Dict[str, Any]] = {}
        self.replay = ReplayWindow()
        self.delivered_events: List[Dict[str, Any]] = []
        self.buzzer = VirtualBuzzer(lab.clock, lab.trace)
        self.client = "central_iot"

    @property
    def network(self) -> VirtualNetwork:
        return self.lab.network

    def start(self) -> None:
        self.network.register(DEVICE_CENTRAL, self.receive)
        self.lab.clock.call_later(self.lab.config.mqtt_initial_ms, self._mqtt_attempt)
        self.lab.clock.call_later(1000, self._update_liveness)

    def _mqtt_attempt(self) -> None:
        if self.mqtt_connected:
            return
        lwt = (TOPIC_V3_ESTADO, "offline", True)
        ok = self.lab.mqtt.connect(self.client, self._mqtt_message, lwt=lwt)
        if ok:
            self.mqtt_connected = True
            self.mode = "HA"
            self.mqtt_failures = 0
            for topic in (TOPIC_BOCINA_CMD, TOPIC_V3_BOCINA_CMD, TOPIC_MODO, TOPIC_V3_MODO):
                self.lab.mqtt.subscribe(self.client, topic)
            self._publish_connection_state()
            self._publish_discovery()
            self.lab.trace.add("central_mode", mode="HA")
        else:
            self.mode = "LOCAL"
            self.mqtt_failures += 1
            self.lab.trace.add("central_mode", mode="LOCAL")
        interval = self.lab.config.mqtt_probe_ms if self.mode == "LOCAL" else self.lab.config.mqtt_reconnect_ms
        self.lab.clock.call_later(interval, self._mqtt_attempt)

    def _publish_connection_state(self) -> None:
        for topic, payload in ((TOPIC_ESTADO, "online"), (TOPIC_V3_ESTADO, "online"),
                               (TOPIC_BOCINA_STATE, "ON" if self.buzzer.on else "OFF"),
                               (TOPIC_V3_BOCINA_STATE, "ON" if self.buzzer.on else "OFF"),
                               (TOPIC_MODO_STATE, self.alarm_mode), (TOPIC_V3_MODO_STATE, self.alarm_mode),
                               (TOPIC_IP, "192.168.0.201"), (TOPIC_V3_IP, "192.168.0.201")):
            self.lab.mqtt.publish(self.client, topic, payload, retained=True)

    def _publish_discovery(self) -> None:
        entities = [
            ("binary_sensor", "central_alarma_pir", "Alarma - Movimiento PIR", TOPIC_V3_EVENTO, "detectado"),
            ("binary_sensor", "central_alarma_timbre", "Alarma - Timbre", TOPIC_V3_TIMBRE, "presionado"),
            ("binary_sensor", "central_alarma_online", "Central Alarma - Online", TOPIC_V3_ESTADO, "online"),
            ("switch", "central_alarma_manual", "Alarma - Forzar Bocina", TOPIC_V3_BOCINA_STATE, "ON"),
            ("select", "central_alarma_modo", "Alarma - Modo", TOPIC_V3_MODO_STATE, "armado"),
            ("sensor", "central_alarma_uptime", "Central Alarma - Uptime", TOPIC_V3_UPTIME, None),
            ("sensor", "central_alarma_ip", "Central Alarma - IP", TOPIC_V3_IP, None),
        ]
        for component, unique_id, name, state_topic, payload_on in entities:
            config = {"name": name, "unique_id": unique_id, "state_topic": state_topic,
                      "availability_topic": TOPIC_V3_ESTADO,
                      "device": {"identifiers": ["central_iot"], "name": "Central Alarma IoT",
                                 "manufacturer": "Casero", "model": "ESP8266 NodeMCU", "sw_version": "V4"}}
            if payload_on:
                config["payload_on"] = payload_on
            if unique_id == "central_alarma_manual":
                config["command_topic"] = TOPIC_V3_BOCINA_CMD
            if unique_id == "central_alarma_modo":
                config["command_topic"] = TOPIC_V3_MODO
                config["options"] = ["armado", "desarmado"]
            topic = "homeassistant/{}/{}/config".format(component, unique_id)
            self.lab.mqtt.publish(self.client, topic, json.dumps(config, sort_keys=True), retained=True)

    def _mqtt_message(self, topic: str, payload: str) -> None:
        if topic in (TOPIC_BOCINA_CMD, TOPIC_V3_BOCINA_CMD):
            if payload == "ON":
                self.buzzer.timed_on(self.lab.config.buzzer_motion_ms, "mqtt")
            elif payload == "OFF":
                self.buzzer.off("mqtt")
            self._publish_connection_state()
        elif topic in (TOPIC_MODO, TOPIC_V3_MODO) and payload in ("armado", "desarmado"):
            self.alarm_mode = payload
            self.lab.mqtt.publish(self.client, TOPIC_MODO_STATE, payload, retained=True)
            self.lab.mqtt.publish(self.client, TOPIC_V3_MODO_STATE, payload, retained=True)

    def receive(self, raw: bytes) -> None:
        try:
            packet = decode(raw)
        except ProtocolError as error:
            self.lab.trace.add("central_reject", reason=error.code)
            return
        if self.lab.config.auth_required:
            auth = check_auth(packet, self.lab.key, AuthPolicy.REQUIRED)
            if not auth.accepted:
                self.lab.trace.add("central_reject", reason=auth.reason)
                return

        if packet.dst not in (DEVICE_CENTRAL, 0xFF):
            return
        if packet.msg_type != 0x10 and packet.flags & ACK_REQUIRED_FLAG:
            ack = Packet(msg_type=0x10, src=DEVICE_CENTRAL, dst=packet.src,
                         boot_id=self.boot_id, seq=packet.seq, flags=0, payload=b"")
            self.network.send(DEVICE_CENTRAL, packet.src, encode(self._prepare(ack)))

        if packet.msg_type == 0x10:
            return
        replay_reason = self.replay.observe(packet) if packet.flags & RELIABLE_FLAG else "accepted_unreliable"
        accepted = replay_reason.startswith("accepted_")
        self.registry.setdefault(packet.src, {"state": "UNKNOWN"})
        self.registry[packet.src].update({"boot_id": packet.boot_id, "last_seen": self.lab.clock.now_ms,
                                          "state": "ONLINE"})
        if not accepted:
            self.lab.trace.add("central_duplicate_or_replay", reason=replay_reason, src=packet.src,
                               seq=packet.seq)
            return

        if packet.msg_type == 0x20:
            self.registry[packet.src].update({"name": _string(packet.payload, DEVICE_NAME_TAG),
                                              "device_type": _u8(packet.payload, DEVICE_TYPE_TAG)})
            self.lab.trace.add("hello", src=packet.src)
        elif packet.msg_type == 0x01:
            self._handle_event(packet)
        elif packet.msg_type == 0x11:
            self.lab.trace.add("heartbeat", src=packet.src)
        elif packet.msg_type == 0x13:
            self.lab.trace.add("state_report", src=packet.src)

    def _prepare(self, packet: Packet) -> Packet:
        return sign_packet(packet, self.lab.key) if self.lab.config.auth_required else packet

    def _handle_event(self, packet: Packet) -> None:
        code = _u8(packet.payload, EVENT_TYPE_TAG)
        active = _u8(packet.payload, EVENT_VALUE_TAG, 1)
        event = {"src": packet.src, "code": code, "active": active, "seq": packet.seq,
                 "boot_id": packet.boot_id, "t_ms": self.lab.clock.now_ms}
        self.delivered_events.append(event)
        self.lab.trace.add("event_delivered", src=packet.src, code=code, seq=packet.seq)
        if self.mqtt_connected:
            name = {EVENT_MOTION: "MOTION", EVENT_TIMBRE: "TIMBRE"}.get(code, "UNKNOWN")
            self.lab.mqtt.publish(self.client, "casa/iot/device_{:02X}/evento".format(packet.src),
                                  "{}|{}".format(name, active))
            if active and code == EVENT_MOTION:
                self.lab.mqtt.publish(self.client, TOPIC_V3_EVENTO, "detectado")
            if active and code == EVENT_TIMBRE:
                self.lab.mqtt.publish(self.client, TOPIC_V3_TIMBRE, "presionado")
        if not active:
            return
        if code == EVENT_TIMBRE:
            self.buzzer.timed_on(self.lab.config.buzzer_timbre_ms, "TIMBRE")
        elif code == EVENT_MOTION and self.alarm_mode == "armado":
            self.buzzer.timed_on(self.lab.config.buzzer_motion_ms, "MOTION")

    def _update_liveness(self) -> None:
        for device_id, info in self.registry.items():
            elapsed = self.lab.clock.now_ms - info.get("last_seen", self.lab.clock.now_ms)
            info["state"] = "ONLINE" if elapsed < self.lab.config.stale_ms else (
                "STALE" if elapsed < self.lab.config.offline_ms else "OFFLINE")
        self.lab.clock.call_later(1000, self._update_liveness)


class VirtualLab:
    def __init__(self, seed: int = 7, config: Optional[LabConfig] = None):
        self.config = config or LabConfig()
        self.key = SIMULATOR_KEY
        self.clock = VirtualClock()
        self.trace = Trace(self.clock)
        self.network = VirtualNetwork(self.clock, self.trace, seed=seed,
                                      delay_ms=self.config.network_delay_ms)
        self.mqtt = MemoryMqttBroker(self.trace)
        self.ha = VirtualHA(self.mqtt)
        self.central: Optional[VirtualCentral] = None
        self.emitter: Optional[VirtualEmitter] = None

    def add_central(self, boot_id: int = 0x2001) -> VirtualCentral:
        self.central = VirtualCentral(self, boot_id)
        return self.central

    def add_pir_timbre(self, device_id: int = 0x02, boot_id: int = 0x1001) -> VirtualEmitter:
        self.emitter = VirtualEmitter(self, device_id, boot_id)
        return self.emitter

    def start(self) -> None:
        if not self.central or not self.emitter:
            raise RuntimeError("add_central() and add_pir_timbre() are required before start()")
        self.ha.start()
        self.central.start()
        self.emitter.start()

    def run_until(self, target_ms: int) -> None:
        self.clock.run_until(target_ms)

    def summary(self) -> Dict[str, Any]:
        return {"time_ms": self.clock.now_ms,
                "central_mode": self.central.mode if self.central else None,
                "events": len(self.central.delivered_events) if self.central else 0,
                "buzzer_on": self.central.buzzer.on if self.central else False,
                "mqtt_connected": self.central.mqtt_connected if self.central else False,
                "ha_entities": len(self.ha.entities),
                "registry": {str(k): dict(v) for k, v in (self.central.registry.items() if self.central else [])}}


class ScenarioFailure(AssertionError):
    pass


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ScenarioFailure(message)


def _new_lab(seed: int = 7, auth_required: bool = False) -> Tuple[VirtualLab, VirtualEmitter, VirtualCentral]:
    lab = VirtualLab(seed=seed, config=LabConfig(auth_required=auth_required))
    central = lab.add_central()
    emitter = lab.add_pir_timbre()
    lab.start()
    return lab, emitter, central


def scenario_normal_ha(seed: int) -> str:
    lab, emitter, central = _new_lab(seed)
    lab.clock.call_later(1200, emitter.pir_high)
    lab.run_until(3000)
    _require(central.mode == "HA", "central did not enter HA mode")
    _require(len(central.delivered_events) == 1, "MOTION was not delivered exactly once")
    _require(lab.ha.state("central_alarma_pir") == "detectado", "HA did not receive PIR state")
    _require(len(lab.ha.entities) == 7, "HA discovery did not create seven entities")
    _require(not central.buzzer.on, "MOTION buzzer did not expire")
    return "PIR delivered once, buzzer timed, MQTT/HA discovery active"


def scenario_pir_sustained(seed: int) -> str:
    lab, emitter, central = _new_lab(seed)
    lab.clock.call_later(100, emitter.pir_high)
    lab.clock.call_later(2200, emitter.pir_high)
    lab.run_until(2600)
    _require(len(central.delivered_events) == 1, "sustained PIR generated repeated events")
    return "sustained PIR generated one event"


def scenario_simultaneous_inputs(seed: int) -> str:
    lab, emitter, central = _new_lab(seed)
    lab.clock.call_later(200, emitter.pir_high)
    lab.clock.call_later(200, emitter.press_timbre)
    lab.run_until(3000)
    codes = [event["code"] for event in central.delivered_events]
    _require(codes.count(EVENT_MOTION) == 1 and codes.count(EVENT_TIMBRE) == 1,
             "PIR and timbre were not delivered independently")
    return "PIR and timbre delivered as independent events"


def scenario_loss_first_event_retry(seed: int) -> str:
    lab, emitter, central = _new_lab(seed)
    lab.run_until(100)
    lab.network.drop_first(emitter.device_id, DEVICE_CENTRAL, 0x01, 1)
    emitter.press_timbre()
    lab.run_until(2500)
    _require(len(central.delivered_events) == 1, "lost event was not recovered exactly once")
    _require(lab.trace.count("reliable_retry", sender=emitter.device_id) >= 1,
             "no reliable retry was observed")
    return "first EVENT dropped, retry delivered once and ACKed"


def scenario_ack_loss_duplicate(seed: int) -> str:
    lab, emitter, central = _new_lab(seed)
    lab.run_until(100)
    lab.network.drop_first(DEVICE_CENTRAL, emitter.device_id, 0x10, 1)
    emitter.press_timbre()
    lab.run_until(2500)
    _require(len(central.delivered_events) == 1, "duplicate retry caused a second alarm effect")
    _require(lab.trace.count("central_duplicate_or_replay") >= 1,
             "lost ACK did not produce an observable duplicate")
    return "lost ACK caused duplicate delivery attempt but one application effect"


def scenario_mqtt_local_recovery(seed: int) -> str:
    lab, emitter, central = _new_lab(seed)
    lab.mqtt.set_available(False)
    lab.clock.call_later(200, emitter.press_timbre)
    lab.run_until(2500)
    _require(central.mode == "LOCAL", "central did not keep LOCAL mode with broker down")
    _require(len(central.delivered_events) == 1 and central.buzzer.transitions,
             "alarm did not work without MQTT")
    lab.mqtt.set_available(True)
    lab.run_until(6000)
    _require(central.mode == "HA", "central did not recover to HA")
    _require(len(lab.ha.entities) == 7, "discovery was not republished after recovery")
    return "LOCAL alarm survived broker outage and recovered to HA"


def scenario_auth_required(seed: int) -> str:
    lab, emitter, central = _new_lab(seed, auth_required=True)
    lab.clock.call_later(200, emitter.press_timbre)
    lab.run_until(2500)
    _require(len(central.delivered_events) == 1, "valid authenticated event was rejected")
    _require(lab.trace.count("central_reject") == 0, "authenticated traffic was rejected")
    return "authenticated HELLO/EVENT/ACK flow accepted"


def scenario_replay_rejected(seed: int) -> str:
    lab, emitter, central = _new_lab(seed)
    lab.run_until(100)
    emitter.press_timbre()
    lab.run_until(500)
    event_packet = next(packet for packet in emitter.sent_packets if int(packet.msg_type) == 0x01)
    for seq in range(10, 19):
        heartbeat = Packet(msg_type=0x11, src=emitter.device_id, dst=DEVICE_CENTRAL,
                           boot_id=emitter.boot_id, seq=seq, flags=RELIABLE_FLAG,
                           payload=tlv(UPTIME_TAG, seq.to_bytes(4, "big")))
        central.receive(encode(heartbeat))
    central.receive(encode(event_packet))
    _require(len(central.delivered_events) == 1, "old EVENT replay caused a second effect")
    _require(lab.trace.count("central_duplicate_or_replay") >= 1,
             "old EVENT replay was not rejected")
    return "replay outside the window rejected without second effect"


SCENARIO_FUNCTIONS = {
    "normal-ha": scenario_normal_ha,
    "pir-sustained": scenario_pir_sustained,
    "simultaneous-inputs": scenario_simultaneous_inputs,
    "loss-first-event": scenario_loss_first_event_retry,
    "ack-loss-duplicate": scenario_ack_loss_duplicate,
    "mqtt-local-recovery": scenario_mqtt_local_recovery,
    "auth-required": scenario_auth_required,
    "replay-rejected": scenario_replay_rejected,
}
SCENARIOS = tuple(SCENARIO_FUNCTIONS)


def run_scenarios(selected: str = "all", seed: int = 7) -> List[Dict[str, Any]]:
    names = list(SCENARIOS) if selected == "all" else [selected]
    unknown = [name for name in names if name not in SCENARIO_FUNCTIONS]
    if unknown:
        raise ValueError("unknown scenario: {}".format(", ".join(unknown)))
    results = []
    for name in names:
        lab = None
        try:
            detail = SCENARIO_FUNCTIONS[name](seed)
            # The scenario creates its own lab; this branch is filled by the
            # lightweight result below. Detailed traces are available through
            # the direct VirtualLab API and are intentionally not duplicated in
            # normal CLI output.
            results.append({"name": name, "passed": True, "detail": detail})
        except Exception as error:
            results.append({"name": name, "passed": False, "detail": str(error)})
    return results
