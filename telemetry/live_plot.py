"""
Live serial telemetry reader + plotter.
Reads framed bytes from USART2 (via ST-LINK VCP), parses, plots RPM vs setpoint.

--mock replaces the serial port with synthetic frames from mock_stream, so the
full parse -> log -> plot pipeline runs with no hardware attached.
"""
import argparse
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque

from protocol import FrameParser
from csv_logger import CSVLogger
from serial_source import open_source

MAX_POINTS = 300

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--port', default=None, help='Serial port (auto-detects usbmodem if omitted)')
    ap.add_argument('--csv', default='telemetry_log.csv', help='CSV output path')
    ap.add_argument('--mock', action='store_true', help='Use synthetic mock_stream data instead of serial')
    args = ap.parse_args()

    ser = open_source(args)
    parser = FrameParser()
    logger = CSVLogger(args.csv)

    t_data = deque(maxlen=MAX_POINTS)
    rpm_data = deque(maxlen=MAX_POINTS)
    setpoint_data = deque(maxlen=MAX_POINTS)

    fig, ax = plt.subplots()
    line_rpm, = ax.plot([], [], label='RPM')
    line_setpoint, = ax.plot([], [], label='Setpoint', linestyle='--')
    ax.set_xlabel('Time (ms)')
    ax.set_ylabel('RPM')
    ax.legend()

    def update(frame_num):
        raw = ser.read(256)
        if raw:
            for parsed in parser.feed(raw):
                logger.log(parsed)
                t_data.append(parsed['timestamp_ms'])
                rpm_data.append(parsed['rpm'])
                setpoint_data.append(parsed['setpoint_rpm'])

        if t_data:
            line_rpm.set_data(t_data, rpm_data)
            line_setpoint.set_data(t_data, setpoint_data)
            ax.relim()
            ax.autoscale_view()
        return line_rpm, line_setpoint

    ani = FuncAnimation(fig, update, interval=50, cache_frame_data=False)
    try:
        plt.show()
    finally:
        logger.close()
        ser.close()

if __name__ == '__main__':
    main()
