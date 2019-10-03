#! /usr/bin/env python3


from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any, List, Optional, Tuple, TypeVar, Union

import inspect
import os.path
import queue
import random
import threading
import time


def get_current_time(date=True, precision=3, time_zone=True):
    now = time.time()
    form = "%Y-%m-%d %H:%M:%S" if date else "%H:%M:%S"
    if precision:
        subsec = round((now - int(now)) * (10 ** precision))
        form = "{{}}.{{:0{}d}}".format(precision).format(form, subsec)
    if time_zone:
        form = " ".join([form, "%Z"])
    return time.strftime(form, time.localtime(now))


class Thread(threading.Thread):
    id = 0
    channel = {}

    def __init__(self, target, name=None, args=(), kwargs={}, *, daemon=None):
        self.id = Thread.id
        Thread.id += 1 # thanks to Python's GIL we may do this without worrying about race condition
        super().__init__(target=target, name=name, args=args, kwargs=kwargs, daemon=daemon)

    def run(self):
        super().run()


@dataclass
class LoggingConfig:
    show_elapsed_time:  bool    = True
    show_clock_time:    bool    = True
    show_thread:        bool    = True
    show_trace:         bool    = True
    show_log_level:     bool    = True
    start_time:         float   = time.time()
    buffered:           bool    = None
    to_screen:          bool    = None


class MessageCenter:
    def __init__(self, writer: TypeVar("Writer")):
        self.center = dict()
        self.writer = writer

    def store(self, msg):
        thread_id = threading.get_ident()
        if thread_id in self.center:
            self.center[thread_id].put(msg)
        else:
            self.center[thread_id] = queue.Queue()
            self.store(msg)

    def flush(self):
        thread_id = threading.get_ident()
        self.store("")
        while True:
            item = self.center[thread_id].get()
            if item != "":
                self.writer.raw_write(item)
            else:
                return


class Writer(ABC):
    def __init__(self, config: LoggingConfig = None):
        self.config = config if config else LoggingConfig()
        if self.config.buffered:
            self._message_center = MessageCenter(self)
            self._rlock = threading.RLock()
        if self.config.buffered:
            self._message_center = MessageCenter(self)
            self._rlock = threading.RLock()

    def prepend(self, msg, level=None):
        prefix = str()
        if self.config.show_clock_time:
            prefix += "{} ".format(get_current_time())
        if self.config.show_elapsed_time:
            prefix += "({:8.2f} s) ".format(time.time() - self.config.start_time)
        if self.config.show_thread:
            prefix += "[{:8}] ".format((threading.current_thread().name))
        if self.config.show_trace:
            frame = inspect.stack()[4]
            prefix += "{:<15}{:10} ".format(os.path.basename(frame.filename) + ":" + str(frame.lineno), frame.function + "()")
        if self.config.show_log_level:
            prefix += "{:5} ".format(level if self.config.show_log_level and level else "")
        if prefix:
            return prefix + " | " + msg
        else:
            return msg

    def write(self, msg, level=None):
        if self.config.buffered:
            self.buffered_write(msg, level)
        else:
            self.unbuffered_write(msg, level)

    @abstractmethod
    def raw_write(self, msg):
        pass

    def unbuffered_write(self, msg, level=None):
        self.raw_write(self.prepend(msg, level))

    def buffered_write(self, msg, level=None):
        self._message_center.store(self.prepend(msg, level))

    def flush(self):
        if self.config.buffered:
            with self._rlock:
                self._message_center.flush()


class ScreenWriter(Writer):
    def __init__(self, config: LoggingConfig = None):
        super().__init__(config)
        self.config.to_screen = True

    def raw_write(self, msg):
        print(msg)


class FileWriter(Writer):
    def __init__(self, filename="test.log", config: LoggingConfig = None):
        super().__init__(config)
        self.config.to_screen = False
        self.filename = filename

    def raw_write(self, msg):
        with open(self.filename, "a") as file:
            file.write(msg + "\n")


class Logger:
    start_time = time.time()

    def __init__(self, *writers: Tuple[Writer]):
        self.writers = writers
        for w in self.writers:
            w.config.start_time = Logger.start_time

    def log(self, msg, level=None):
        for w in self.writers:
            w.write(msg, level)

    def flush(self):
        for w in self.writers:
            w.flush()


# ----------------- test ------------------------------------------------------


def _pause(wait=None):
    if wait:
        time.sleep(wait)
    else:
        time.sleep(random.randint(0, 3))


def _alice(logger):
    logger.log("Hello")
    _pause()
    logger.log("My name is Alice.", level="DEBUG")
    logger.flush()


def _bob(logger):
    logger.log("Hi, everyone")
    _pause()
    logger.log("I'm Bob.", level="DEBUG")
    logger.flush()


def main():
    config = LoggingConfig(buffered=True)
    logger = Logger(ScreenWriter(config=config), FileWriter(config=config))

    t_alice = Thread(target=_alice, args=(logger,))
    t_bob = Thread(target=_bob, args=(logger,))

    for t in (t_alice, t_bob):
        t.start()

    for t in (t_alice, t_bob):
        t.join()


if __name__ == "__main__":
    main()
