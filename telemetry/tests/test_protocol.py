import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from protocol import pack_frame, FrameParser
from mock_stream import generate_mock_stream

def test_round_trip():
    frame = pack_frame(1000, 150, 75, 150)
    parser = FrameParser()
    results = list(parser.feed(frame))
    assert len(results) == 1
    assert results[0]['timestamp_ms'] == 1000
    assert results[0]['rpm'] == 150
    assert results[0]['pwm_duty'] == 75
    assert results[0]['setpoint_rpm'] == 150

def test_split_across_reads():
    frame = pack_frame(500, 100, 50, 100)
    parser = FrameParser()
    results = []
    for i in range(0, len(frame), 3):  # feed 3 bytes at a time
        results.extend(parser.feed(frame[i:i+3]))
    assert len(results) == 1
    assert results[0]['rpm'] == 100

def test_resync_after_corruption():
    chunks = generate_mock_stream(n_frames=50, corrupt_every=10)
    parser = FrameParser()
    results = []
    for chunk in chunks:
        results.extend(parser.feed(chunk))
    # expect fewer than 50 due to corrupted frames being dropped,
    # but parser must not crash and must recover valid frames after corruption
    assert 0 < len(results) < 50
    for r in results:
        assert isinstance(r['rpm'], int)

def test_garbage_prefix_does_not_break_parser():
    garbage = bytes([0x00, 0xFF, 0x12, 0x34])
    frame = pack_frame(2000, 200, 80, 200)
    parser = FrameParser()
    results = list(parser.feed(garbage + frame))
    assert len(results) == 1
    assert results[0]['timestamp_ms'] == 2000

if __name__ == '__main__':
    test_round_trip()
    test_split_across_reads()
    test_resync_after_corruption()
    test_garbage_prefix_does_not_break_parser()
    print("All tests passed.")
