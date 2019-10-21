#! /usr/bin/env python3

# standard libraries
import threading

class ExceptionThread(threading.Thread):
    id = 0

    def __init__(self, channel, report, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.id = ExceptionThread.id
        self.channel = channel
        self.report = report
        ExceptionThread.id += 1

    def run(self):
        try:
            super().run()
        except Exception as e:
            self.report(self.channel, self.id, str(e))
