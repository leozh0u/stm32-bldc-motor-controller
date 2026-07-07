"""
UART telemetry protocol: framed binary, XOR checksum, stream resync.
Frame: START(1) LEN(1) PAYLOAD(10) CHECKSUM(1) END(1) = 14 bytes
Payload: uint32 timestamp_ms, int16 rpm, int16 pwm_duty, int16 setpoint_rpm
"""
import struct

START = 0xAA
END = 0x55
PAYLOAD_LEN = 10

def checksum(payload: bytes) -> int:
    c = 0
    for b in payload:
        c ^= b
    return c

def pack_frame(timestamp_ms: int, rpm: int, pwm_duty: int, setpoint_rpm: int) -> bytes:
    payload = struct.pack('<Ihhh', timestamp_ms, rpm, pwm_duty, setpoint_rpm)
    cs = checksum(payload)
    return bytes([START, PAYLOAD_LEN]) + payload + bytes([cs, END])

def unpack_payload(payload: bytes) -> dict:
    timestamp_ms, rpm, pwm_duty, setpoint_rpm = struct.unpack('<Ihhh', payload)
    return {
        'timestamp_ms': timestamp_ms,
        'rpm': rpm,
        'pwm_duty': pwm_duty,
        'setpoint_rpm': setpoint_rpm,
    }


class FrameParser:
    """
    Incremental state-machine parser. Feed it raw bytes as they arrive
    from serial (which may split frames across reads); it emits parsed
    dicts and silently resyncs past corrupted frames.

    Counters (for link health checks):
      frames_ok — frames parsed successfully
      errors    — resyncs caused by bad length, checksum, or end byte
    """
    WAIT_START, WAIT_LEN, WAIT_PAYLOAD, WAIT_CHECKSUM, WAIT_END = range(5)

    def __init__(self):
        self.state = self.WAIT_START
        self.payload = bytearray()
        self.expected_len = 0
        self.checksum_byte = 0
        self.frames_ok = 0
        self.errors = 0

    def feed(self, data: bytes):
        """Yield parsed telemetry dicts found in `data`."""
        for b in data:
            frame = self._feed_byte(b)
            if frame is not None:
                yield frame

    def _reset(self):
        self.state = self.WAIT_START
        self.payload = bytearray()
        self.expected_len = 0

    def _feed_byte(self, b: int):
        if self.state == self.WAIT_START:
            if b == START:
                self.state = self.WAIT_LEN
            # else: not a start byte, discard and keep waiting

        elif self.state == self.WAIT_LEN:
            self.expected_len = b
            if self.expected_len != PAYLOAD_LEN:
                self.errors += 1
                self._reset()  # malformed length, resync
            else:
                self.state = self.WAIT_PAYLOAD

        elif self.state == self.WAIT_PAYLOAD:
            self.payload.append(b)
            if len(self.payload) == self.expected_len:
                self.state = self.WAIT_CHECKSUM

        elif self.state == self.WAIT_CHECKSUM:
            self.checksum_byte = b
            self.state = self.WAIT_END

        elif self.state == self.WAIT_END:
            if b == END and self.checksum_byte == checksum(bytes(self.payload)):
                frame = unpack_payload(bytes(self.payload))
                self.frames_ok += 1
                self._reset()
                return frame
            else:
                self.errors += 1
                self._reset()  # bad end byte or checksum mismatch, resync

        return None
