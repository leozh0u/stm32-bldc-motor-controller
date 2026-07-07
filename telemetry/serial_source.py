"""
Serial-source helpers shared by live_plot.py and check_serial.py:
ST-LINK VCP auto-detection and a mock source that duck-types
serial.Serial over mock_stream chunks. Imports pyserial lazily so
--mock works with no serial hardware or pyserial installed.
"""
import sys

BAUD = 115200


class MockSerial:
    """Duck-types serial.Serial.read()/close() over mock_stream chunks."""
    def __init__(self, n_frames=500, corrupt_every=25):
        from mock_stream import generate_mock_stream
        self._chunks = iter(generate_mock_stream(n_frames=n_frames,
                                                 corrupt_every=corrupt_every))

    def read(self, _size):
        return next(self._chunks, b'')

    def close(self):
        pass


def find_port():
    import serial.tools.list_ports
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if 'usbmodem' in p.device:
            return p.device
    return None


def open_source(args):
    """Returns a serial.Serial or MockSerial per CLI args (needs .mock/.port)."""
    if args.mock:
        print("Mock mode: synthetic telemetry, no hardware")
        return MockSerial()
    import serial
    port = args.port or find_port()
    if port is None:
        print("No usbmodem serial port found. Pass --port explicitly.")
        sys.exit(1)
    print(f"Connecting to {port} @ {BAUD} baud")
    return serial.Serial(port, BAUD, timeout=0.1)
