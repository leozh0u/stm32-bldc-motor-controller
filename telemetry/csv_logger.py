"""CSV telemetry logger. Appends parsed frames to disk."""
import csv
import os

FIELDNAMES = ['timestamp_ms', 'rpm', 'pwm_duty', 'setpoint_rpm']

class CSVLogger:
    def __init__(self, path: str):
        self.path = path
        write_header = not os.path.exists(path)
        self.file = open(path, 'a', newline='')
        self.writer = csv.DictWriter(self.file, fieldnames=FIELDNAMES)
        if write_header:
            self.writer.writeheader()
            self.file.flush()

    def log(self, frame: dict):
        self.writer.writerow(frame)
        self.file.flush()

    def close(self):
        self.file.close()
