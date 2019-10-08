#! /usr/bin/env python3

# standard libraries
import abc
import dataclasses
import enum
import inspect
import os.path
import queue
import random
import re
import threading
import time
import typing

# user-defined modules
import color
import helper


class LoggingLevels(enum.IntEnum):
    TRACE = 10
    DEBUG = 20
    INFO  = 30
    WARN  = 40
    ERROR = 50
    FATAL = 60


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


@dataclasses.dataclass
class WriterConfig:
    buffered:           bool = None
    monochrome:         bool = None
    show_clock_time:    bool = True
    show_elapsed_time:  bool = True
    show_log_level:     bool = True
    show_thread:        bool = True
    show_trace:         bool = True
    threshold: LoggingLevels = LoggingLevels.DEBUG


class Writer(abc.ABC):
    def __init__(self, config: WriterConfig = None):
        self.config = config if config else WriterConfig()
        self.buffered = config.buffered
        self.monochrome = config.monochrome
        self.show_clock_time = config.show_clock_time
        self.show_elapsed_time = config.show_elapsed_time
        self.show_log_level = config.show_log_level
        self.show_thread = config.show_thread
        self.show_trace = config.show_trace
        self.start_time = time.time()
        self.threshold = config.threshold

        self._color_regex = re.compile(r"\033\[[0-9]+(;[0-9]+)?m")
        if self.buffered:
            self._message_center = MessageCenter(self)
            self._rlock = threading.RLock()

    def prepend(self, msg, level=None):
        def stylize(prefix, level):
            if level <= LoggingLevels.DEBUG:
                prefix =color.faint(prefix)
            elif level == LoggingLevels.WARN:
                prefix = color.yellow(prefix)
            elif level == LoggingLevels.ERROR:
                prefix = color.red(prefix)
            elif level == LoggingLevels.FATAL:
                prefix = color.bold(color.red(prefix))
            return prefix
        if self.monochrome:
            msg = self._color_regex.sub("", msg)
        prefix = str()
        if self.show_clock_time:
            prefix += "{} ".format(helper.get_current_time())
        if self.show_elapsed_time:
            prefix += "({:9.3f}s) ".format(time.time() - self.start_time)
        if self.show_trace:
            frame = inspect.stack()[5]
            prefix += "{:10.10s}:{:4.4s} {:18.18s} ".format(
                        os.path.basename(frame.filename),
                        str(frame.lineno),
                        helper.compress(frame.function) + "()")
        if self.show_thread:
            prefix += "[{:10}] ".format((threading.current_thread().name))
        if self.show_log_level:
            prefix += "{:5} ".format(level.name if self.show_log_level and level else "")
        if prefix:
            prefix = prefix if self.monochrome else stylize(prefix, level)
            if "\n" in msg:
                return "\n".join([prefix + "│ " + x for x in msg.splitlines()])
            else:
                return prefix + "│ " + msg
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

    def unbuffered_write(self, msg, level=None):
        self.raw_write(self.prepend(msg, level))

    def buffered_write(self, msg, level=None):
        self._message_center.store(self.prepend(msg, level))

    def flush(self):
        if self.buffered:
            with self._rlock:
                self._message_center.flush()


class ScreenWriter(Writer):
    def __init__(self, config: WriterConfig):
        super().__init__(config)

    def raw_write(self, msg):
        print(msg)


class FileWriter(Writer):
    def __init__(self, filename="test.log", config: WriterConfig = None):
        super().__init__(config)
        self.filename = filename

    def raw_write(self, msg):
        with open(self.filename, "a") as file:
            file.write(msg + "\n")


class Logger:
    def __init__(self, *writers: typing.Tuple[Writer]):
        self.writers = writers
        start_time = time.time()
        for w in self.writers:
            w.config.start_time = start_time

    def log(self, msg, level: typing.Union[str, LoggingLevels]):
        if isinstance(level, str):
            level = LoggingLevels[level]
        for w in self.writers:
            w.write(msg, level)

    def trace(self, msg):
        self.log(msg, level=LoggingLevels.TRACE)

    def debug(self, msg):
        self.log(msg, level=LoggingLevels.DEBUG)

    def info(self, msg):
        self.log(msg, level=LoggingLevels.INFO)

    def warn(self, msg):
        self.log(msg, level=LoggingLevels.WARN)

    def error(self, msg):
        self.log(msg, level=LoggingLevels.ERROR)

    def fatal(self, msg):
        self.log(msg, level=LoggingLevels.FATAL)

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
    logger.debug(color.green("Bob:\tHi, everyone."))
    _pause()
    logger.debug("Bob:\tI'm {}.".format(color.blue("Bob")))
    logger.error("Bob:\tNice to meet you")
    logger.flush()


def test():
    sconfig = WriterConfig(buffered=True, monochrome=False)
    fconfig = WriterConfig(buffered=False, monochrome=True)
    logger = Logger(ScreenWriter(config=sconfig), FileWriter(config=fconfig))

    t_alice = threading.Thread(target=_alice, args=(logger,))
    t_bob = threading.Thread(target=_bob, args=(logger,))

    for t in (t_alice, t_bob):
        t.start()

    for t in (t_alice, t_bob):
        t.join()

if __name__ == "__main__":
    test()
