"""
Generates fake framed telemetry bytes to test the pipeline with no
hardware attached. Simulates PID settling toward a setpoint, and
optionally injects byte corruption to test parser resync.
"""
import random
from protocol import pack_frame

def generate_mock_stream(n_frames=200, setpoint_rpm=150, corrupt_every=0):
    """
    Yields raw bytes chunks (randomly sized, like real serial reads).
    corrupt_every: if >0, flips a random byte every N frames to test resync.
    """
    rpm = 0
    t = 0
    all_bytes = bytearray()

    for i in range(n_frames):
        rpm += (setpoint_rpm - rpm) * 0.1 + random.uniform(-2, 2)
        pwm = min(100, max(0, int((setpoint_rpm - rpm) * 2 + 50)))
        frame = bytearray(pack_frame(t, int(rpm), pwm, setpoint_rpm))

        if corrupt_every and i % corrupt_every == 0 and i > 0:
            idx = random.randint(0, len(frame) - 1)
            frame[idx] ^= 0xFF  # flip bits to corrupt

        all_bytes.extend(frame)
        t += 20

    # split into random-sized chunks to simulate real serial.read()
    chunks = []
    pos = 0
    while pos < len(all_bytes):
        size = random.randint(3, 20)
        chunks.append(bytes(all_bytes[pos:pos+size]))
        pos += size
    return chunks
