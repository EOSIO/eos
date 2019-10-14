#! /usr/bin/env python3

# standard libraries
import threading

# class Thread(threading.Thread):
#     id = 0
#     channel = {}

#     def __init__(self, target, name=None, args=(), kwargs={}, *, daemon=None):
#         self.id = Thread.id
#         Thread.id += 1 # thanks to Python's GIL we may do this without worrying about race condition
#         super().__init__(target=target, name=name, args=args, kwargs=kwargs, daemon=daemon)

#     def run(self):
#         super().run()



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
        except Exception:
            self.report(self.channel, self.id)
