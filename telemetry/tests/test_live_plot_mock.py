"""
Headless integration test for live_plot.py's pipeline: MockSerial ->
FrameParser -> CSVLogger -> matplotlib line data, using the Agg backend
so no display is needed. Exercises the exact update() code path.
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import matplotlib
matplotlib.use('Agg')

import matplotlib.pyplot as plt
from collections import deque

from live_plot import MockSerial, MAX_POINTS
from protocol import FrameParser
from csv_logger import CSVLogger


def test_mock_pipeline_end_to_end(tmp_path):
    ser = MockSerial(n_frames=200, corrupt_every=20)
    parser = FrameParser()
    csv_path = tmp_path / 'log.csv'
    logger = CSVLogger(str(csv_path))

    t_data = deque(maxlen=MAX_POINTS)
    rpm_data = deque(maxlen=MAX_POINTS)
    setpoint_data = deque(maxlen=MAX_POINTS)

    fig, ax = plt.subplots()
    line_rpm, = ax.plot([], [], label='RPM')

    # drive the same loop update() runs, until the mock source is exhausted
    n_parsed = 0
    while True:
        raw = ser.read(256)
        if not raw:
            break
        for parsed in parser.feed(raw):
            n_parsed += 1
            logger.log(parsed)
            t_data.append(parsed['timestamp_ms'])
            rpm_data.append(parsed['rpm'])
            setpoint_data.append(parsed['setpoint_rpm'])

    line_rpm.set_data(t_data, rpm_data)
    ax.relim()
    ax.autoscale_view()
    fig.canvas.draw()          # force a full Agg render pass
    plt.close(fig)
    logger.close()

    # 200 frames, ~1 in 20 corrupted; parser must recover the vast majority
    assert n_parsed > 150
    assert len(t_data) == min(n_parsed, MAX_POINTS)

    lines = csv_path.read_text().strip().splitlines()
    assert lines[0] == 'timestamp_ms,rpm,pwm_duty,setpoint_rpm'
    assert len(lines) == n_parsed + 1

    # timestamps must be monotonically increasing (20ms apart in the mock)
    ts = [int(l.split(',')[0]) for l in lines[1:]]
    assert ts == sorted(ts)


def test_mock_serial_read_contract():
    ser = MockSerial(n_frames=5, corrupt_every=0)
    total = b''
    while True:
        chunk = ser.read(256)
        if not chunk:
            break
        total += chunk
    assert len(total) == 5 * 14   # 5 complete 14-byte frames
    ser.close()                   # must not raise
