"""
Serial link smoke test — no matplotlib needed. First tool to run during
hardware bring-up: confirms frames are arriving, well-formed, and sane
before firing up the live plotter.

Reads for --seconds (default 5), prints each parsed frame's fields at
~2Hz plus running ok/error counts, then a summary. Exit code 0 if at
least one valid frame arrived and the error rate was < 50%, else 1.

  .venv/bin/python check_serial.py            # live from ST-LINK VCP
  .venv/bin/python check_serial.py --mock     # synthetic data, no board
"""
import sys
import time
import argparse

from protocol import FrameParser
from serial_source import open_source


def run(ser, parser, seconds, now=time.monotonic):
    """Read/parse loop. Returns the last parsed frame (or None).

    Split from main() so tests can drive it with MockSerial and a fake
    clock. Stops early if the source dries up (mock exhaustion).
    """
    deadline = now() + seconds
    next_print = now()
    last = None

    while now() < deadline:
        raw = ser.read(256)
        if not raw:
            # real serial.Serial has .port and idles on timeout; MockSerial
            # has no .port and an empty read means the stream is exhausted
            if getattr(ser, 'port', None) is None:
                break
            continue
        for frame in parser.feed(raw):
            last = frame
        if now() >= next_print and last is not None:
            next_print = now() + 0.5
            print(f"ok={parser.frames_ok} err={parser.errors}  "
                  f"t={last['timestamp_ms']}ms rpm={last['rpm']} "
                  f"duty={last['pwm_duty']} sp={last['setpoint_rpm']}")
    return last


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--port', default=None,
                    help='Serial port (auto-detects usbmodem if omitted)')
    ap.add_argument('--mock', action='store_true',
                    help='Use synthetic mock_stream data instead of serial')
    ap.add_argument('--seconds', type=float, default=5.0,
                    help='How long to listen (default 5)')
    args = ap.parse_args()

    ser = open_source(args)
    parser = FrameParser()
    try:
        run(ser, parser, args.seconds)
    finally:
        ser.close()

    total = parser.frames_ok + parser.errors
    print(f"\n{parser.frames_ok} valid frames, {parser.errors} errors "
          f"in {args.seconds:g}s")
    if parser.frames_ok == 0:
        print("FAIL: no valid frames. Check wiring, baud (115200), and that "
              "firmware is running (not in MODE_CALIBRATE_CPR, which sends "
              "text, not frames).")
        sys.exit(1)
    if parser.errors * 2 > total:
        print("FAIL: error rate over 50% — noisy or misconfigured link.")
        sys.exit(1)
    print("PASS: link healthy.")


if __name__ == '__main__':
    main()
