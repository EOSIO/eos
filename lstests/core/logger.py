#! /usr/bin/env python3

# standard libraries
import abc
# import dataclasses
import enum
import inspect
import os
import queue
import random
import re
import threading
import time
import typing

# user-defined modules
import color
import helper

# VERTICAL_BAR = "|"
VERTICAL_BAR = "│"

class LogLevel(enum.IntEnum):
    ALL   = 0
    TRACE = 10
    DEBUG = 20
    INFO  = 30
    WARN  = 40
    ERROR = 50
    FATAL = 60
    OFF   = 100

    def __str__(self):
        return self.name

    @staticmethod
    def to_int(x: typing.Union[int, str, typing.TypeVar("LogLevel")]):
        """make best effort to convert an int, str or LogLevel to int value"""
        if isinstance(x, int):
            return int(x)
        if isinstance(x, str):
            try:
                return int(LogLevel[x.upper()])
            except KeyError:
                return -1
        return -1


    @staticmethod
    def to_str(x: typing.Union[int, str, typing.TypeVar("LogLevel")]):
        """make best effort to convert an int, str or LogLevel to name"""
        if isinstance(x, LogLevel):
            return str(x)
        if isinstance(x, int):
            try:
                return LogLevel.to_str(LogLevel(x))
            except ValueError:
                return str(x)
        if isinstance(x, str):
            try:
                return LogLevel.to_str(LogLevel[x.upper()])
            except KeyError:
                return ""


class MessageCenter:
    def __init__(self, writer: typing.TypeVar("Writer")):
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


# @dataclasses.dataclass
# class WriterConfig:
#     threshold:          typing.Union[int, str, LogLevel]
#     buffered:           bool
#     monochrome:         bool
#     show_clock_time:    bool = True
#     show_elapsed_time:  bool = True
#     show_filename:      bool = True
#     show_lineno:        bool = True
#     show_func:      bool = True
#     show_thread:        bool = True
#     show_log_level:     bool = True

#     def __post_init__(self):
#         if isinstance(self.threshold, str):
#             self.threshold = LogLevel[self.threshold]


class Writer(abc.ABC):
    def __init__(self,
                 threshold: typing.Union[int, str, LogLevel],
                 buffered: bool = True,
                 monochrome: bool = False,
                 show_clock_time: bool = True,
                 show_elapsed_time: bool = True,
                 show_filename: bool = True,
                 show_lineno: bool = True,
                 show_func: bool = True,
                 show_thread: bool = True,
                 show_log_level: bool = True):
        self.threshold = LogLevel.to_int(threshold)
        self.buffered = buffered
        self.monochrome = monochrome
        self.show_clock_time = show_clock_time
        self.show_elapsed_time = show_elapsed_time
        self.show_filename = show_filename
        self.show_lineno = show_lineno
        self.show_func = show_func
        self.show_thread = show_thread
        self.show_log_level = show_log_level
        self.start_time = time.time()

        self._color_regex = re.compile(r"\033\[[0-9]+(;[0-9]+)?m")
        if self.buffered:
            self._message_center = MessageCenter(self)
            self._rlock = threading.RLock()

    def prepend(self, msg, level):
        def stylize(prefix, level):
            if level <= LogLevel.DEBUG:
                prefix =color.faint(prefix)
            elif level == LogLevel.WARN:
                prefix = color.yellow(prefix)
            elif level == LogLevel.ERROR:
                prefix = color.red(prefix)
            elif level == LogLevel.FATAL:
                prefix = color.bold(color.red(prefix))
            return prefix
        if self.monochrome:
            msg = self._color_regex.sub("", msg)
        prefix = str()
        if self.show_clock_time:
            prefix += "{} ".format(helper.get_current_time())
        if self.show_elapsed_time:
            prefix += "({:9.3f}s) ".format(time.time() - self.start_time)
        depth = len(inspect.stack()) - 1
        frame = inspect.stack()[min(6, depth)]
        if self.show_filename:
            prefix += "{:10.10s}:".format(os.path.basename(frame.filename))
        if self.show_lineno:
            prefix += "{:4.4s} ".format(str(frame.lineno))
        if self.show_func:
            prefix += "{:18.18s} ".format(helper.compress(frame.function) + "()")
        if self.show_thread:
            prefix += "[{:10}] ".format((threading.current_thread().name))
        if self.show_log_level:
            prefix += "{:5} ".format(LogLevel.to_str(level))
        if prefix:
            prefix = prefix if self.monochrome else stylize(prefix, level)
            return "\n".join([prefix + VERTICAL_BAR + " " + x for x in msg.splitlines()])
        else:
            return msg

    def write(self, msg, level):
        if level >= self.threshold:
            if self.buffered:
                self.buffered_write(msg, level)
            else:
                self.unbuffered_write(msg, level)

    @abc.abstractmethod
    def raw_write(self, msg):
        pass

    def unbuffered_write(self, msg, level):
        self.raw_write(self.prepend(msg, level))

    def buffered_write(self, msg, level):
        self._message_center.store(self.prepend(msg, level))

    def flush(self):
        if self.buffered:
            with self._rlock:
                self._message_center.flush()


class ScreenWriter(Writer):
    def __init__(self,
                 threshold: typing.Union[int, str, LogLevel],
                 buffered: bool = True,
                 monochrome: bool = False,
                 show_clock_time: bool = True,
                 show_elapsed_time: bool = True,
                 show_filename: bool = True,
                 show_lineno: bool = True,
                 show_func: bool = True,
                 show_thread: bool = True,
                 show_log_level: bool = True):
        super().__init__(threshold,
                         buffered,
                         monochrome,
                         show_clock_time,
                         show_elapsed_time,
                         show_filename,
                         show_lineno,
                         show_func,
                         show_thread,
                         show_log_level)

    def raw_write(self, msg):
        print(msg)


class FileWriter(Writer):
    def __init__(self,
                 filename,
                 threshold: typing.Union[int, str, LogLevel],
                 buffered: bool = True,
                 monochrome: bool = False,
                 show_clock_time: bool = True,
                 show_elapsed_time: bool = True,
                 show_filename: bool = True,
                 show_lineno: bool = True,
                 show_func: bool = True,
                 show_thread: bool = True,
                 show_log_level: bool = True):
        super().__init__(threshold,
                         buffered,
                         monochrome,
                         show_clock_time,
                         show_elapsed_time,
                         show_filename,
                         show_lineno,
                         show_func,
                         show_thread,
                         show_log_level)
        self.filename = filename

    def raw_write(self, msg):
        with open(self.filename, "a") as f:
            print(msg, file=f)


class Logger:
    def __init__(self, *writers: typing.Tuple[Writer]):
        self.writers = writers
        start_time = time.time()
        for w in self.writers:
            w.start_time = start_time

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        self.flush()

    def log(self, msg, level: typing.Union[int, str, LogLevel], buffer=False):
        level = LogLevel.to_int(level)
        for w in self.writers:
            w.write(str(msg), level)
        if not buffer:
            self.flush()

    def trace(self, msg, buffer=False):
        self.log(msg, level=LogLevel.TRACE, buffer=buffer)

    def debug(self, msg, buffer=False):
        self.log(msg, level=LogLevel.DEBUG, buffer=buffer)

    def info(self, msg, buffer=False):
        self.log(msg, level=LogLevel.INFO, buffer=buffer)

    def warn(self, msg, colorize=True, buffer=False):
        self.log(color.yellow(msg) if colorize else msg, level=LogLevel.WARN, buffer=buffer)

    def error(self, msg, colorize=True, buffer=False, assert_false=None):
        # assert_false implies no buffer (must flush immediately)
        buffer = not helper.override(not buffer, assert_false)
        self.log(color.red(msg)if colorize else msg, level=LogLevel.ERROR, buffer=buffer)
        if assert_false:
            assert False

    def fatal(self, msg, colorize=True, buffer=False, assert_false=True):
        # assert_false implies flush
        buffer = not helper.override(not buffer, assert_false)
        self.log(color.bold(color.red(msg)) if colorize else msg, level=LogLevel.FATAL, buffer=buffer)
        if assert_false:
            assert False

    def flush(self):
        for w in self.writers:
            w.flush()




# ----------------- test ------------------------------------------------------


def _pause(wait=None):
    time.sleep(random.randint(0, 3) if wait is None else wait)


def _alice(logger):
    logger.info("Alice:\tHello!")
    _pause()
    logger.warn("Alice:\tMy name is {}.".format(color.yellow("Alice")))
    logger.flush()
    _pause()
    logger.debug(color.red("Alice:") + "\nThis is a \nMultiline\nStatement")
    logger.flush()


def _bob(logger):
    logger.log(color.green("Bob:\tHi, everyone."), level=38)
    _pause()
    logger.debug("Bob:\tI'm {}.".format(color.blue("Bob")))
    logger.error("Bob:\tNice to meet you")
    logger.flush()
    logger.trace("Bob:\tThis is a trace information.")
    logger.flush()


def test():
    logger = Logger(ScreenWriter(threshold="debug"), FileWriter(filename="trace.log", threshold="trace"))

    t_alice = threading.Thread(target=_alice, args=(logger,))
    t_bob = threading.Thread(target=_bob, args=(logger,))

    for t in (t_alice, t_bob):
        t.start()

    for t in (t_alice, t_bob):
        t.join()

if __name__ == "__main__":
    test()
