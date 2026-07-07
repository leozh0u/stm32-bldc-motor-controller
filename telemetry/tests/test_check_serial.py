"""
Tests for check_serial.py's read/parse loop and FrameParser's health
counters, driven entirely by MockSerial (no hardware, no matplotlib).
"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from protocol import pack_frame, FrameParser
from serial_source import MockSerial
from check_serial import run


def test_parser_counters_clean_stream():
    parser = FrameParser()
    for t in range(10):
        list(parser.feed(pack_frame(t * 20, 100, 50, 150)))
    assert parser.frames_ok == 10
    assert parser.errors == 0


def test_parser_counters_count_corruption():
    parser = FrameParser()
    good = pack_frame(0, 100, 50, 150)
    bad = bytearray(good)
    bad[5] ^= 0xFF                       # corrupt a payload byte -> checksum fail
    list(parser.feed(bytes(bad) + good))
    assert parser.frames_ok == 1
    assert parser.errors >= 1


def test_run_over_mock_stream():
    ser = MockSerial(n_frames=300, corrupt_every=25)
    parser = FrameParser()
    last = run(ser, parser, seconds=60)  # mock exhausts long before 60s
    assert last is not None
    assert parser.frames_ok > 250        # ~12 corrupted of 300
    assert parser.errors > 0
    assert isinstance(last['rpm'], int)
