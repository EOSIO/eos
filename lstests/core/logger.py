#! /usr/bin/env python3

# standard libraries
import abc
import codecs
import inspect
import os.path
import queue
import threading
import time
import typing
# user-defined modules
if __package__:
    from . import color
    from . import helper
else:
    import color
    import helper


VERTICAL_BAR = "│"


class LogLevel(int):
    _val_of = {
        "ALL":   0,
        "TRACE": 10,
        "DEBUG": 20,
        "INFO":  30,
        "WARN":  40,
        "ERROR": 50,
        "FATAL": 60,
        "FLAG":  90,
        "OFF":   100
    }

    _name_of = {
        0:   "ALL",
        10:  "TRACE",
        20:  "DEBUG",
        30:  "INFO",
        40:  "WARN",
        50:  "ERROR",
        60:  "FATAL",
        90:  "FLAG",
        100: "OFF"
    }

    def __new__(cls, x):
        if isinstance(x, int):
            return int.__new__(cls, x)
        if isinstance(x, str):
            return int.__new__(cls, LogLevel._val_of[x.upper()])

    def __str__(self):
        try:
            return LogLevel._name_of[self]
        except KeyError:
            return int.__str__(self)


class _MessageCenter:
    def __init__(self, writer: typing.TypeVar("_Writer")):
        self.center = dict()
        self.writer = writer

    def store(self, msg):
        thread_id = threading.get_ident()
        self.center.setdefault(thread_id, queue.Queue())
        self.center[thread_id].put(msg)

    def flush(self):
        thread_id = threading.get_ident()
        self.store("")
        while True:
            item = self.center[thread_id].get()
            if item == "":
                return
            self.writer._raw_write(item)


class _Writer(abc.ABC):
    def __init__(self,
                 threshold: typing.Union[int, str, LogLevel],
                 monochrome: bool,
                 buffered: bool,
                 show_clock_time: bool,
                 show_elapsed_time: bool,
                 show_filename: bool,
                 show_lineno: bool,
                 show_function: bool,
                 show_thread: bool,
                 show_log_level: bool):
        self.threshold = LogLevel(threshold)
        self.monochrome = monochrome
        self.buffered = buffered
        self.show_clock_time = show_clock_time
        self.show_elapsed_time = show_elapsed_time
        self.show_filename = show_filename
        self.show_lineno = show_lineno
        self.show_function = show_function
        self.show_thread = show_thread
        self.show_log_level = show_log_level
        if self.buffered:
            self._message_center = _MessageCenter(self)
            self._rlock = threading.RLock()

    def write(self, msg, level: LogLevel):
        if level >= self.threshold:
            if self.buffered:
                self._buffered_write(msg, level)
            else:
                self._unbuffered_write(msg, level)

    def flush(self):
        if self.buffered:
            with self._rlock:
                self._message_center.flush()

    @staticmethod
    def _colorize(txt, level):
        if level <= LogLevel("debug"):
            return color.faint(txt)
        if level == LogLevel("warn"):
            return color.yellow(txt)
        if level == LogLevel("error"):
            return color.red(txt)
        if level == LogLevel("fatal"):
            return color.bold(color.red(txt))
        if level == LogLevel("flag"):
            return color.bold(txt)
        return txt

    def _prefix(self, msg: str, level: LogLevel):
        if self.monochrome:
            msg = color.REGEX.sub("", msg)
        pre = ""
        if self.show_clock_time:
            pre += f"{helper.get_time()} "
        if self.show_elapsed_time:
            pre += f"({time.time() - self._start_time:8.3f}s) "
        try:
            frame = inspect.stack()[5]
        except IndexError:
            frame = inspect.stack()[-1]
        if self.show_filename:
            filename = os.path.basename(frame.filename)
            pre += f"{helper.squeeze(filename, maxlen=10):10.10}:"
        if self.show_lineno:
            pre += f"{frame.lineno:<4} "
        if self.show_function:
            pre += f"{helper.squeeze(frame.function, maxlen=15)+'()':17.17} "
        if self.show_thread:
            pre += f"[{threading.current_thread().name:10.10}] "
        if self.show_log_level:
            pre += f"{str(level):5.5} "
        if pre:
            if not self.monochrome:
                pre = self._colorize(pre, level)
            return "\n".join([pre + VERTICAL_BAR + " " + x for x in msg.splitlines()])
        else:
            return msg

    @abc.abstractmethod
    def _raw_write(self, msg):
        pass

    def _unbuffered_write(self, msg, level):
        self._raw_write(self._prefix(msg, level))

    def _buffered_write(self, msg, level):
        self._message_center.store(self._prefix(msg, level))


class ScreenWriter(_Writer):
    def __init__(self,
                 threshold: typing.Union[int, str, LogLevel],
                 monochrome: bool = False,
                 buffered: bool = True,
                 show_clock_time: bool = True,
                 show_elapsed_time: bool = True,
                 show_filename: bool = True,
                 show_lineno: bool = True,
                 show_function: bool = True,
                 show_thread: bool = True,
                 show_log_level: bool = True):
        super().__init__(threshold,
                         monochrome,
                         buffered,
                         show_clock_time,
                         show_elapsed_time,
                         show_filename,
                         show_lineno,
                         show_function,
                         show_thread,
                         show_log_level)

    def _raw_write(self, msg):
        print(msg, flush=True)


class FileWriter(_Writer):
    def __init__(self,
                 filename,
                 threshold: typing.Union[int, str, LogLevel],
                 monochrome: bool = False,
                 buffered: bool = True,
                 show_clock_time: bool = True,
                 show_elapsed_time: bool = True,
                 show_filename: bool = True,
                 show_lineno: bool = True,
                 show_function: bool = True,
                 show_thread: bool = True,
                 show_log_level: bool = True):
        super().__init__(threshold,
                         monochrome,
                         buffered,
                         show_clock_time,
                         show_elapsed_time,
                         show_filename,
                         show_lineno,
                         show_function,
                         show_thread,
                         show_log_level)
        self.filename = filename

    def _raw_write(self, msg):
        with codecs.open(self.filename, "a", "utf-8") as f:
            print(msg, file=f, flush=True)


class Logger:
    def __init__(self, *writers: typing.Tuple[_Writer]):
        self.writers = writers
        now = time.time()
        for w in self.writers:
            w._start_time = now

    def __enter__(self):
        return self

    def __exit__(self, except_type, except_value, except_traceback):
        self.flush()

    def log(self, msg, level: typing.Union[int, str, LogLevel], buffer=False):
        level = LogLevel(level)
        for w in self.writers:
            w.write(str(msg), level)
        if not buffer:
            self.flush()

    def trace(self, msg, buffer=False):
        self.log(msg, level="trace", buffer=buffer)

    def debug(self, msg, buffer=False):
        self.log(msg, level="debug", buffer=buffer)

    def info(self, msg, buffer=False):
        self.log(msg, level="info", buffer=buffer)

    def warn(self, msg, buffer=False):
        self.log(msg, level="warn", buffer=buffer)

    def error(self, msg, buffer=False):
        self.log(msg, level="error", buffer=buffer)

    def fatal(self, msg, buffer=False):
        self.log(msg, level="fatal", buffer=buffer)

    def flag(self, msg, buffer=False):
        self.log(msg, level="flag", buffer=buffer)

    def flush(self):
        for w in self.writers:
            w.flush()


# ----------------- test ------------------------------------------------------


def _alice(logger):
    logger.info(f"Alice:\tHello! My name is {color.green('Alice')}.")
    _pause()
    logger.debug("Alice:\tI am sending a line at DEBUG level.")
    _pause()
    logger.warn(f"Alice:\tA {color.yellow('WARNING')} message:", buffer=True)
    _pause()
    logger.warn("Alice:\t1. When anyone tries to say anything,", buffer=True)
    _pause()
    logger.warn("Alice:\t2. he/she shouldn't get interrupted", buffer=True)
    _pause()
    logger.warn("Alice:\t3. before he/she finishes.")


def _bob(logger):
    logger.info(f"Bob:\tHi, everyone! I'm {color.blue('Bob')}.")
    _pause()
    logger.log(f"Bob:\tI am logging a line with log level 38.", level=38)
    _pause()
    logger.debug("""Bob:\tThis is a
            multiline
            statement.""")
    _pause()
    logger.info(f"Bob:\tAn {color.cyan('IMPORTANT')} notice:", buffer=True)
    _pause()
    logger.info("Bob:\tYou should see a line in the log file", buffer=True)
    _pause()
    logger.info("Bob:\t...but not printed out on the screen.", buffer=True)
    _pause()
    logger.info("Bob:\tIf not, you've got an error.", buffer=True)
    logger.flush()
    _pause()
    logger.trace(f"Bob:\t{color.bold('***TRACE: ONLY VISIBLE IN FILE***')}")


def _charlie(logger):
    logger.error("Charlie:\tI am buffering my words. Then I am", buffer=True)
    logger.error("Charlie:\tgoing to trigger an error. After the", buffer=True)
    logger.error("Charlie:\terror occurs, all the buffered words", buffer=True)
    logger.error("Charlie:\tshould get flushed.", buffer=True)
    logger.fatal(f"Charlie:\t-- A {color.red('FATAL')} action --", buffer=True)
    raise RuntimeError


def _pause(sleep=None):
    time.sleep(random.randint(0, 2) if sleep is None else sleep)


def _main():
    filename = "logger-trace.log"
    with open(filename, "w") as file:
        pass
    logger = Logger(ScreenWriter(threshold="debug"),
                    FileWriter(filename=filename, threshold="trace"))
    alice_thread = threading.Thread(target=_alice, args=(logger,))
    bob_thread = threading.Thread(target=_bob, args=(logger,))
    for t in (alice_thread, bob_thread):
        t.start()
    for t in (alice_thread, bob_thread):
        t.join()
    with logger as logger:
        try:
            _charlie(logger)
        except RuntimeError:
            print("---------------- A RuntimeError Occurred -----------------")


if __name__ == "__main__":
    import random
    _main()
