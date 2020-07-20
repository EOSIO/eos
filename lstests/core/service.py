#! /usr/bin/env python3

"""core.service - Core functions to allow tests to run on launcher service

This module provides a Service class and a Cluster class that work together to
allow Python test scripts to run on launcher service. The Service class
connects to the launcher service. The Cluster class then launches a cluster of
nodes.
"""

# standard libraries
import argparse
import base64
import collections
import errno
import functools
import math
import os
import platform
import socket
import string
import threading
import time
import typing
# user-defined modules
if __package__:
    from .logger import LogLevel, Logger, ScreenWriter, FileWriter
    from .connection import Connection
    from . import color
    from . import helper
else:
    from logger import LogLevel, Logger, ScreenWriter, FileWriter
    from connection import Connection
    import color
    import helper

# constants
PROGRAM = "launcher-service"
PROGRAM_LOG = "launcher-service.log"
PRODUCER_KEY = "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
PREACTIVATE_FEATURE = "0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"
# service-related defaults
DEFAULT_ADDR = "127.0.0.1"
DEFAULT_PORT = 1234
DEFAULT_WDIR = "."
DEFAULT_FILE = os.path.join(".", "programs", PROGRAM, PROGRAM)
DEFAULT_GENE = os.path.join(".", "genesis.json")
DEFAULT_START = False
DEFAULT_KILL = False
DEFAULT_LAUNCHER_MAXIMUM_IDLE_TIME = 10
DEFAULT_EXTRA_SERVICE_ARGS = ""
# cluster-related defaults
DEFAULT_CDIR = "../unittests/contracts/old_versions/v1.6.0-rc3"
DEFAULT_CLUSTER_ID = 0
DEFAULT_NODE_COUNT = 4
DEFAULT_PNODE_COUNT = 4
DEFAULT_PRODUCER_COUNT = 4
DEFAULT_UNSTARTED_COUNT = 0
DEFAULT_TOPOLOGY = "mesh"
DEFAULT_CENTER_NODE_ID = None
DEFAULT_EXTRA_ARGS = ""
DEFAULT_EXTRA_CONFIGS = []
DEFAULT_SPECIAL_LOG_LEVELS = []
DEFAULT_DONT_NEWACCO = False
DEFAULT_DONT_SETPROD = False
DEFAULT_HTTP_RETRY = 100
DEFAULT_HTTP_SLEEP = 0.25
DEFAULT_VERIFY_ASYNC = False
DEFAULT_VERIFY_RETRY = 100
DEFAULT_VERIFY_SLEEP = 0.25
DEFAULT_SYNC_RETRY = 100
DEFAULT_SYNC_SLEEP = 1
# logger-related defaults
DEFAULT_MONOCHROME = False
DEFAULT_BUFFERED = True
DEFAULT_DONT_RENAME = False
DEFAULT_SHOW_CLOCK_TIME = True
DEFAULT_SHOW_ELAPSED_TIME = True
DEFAULT_SHOW_FILENAME = True
DEFAULT_SHOW_LINENO = True
DEFAULT_SHOW_FUNCTION = True
DEFAULT_SHOW_THREAD = True
DEFAULT_SHOW_LOG_LEVEL = True
DEFAULT_HIDE_ALL = False
# service-related help text
HELP_HELP = "Show this message and exit"
HELP_ADDR = "IP address of launcher service"
HELP_PORT = "Listening port of launcher service"
HELP_WDIR = "Working directory"
HELP_FILE = "Path to executable file of launcher service"
HELP_GENE = "Path to genesis file"
HELP_START = "Always start a new launcher service"
HELP_KILL = "Kill existing launcher services (if any)"
HELP_LAUNCHER_MAXIMUM_IDLE_TIME = "Maximum time to allow a newly launched launcher-service to remain idle before shutting down"
HELP_EXTRA_SERVICE_ARGS = "Extra arguments to pass to launcher service"
# cluster-related help text
HELP_CDIR = "Smart contracts directory"
HELP_CLUSTER_ID = "Cluster ID to launch with"
HELP_NODE_COUNT = "Number of nodes"
HELP_PNODE_COUNT = "Number of nodes with producers"
HELP_PRODUCER_COUNT = "Number of producers"
HELP_UNSTARTED_COUNT = "Number of unstarted nodes"
HELP_TOPOLOGY = "Cluster topology to launch with"
HELP_CENTER_NODE_ID = "Center node ID (for bridge or star topology)"
HELP_EXTRA_ARGS = "Extra arguments to pass to nodeos"
HELP_EXTRA_CONFIGS = "Extra configs to pass to nodeos"
HELP_DONT_NEWACCO = "Do not create accounts after launch"
HELP_DONT_SETPROD = "Do not set producers after launch"
HELP_HTTP_RETRY = "HTTP connection: max num of retries"
HELP_HTTP_SLEEP = "HTTP connection: sleep time between retries"
HELP_VERIFY_ASYNC = "Verify transaction: verify asynchronously"
HELP_VERIFY_RETRY = "Verify transaction: max num of retries"
HELP_VERIFY_SLEEP = "Verify transaction: sleep time between retries"
HELP_SYNC_RETRY = "Check sync: max num of retries"
HELP_SYNC_SLEEP = "Check sync: sleep time between retries"
# logger-related help text
HELP_LOG_LEVEL = "Stdout logging level (numeric)"
HELP_LOG_ALL = "Set stdout logging level to ALL (0)"
HELP_TRACE = "Set stdout logging level to TRACE (10)"
HELP_DEBUG = "Set stdout logging level to DEBUG (20)"
HELP_INFO = "Set stdout logging level to INFO (30)"
HELP_WARN = "Set stdout logging level to WARN (40)"
HELP_ERROR = "Set stdout logging level to ERROR (50)"
HELP_FATAL = "Set stdout logging level to FATAL (60)"
HELP_FLAG = "Set stdout logging level to FLAG (90)"
HELP_LOG_OFF = "Set stdout logging level to OFF (100)"
HELP_MONOCHROME = "Do not print in colors for stdout logging"
HELP_DONT_BUFFER = "Do not buffer for stdout logging"
HELP_DONT_RENAME = "Do not rename log files by cluster ID"
HELP_HIDE_CLOCK_TIME = "Hide clock time in stdout logging"
HELP_HIDE_ELAPSED_TIME = "Hide elapsed time in stdout logging"
HELP_HIDE_FILENAME = "Hide filename in stdout logging"
HELP_HIDE_LINENO = "Hide line number in stdout logging"
HELP_HIDE_FUNCTION = "Hide function name in stdout logging"
HELP_HIDE_THREAD = "Hide thread name in stdout logging"
HELP_HIDE_LOG_LEVEL = "Hide log level in stdout logging"
HELP_HIDE_ALL = "Hide all the above in stdout logging"


class LauncherServiceError(RuntimeError):
    """Launcher service-related runtime error.

    For example, a LauncherServiceError may be raised when a Service object
    failed to connect to launcher service in initialization.
    """
    def __init__(self, message):
        super().__init__(message)


class BlockchainError(RuntimeError):
    """Blockchain-related runtime error.

    For example, a BlockchainError may be raised when nodes still failed to get
    ready after many retries."""
    def __init__(self, message):
        super().__init__(message)


class SyncError(BlockchainError):
    """Blockchain error when nodes out of sync."""
    def __init__(self, message):
        super().__init__(message)


def bassert(condition, message=None):
    """A helper function to raise BlockchainError.

    A BlockchainError with message will be raised when condition is not met."""
    if not condition:
        raise BlockchainError(message=message)


class ExceptionThread(threading.Thread):
    """A thread that reports exceptions to a public channel.

    By checking the channel, which could be a list or a dict, it is possible to
    tell which threads have met exceptions.
    """
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


class CommandLineArguments:
    """Command-line arguments to pass to Service and Cluster.

    Command-line arguments have the highest priority, which override in-script
    arguments, which override default values.
    """
    def __init__(self):
        cla = self.parse()
        # service-related options
        self.addr = cla.addr
        self.port = cla.port
        self.wdir = cla.wdir
        self.file = cla.file
        self.gene = cla.gene
        self.start = cla.start
        self.kill = cla.kill
        self.launcher_maximum_idle_time = cla.launcher_maximum_idle_time
        self.extra_service_args = cla.extra_service_args
        # cluster-related options
        self.cdir = cla.cdir
        self.cluster_id = cla.cluster_id
        self.node_count = cla.node_count
        self.pnode_count = cla.pnode_count
        self.producer_count = cla.producer_count
        self.unstarted_count = cla.unstarted_count
        self.topology = cla.topology
        self.center_node_id = cla.center_node_id
        self.extra_args = cla.extra_args
        self.extra_configs = cla.extra_configs
        if self.extra_configs:
            self.extra_configs = self.extra_configs.split(" ")

        self.dont_newacco = cla.dont_newacco
        self.dont_setprod = cla.dont_setprod
        self.http_retry = cla.http_retry
        self.http_sleep = cla.http_sleep
        self.verify_async = cla.verify_async
        self.verify_retry = cla.verify_retry
        self.verify_sleep = cla.verify_sleep
        self.sync_retry = cla.sync_retry
        self.sync_sleep = cla.sync_sleep
        # logger-related options
        self.threshold = cla.threshold
        self.monochrome = cla.monochrome
        self.buffered = cla.buffered
        self.dont_rename = cla.dont_rename
        self.show_clock_time = cla.show_clock_time
        self.show_elapsed_time = cla.show_elapsed_time
        self.show_filename = cla.show_filename
        self.show_lineno = cla.show_lineno
        self.show_function = cla.show_function
        self.show_thread = cla.show_thread
        self.show_log_level = cla.show_log_level
        self.hide_all = cla.hide_all

    @staticmethod
    def parse():
        """Parse for command-line arguments for Logger, Service, Cluster.

        Appending "-h/--help" to any test script will show help information."""
        desc = color.decorate("Launcher Service-based EOSIO Testing Framework", style="underline", fcolor="green")
        left = 5
        form = lambda text, value=None: "{} ({})".format(helper.pad(text, left=left, total=55), value)
        parser = argparse.ArgumentParser(description=desc, add_help=False,
                 formatter_class=lambda prog: argparse.RawTextHelpFormatter(prog, max_help_position=50))
        parser.add_argument("-h", "--help", action="help", help=" " * left + HELP_HELP)
        # service-related options
        parser.add_argument("-a", "--addr", type=str, metavar="IP", help=form(HELP_ADDR, DEFAULT_ADDR))
        parser.add_argument("-o", "--port", type=int, help=form(HELP_PORT, DEFAULT_PORT))
        parser.add_argument("-w", "--wdir", type=str, metavar="PATH", help=form(HELP_WDIR, DEFAULT_WDIR))
        parser.add_argument("-f", "--file", type=str, metavar="PATH", help=form(HELP_FILE, DEFAULT_FILE))
        parser.add_argument("-g", "--gene", type=str, metavar="PATH", help=form(HELP_GENE, DEFAULT_GENE))
        parser.add_argument("-s", "--start", action="store_true", default=None, help=form(HELP_START, DEFAULT_START))
        parser.add_argument("-k", "--kill", action="store_true", default=None, help=form(HELP_KILL, DEFAULT_KILL))
        parser.add_argument("--launcher-maximum-idle-time", type=int, default=None, help=form(HELP_LAUNCHER_MAXIMUM_IDLE_TIME, DEFAULT_LAUNCHER_MAXIMUM_IDLE_TIME))
        parser.add_argument("--extra-service-args", type=str, metavar="ARGS",
                            help=form(HELP_EXTRA_SERVICE_ARGS, DEFAULT_EXTRA_SERVICE_ARGS))
        # cluster-related options
        parser.add_argument("-c", "--cdir", metavar="PATH",
                            help=form(HELP_CDIR, DEFAULT_CDIR))
        parser.add_argument("-i", "--cluster-id", type=int, metavar="ID",
                            help=form(HELP_CLUSTER_ID, DEFAULT_CLUSTER_ID))
        parser.add_argument("-n", "--node-count", type=int, metavar="NUM",
                            help=form(HELP_NODE_COUNT, DEFAULT_NODE_COUNT))
        parser.add_argument("-p", "--pnode-count", type=int, metavar="NUM",
                            help=form(HELP_PNODE_COUNT, DEFAULT_PNODE_COUNT))
        parser.add_argument("-q", "--producer-count", type=int, metavar="NUM",
                            help=form(HELP_PRODUCER_COUNT, DEFAULT_PRODUCER_COUNT))
        parser.add_argument("-u", "--unstarted-count", type=int, metavar="NUM",
                            help=form(HELP_UNSTARTED_COUNT, DEFAULT_UNSTARTED_COUNT))
        parser.add_argument("-t", "--topology", type=str, metavar="SHAPE", help=form(HELP_TOPOLOGY, DEFAULT_TOPOLOGY),
                            choices={"mesh", "bridge", "line", "ring", "star", "tree"})
        parser.add_argument("-r", "--center-node-id", type=int, metavar="ID",
                            help=form(HELP_CENTER_NODE_ID, DEFAULT_CENTER_NODE_ID))
        parser.add_argument("--extra-args", type=str, metavar="ARGS",
                            help=form(HELP_EXTRA_ARGS, DEFAULT_EXTRA_ARGS))
        parser.add_argument("--extra-configs", type=str, metavar="CONFIGS",
                            help=form(HELP_EXTRA_CONFIGS, DEFAULT_EXTRA_CONFIGS))
        parser.add_argument("--dont-newacco", action="store_true", default=None,
                            help=form(HELP_DONT_NEWACCO, DEFAULT_DONT_NEWACCO))
        parser.add_argument("--dont-setprod", action="store_true", default=None,
                            help=form(HELP_DONT_SETPROD, DEFAULT_DONT_SETPROD))
        parser.add_argument("--http-retry", type=int, metavar="NUM",
                            help=form(HELP_HTTP_RETRY, DEFAULT_HTTP_RETRY))
        parser.add_argument("--http-sleep", type=float, metavar="TIME",
                            help=form(HELP_HTTP_SLEEP, DEFAULT_HTTP_SLEEP))
        parser.add_argument("-v", "--verify-async", action="store_true", default=None,
                            help=form(HELP_VERIFY_ASYNC, DEFAULT_VERIFY_ASYNC))
        parser.add_argument("--verify-retry", type=int, metavar="NUM",
                            help=form(HELP_VERIFY_RETRY, DEFAULT_VERIFY_RETRY))
        parser.add_argument("--verify-sleep", type=float, metavar="TIME",
                            help=form(HELP_VERIFY_SLEEP, DEFAULT_VERIFY_SLEEP))
        parser.add_argument("--sync-retry", type=int, metavar="NUM",
                            help=form(HELP_SYNC_RETRY, DEFAULT_SYNC_RETRY))
        parser.add_argument("--sync-sleep", type=float, metavar="TIME",
                            help=form(HELP_SYNC_SLEEP, DEFAULT_SYNC_SLEEP))
        # logger-related options
        threshold = parser.add_mutually_exclusive_group()
        threshold.add_argument("-l", "--log-level", dest="threshold", type=int, metavar="LEVEL", action="store",
                               help=form(HELP_LOG_LEVEL))
        threshold.add_argument("--all", dest="threshold", action="store_const", const="all", help=form(HELP_LOG_ALL))
        threshold.add_argument("--trace", dest="threshold", action="store_const", const="trace", help=form(HELP_TRACE))
        threshold.add_argument("--debug", dest="threshold", action="store_const", const="debug", help=form(HELP_DEBUG))
        threshold.add_argument("--info", dest="threshold", action="store_const", const="info", help=form(HELP_INFO))
        threshold.add_argument("--warn", dest="threshold", action="store_const", const="warn", help=form(HELP_WARN))
        threshold.add_argument("--error", dest="threshold", action="store_const", const="error", help=form(HELP_ERROR))
        threshold.add_argument("--fatal", dest="threshold", action="store_const", const="fatal", help=form(HELP_FATAL))
        threshold.add_argument("--flag", dest="threshold", action="store_const", const="flag", help=form(HELP_FLAG))
        threshold.add_argument("--off", dest="threshold", action="store_const", const="off", help=form(HELP_LOG_OFF))
        parser.add_argument("-m", "--monochrome", action="store_true", default=None,
                            help=form(HELP_MONOCHROME, DEFAULT_MONOCHROME))
        parser.add_argument("--dont-buffer", dest="buffered", action="store_false", default=None,
                            help=form(HELP_DONT_BUFFER, not DEFAULT_BUFFERED))
        parser.add_argument("--dont-rename", action="store_true", default=None,
                            help=form(HELP_DONT_RENAME, DEFAULT_DONT_RENAME))
        parser.add_argument("--hide-clock-time", dest="show_clock_time", action="store_false", default=None,
                            help=form(HELP_HIDE_CLOCK_TIME, not DEFAULT_SHOW_CLOCK_TIME))
        parser.add_argument("--hide-elapsed-time", dest="show_elapsed_time", action="store_false",
                            default=None, help=form(HELP_HIDE_ELAPSED_TIME, not DEFAULT_SHOW_ELAPSED_TIME))
        parser.add_argument("--hide-filename", dest="show_filename", action="store_false", default=None,
                            help=form(HELP_HIDE_FILENAME, not DEFAULT_SHOW_FILENAME))
        parser.add_argument("--hide-lineno", dest="show_lineno", action="store_false", default=None,
                            help=form(HELP_HIDE_LINENO, not DEFAULT_SHOW_LINENO))
        parser.add_argument("--hide-function", dest="show_function", action="store_false", default=None,
                            help=form(HELP_HIDE_FUNCTION, not DEFAULT_SHOW_FUNCTION))
        parser.add_argument("--hide-thread", dest="show_thread", action="store_false", default=None,
                            help=form(HELP_HIDE_THREAD, not DEFAULT_SHOW_THREAD))
        parser.add_argument("--hide-log-level", dest="show_log_level", action="store_false", default=None,
                            help=form(HELP_HIDE_LOG_LEVEL, not DEFAULT_SHOW_LOG_LEVEL))
        parser.add_argument("--hide-all", action="store_true", default=False,
                            help=form(HELP_HIDE_ALL, DEFAULT_HIDE_ALL))
        return parser.parse_args()

# =============== BEGIN OF SERVICE CLASS ==============================================================================

class Service:
    """Establish and maintain connection with launcher service. Manage logging.

    A Service object must have a Logger object registered to it at
    initialization. It is possible that the Service object may *modify* the
    settings in the Logger object.

    --- Configuration --
    A Service object is configured at creation, with values from 3 sources:
    (1) defaults,
    (2) in-script arguments, and
    (3) command-line arguments,
    with the latter capable of overriding the former.

    --- Connect --------
    After configuration, the Service will change the working directory,
    register (and possibly *modify*) the Logger, and then connect to launcher
    service.  Currently, it is only possible to connect to a local launcher
    service. If there is already an existing launcher service running in the
    background, the Service will by default connect to it (without starting a
    new one). It is possible to override this default by requesting to always
    start a new launcher service and/or to kill all the existing launcher
    service(s).
    """
    def __init__(self, logger, addr=None, port=None, wdir=None, file=None, gene=None, start=None, kill=None,
                 launcher_maximum_idle_time=None, extra_args=None):
        """Create a Service object to connect to launcher service.

        Parameters
        ----------
        logger : Logger
            Logger object which controls logging behavior.
        addr : str
            IP address of launcher service.
            Currently, only local launcher service is supported. That is, only
            default value "127.0.0.1" is supported.
        port : int
            Listening port of launcher service.
            If there are multiple launcher services running in the background,
            they must have different listening ports.
            Default is 1234.
        wdir : str
            Working directory.
            Default is the current working directory.
        file : str
            Path to executable file of launcher service.
            Can be either absolute or relative to the working directory.
        gene : str
            Path to the genesis file.
            Can be either absolute or relative to the working directory.
        start : bool
            Always start a new launcher service.
            To start a new instance alongside the existing ones, make sure the
            listening ports are different. Otherwise, the new launcher service
            will issue an error and exit.
            Default is False (will not start a new one if there is an existing
            launcher service running in the background).
        kill : bool
            Kill existing launcher services (if any).
            Kill all the existing launcher services running in the background
            (and freshly start a new one).
            Default is False (will not kill existing launcher services; instead
            will connect to an existing launcher service).
        launcher_maximum_idle_time : int
            Time (in minutes) that is the maximum length of time the test will
            take between each call to the launcher-service.  It is used for
            the idle-shutdown passed to the launcher-service if it is launched.
        extra_args : str
            Extra arguments to pass to launcher service.
        """
        # read command-line arguments
        self.cla = CommandLineArguments()
        # configure service
        self.addr  = helper.override(DEFAULT_ADDR,  addr,  self.cla.addr)
        self.port  = helper.override(DEFAULT_PORT,  port,  self.cla.port)
        self.wdir  = helper.override(DEFAULT_WDIR,  wdir,  self.cla.wdir)
        self.file  = helper.override(DEFAULT_FILE,  file,  self.cla.file)
        self.gene  = helper.override(DEFAULT_GENE,  gene,  self.cla.gene)
        self.start = helper.override(DEFAULT_START, start, self.cla.start)
        self.kill  = helper.override(DEFAULT_KILL,  kill,  self.cla.kill)
        self.launcher_maximum_idle_time = helper.override(DEFAULT_LAUNCHER_MAXIMUM_IDLE_TIME, launcher_maximum_idle_time,  self.cla.launcher_maximum_idle_time)
        self.extra_args  = helper.override(DEFAULT_EXTRA_SERVICE_ARGS,  extra_args,  self.cla.extra_service_args)
        # change working dir
        if self.wdir != ".":
            os.chdir(self.wdir)
        # register logger
        self.register_logger(logger)
        # connect
        self.connect()

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        """Flush all buffered logging information in case Service crashes"""
        self.flush()

    def register_logger(self, logger):
        self.logger = logger
        for w in self.logger.writers:
            # override screen writer settings with command-line arguments
            if isinstance(w, ScreenWriter):
                self.threshold = w.threshold = LogLevel(helper.override(w.threshold, self.cla.threshold))
                self.buffered = w.buffered = helper.override(DEFAULT_BUFFERED, w.buffered, self.cla.buffered)
                self.monochrome = w.monochrome = helper.override(DEFAULT_MONOCHROME, w.monochrome, self.cla.monochrome)
                if self.cla.hide_all:
                      w.show_clock_time = w.show_elapsed_time = w.show_filename = w.show_lineno = w.show_function = \
                      w.show_thread = w.show_log_level = False
                else:
                    w.show_clock_time = helper.override(DEFAULT_SHOW_CLOCK_TIME, w.show_clock_time, self.cla.show_clock_time)
                    w.show_elapsed_time = helper.override(DEFAULT_SHOW_ELAPSED_TIME, w.show_elapsed_time, self.cla.show_elapsed_time)
                    w.show_filename = helper.override(DEFAULT_SHOW_FILENAME, w.show_filename, self.cla.show_filename)
                    w.show_lineno = helper.override(DEFAULT_SHOW_LINENO, w.show_lineno, self.cla.show_lineno)
                    w.show_function = helper.override(DEFAULT_SHOW_FUNCTION, w.show_function, self.cla.show_function)
                    w.show_thread = helper.override(DEFAULT_SHOW_THREAD, w.show_thread, self.cla.show_thread)
                    w.show_log_level = helper.override(DEFAULT_SHOW_LOG_LEVEL, w.show_log_level, self.cla.show_log_level)
            # empty log files
            elif isinstance(w, FileWriter):
                if not self.cla.dont_rename and self.cla.cluster_id is not None:
                    bas, ext = os.path.splitext(w.filename)
                    w.filename = f"{bas}-{self.cla.cluster_id}{ext}"
                with open(w.filename, "w"):
                    pass
        # register for shorter names
        self.log = self.logger.log
        self.trace = self.logger.trace
        self.debug = self.logger.debug
        self.info = self.logger.info
        self.warn = self.logger.warn
        self.error = self.logger.error
        self.fatal = self.logger.fatal
        self.flag = self.logger.flag
        self.flush = self.logger.flush

    def connect(self):
        self.print_begin("Connect to Service")
        self.print_working_dir()
        self.print_system_info()
        self.print_config()
        if self.addr == "127.0.0.1":
            self.connect_to_local_service()
        else:
            self.connect_to_remote_service()
        self.print_end("Connect to Service")

    def print_working_dir(self):
        self.print_header("working directory")
        self.debug("{:22}{}".format("Working Directory", os.getcwd()))

    def print_system_info(self):
        self.print_header("system info")
        self.debug("{:22}{}".format("UTC Time", time.strftime("%Y-%m-%d %H:%M:%S %Z", time.gmtime())))
        self.debug("{:22}{}".format("Local Time", time.strftime("%Y-%m-%d %H:%M:%S %Z", time.localtime())))
        self.debug("{:22}{}".format("Platform", platform.platform()))
        self.debug("{:22}{}".format("Python Version", platform.python_version()))

    def print_config(self):
        self.print_header("service configuration")
        self.print_config_helper("-a, --addr",  HELP_ADDR,  self.addr,  DEFAULT_ADDR)
        self.print_config_helper("-o, --port",  HELP_PORT,  self.port,  DEFAULT_PORT)
        self.print_config_helper("-w, --wdir",  HELP_WDIR,  self.wdir,  DEFAULT_WDIR)
        self.print_config_helper("-f, --file",  HELP_FILE,  self.file,  DEFAULT_FILE)
        self.print_config_helper("-g, --gene",  HELP_GENE,  self.gene,  DEFAULT_GENE)
        self.print_config_helper("-s, --start", HELP_START, self.start, DEFAULT_START)
        self.print_config_helper("-k, --kill",  HELP_KILL,  self.kill,  DEFAULT_KILL)
        self.print_config_helper("--launcher-maximum-idle-time", HELP_LAUNCHER_MAXIMUM_IDLE_TIME, self.launcher_maximum_idle_time, DEFAULT_LAUNCHER_MAXIMUM_IDLE_TIME)
        self.print_config_helper("--extra-service-args",  HELP_EXTRA_SERVICE_ARGS,  self.extra_args,  DEFAULT_EXTRA_SERVICE_ARGS)
        self.print_logger_config()

    def print_logger_config(self):
        self.print_header("logger configuration")
        name = str(self.threshold)
        ival = str(int(self.threshold))
        text = "{} ({})".format(name, ival) if name != ival else "{}".format(name)
        self.print_config_helper("-l, --log-level", HELP_LOG_LEVEL, text)
        self.print_config_helper("-m, --monochrome", HELP_MONOCHROME, self.monochrome, DEFAULT_MONOCHROME)
        self.print_config_helper("--dont-buffer", HELP_DONT_BUFFER, not self.buffered, not DEFAULT_BUFFERED)
        self.print_config_helper("--dont-rename", HELP_DONT_RENAME, self.cla.dont_rename, DEFAULT_DONT_RENAME)

    def connect_to_local_service(self):
        self.print_header("connect to local service")
        service_list = self.get_local_services()
        if self.kill:
            self.kill_local_services(service_list)
            service_list.clear()
        if service_list and not self.start:
            for x in service_list:
                if x.port == self.port:
                    service = x
                    break
            else:
                service = service_list[0]
            # only choose an existing launcher if its command line was parsable (so we can determine the port to connect to)
            if service.port != 0:
                self.connect_to_existing_local_service(service)
                return

        self.start_local_service()

    # TO DO IN FUTURE
    def connect_to_remote_service(self):
        self.print_header("connect to remote service")
        self.warn("WARNING: Executable file setting (file={}) ignored.".format(color.yellow(helper.squeeze(self.file, maxlen=30, tail=10))))
        if self.start:
            self.warn("WARNING: Setting to always start a new launcher service (start={}) ignored.".format(color.yellow(self.start)))
        if self.kill:
            self.warn("WARNING: Setting to kill existing launcher services (kill={}) ignored.".format(color.yellow(self.kill)))
        msg = "Connecting to a remote service is a feature in future."
        self.fatal("FATAL: {}".format(msg))
        raise LauncherServiceError(msg)

    def print_begin(self, text, level="info", buffer=False):
        self.log((f">>> [{text}] ".ljust(40, "-") + " BEGIN ").ljust(100, "-"), level=level, buffer=buffer)

    def print_end(self, text, level="info", buffer=False):
        self.log((f">>> [{text}] ".ljust(40, "-") + " END ").ljust(100, "-"), level=level, buffer=buffer)

    def print_header(self, text, level: typing.Union[int, str, LogLevel]="debug", sep=" ", buffer=False):
        level = LogLevel(level)
        if level >= LogLevel("info"):
            colorize = color.bold
            fillchar = "="
        elif level >= LogLevel("debug"):
            colorize = color.black_on_cyan
            fillchar = "-"
        else:
            colorize = color.vanilla
            fillchar = "⎯"
        self.log(helper.pad(colorize(text), total=100, left=20, char=fillchar, sep=sep, textlen=len(text)), level=level, buffer=buffer)

    def print_config_helper(self, label, help, value, default_value=None):
        different = value is not None and value != default_value
        squeezed = helper.squeeze(str(value if value is not None else default_value), maxlen=30, tail=10)
        highlighted = color.blue(squeezed) if different else squeezed
        self.debug("{:31}{:48}{}".format(color.yellow(label), help, highlighted))

    def get_local_services(self) -> typing.List[typing.TypeVar("ServiceInfo")]:
        """Returns a list of 0, 1, or more process IDs"""
        service_list = helper.get_service_list_by_cmd(PROGRAM)
        if len(service_list) == 0:
            self.debug(color.yellow("No launcher service is currently running."))
        elif len(service_list) == 1:
            self.debug(color.green("Launcher service is currently running with process ID {} (port={}).".format(service_list[0].pid, service_list[0].port)))
        else:
            self.debug(color.green("Multiple launcher services are currently running."))
            self.debug(color.green("{:5}{:10}{}".format("#", "PID", "Listening Port")))
            for i, x in enumerate(service_list):
                self.debug(color.green("{:<5}{:<10}{}".format(i + 1, x.pid, x.port)))
        return service_list

    def kill_local_services(self, service_list: typing.List[typing.TypeVar("ServiceInfo")]):
        for x in service_list:
            self.debug(color.yellow("Killing existing launcher service with process ID {} (port={}).".format(x.pid, x.port)))
            helper.terminate(x.pid)

    def connect_to_existing_local_service(self, service: typing.TypeVar("ServiceInfo")):
        self.debug("No new launcher service will be started.")
        self.debug("Configuration of the launcher service to connect to:")
        self.debug("--- Listening port: [{}]".format(color.blue(service.port)))
        self.debug("--- Path to executable file: [{}]".format(color.blue(service.file)))
        if service.gene:
            self.debug("--- Path to genesis file: [{}]".format(color.blue(service.gene)))
        if self.port != service.port:
            self.warn("WARNING: Port setting (port={}) ignored.".format(color.yellow(self.port)))
            self.port = service.port
        if self.file != service.file:
            self.warn("WARNING: Executable file setting (file={}) ignored.".format(color.yellow(self.file)))
            self.file = service.file
        if service.gene and self.gene != service.gene:
            self.warn("WARNING: Genesis file setting (gene={}) ignored.".format(color.yellow(self.gene)))
            self.gene = service.gene
        if self.extra_args:
            self.warn("WARNING: Extra service arguments ({}) are ignored.".format(color.yellow(self.extra_args)))
        self.debug("To always start a new launcher service, pass {} or {}.".format(color.yellow("-s"), color.yellow("--start")))
        self.debug("To kill existing launcher services, pass {} or {}.".format(color.yellow("-k"), color.yellow("--kill")))
        self.debug(color.green("Connected to the launcher service with process ID {} (port={}).".format(service.pid, service.port)))

    def start_local_service(self):
        self.debug(color.green("Starting a new launcher service (port={}).".format(self.port)))
        with open(PROGRAM_LOG, "w") as f:
            pass
        if not self.are_ports_available(self.port):
            msg = f"ERROR: Launcher service cannot use port: {self.port}, it is already taken!"
            self.error(msg)
            raise LauncherServiceError(msg)
        self.debug(color.green(f"Launcher service port: {self.port} is availale."))
        generate_genesis_flag = ""
        if not os.path.isfile(self.gene):
            self.debug(color.green(f"Generating genesis-json file {self.gene}"))
            generate_genesis_flag = " --create-default-genesis"
        idle_timeout_flag = ""
        if self.launcher_maximum_idle_time is not None:
            self.debug(color.green(f"generating {self.gene}"))
            idle_timeout_flag = f"--idle-shutdown={self.launcher_maximum_idle_time}"
        cmdStr =  f"{self.file} --http-server-address=0.0.0.0:{self.port} --genesis-json={self.gene} --http-max-response-time-ms 990000 " + \
                  f"{generate_genesis_flag} {idle_timeout_flag} {self.extra_args} >{PROGRAM_LOG}  2>&1 &"
        self.debug(color.green("Running: {}".format(cmdStr)))
        os.system(cmdStr)
        time.sleep(1)
        with open(PROGRAM_LOG, "r") as f:
            msg = ""
            for line in f:
                if line.startswith("error"):
                    msg = line[line.find("]")+2:]
                    self.error("ERROR: {}".format(msg))
            if msg:
                raise LauncherServiceError(msg)
        service_list = self.get_local_services()
        for x in service_list:
            if x.port == self.port:
                self.debug(color.green("Connected to the launcher service with process ID {} (port={}).".format(x.pid, x.port)))
                return
        else:
            msg = "ERROR: Launcher service (port={}) is not properly started!".format(self.port)
            self.error(msg)
            raise LauncherServiceError(msg)

    def are_ports_available(self, ports):
        """Check if specified port (as int) or ports (as set) is/are available for listening on."""
        assert(ports)
        if isinstance(ports, int):
            ports={ports}
        assert(isinstance(ports, set))

        for port in ports:
            self.debug(color.green(f"Checking if port: {port} is available."))
            assert(isinstance(port, int))
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

            try:
                s.bind(("127.0.0.1", port))
            except socket.error as e:
                if e.errno == errno.EADDRINUSE:
                    self.warn(f"WARNING: Port {port} is already in use.")
                else:
                    # something else raised the socket.error exception
                    self.warn(f"WARNING: Unknown exception while trying to listen on port: {port}.")
                return False
            finally:
                s.close()

        return True

# =============== END OF SERVICE CLASS ================================================================================

# =============== BEGIN OF CLUSTER CLASS ==============================================================================

class Cluster:
    """A cluster of nodes running on launcher service.

    Major proxy for tests to communicate with launcher service. A Cluster
    object must have a Service object registered to it at initialization.

    --- Configuration --
    A Cluster object is configured at creation, with values from 3 sources:
    (1) defaults,
    (2) in-script arguments, and
    (3) command-line arguments,
    with the latter capable of overriding the former.

    --- Launch ---------
    After configuration, the node cluster will be launched with the help of
    "eosio.bios" contract.

    --- Test -----------
    After the node cluster is successfully launched, all the test actions can
    be performed on the cluster, with main API listed below.

    Main API
    --------
    - Start up and shut down
        - launch_cluster()
        - stop_cluster()
        - start_node()
        - stop_node()
        - cleanup()
        - stop_all_nodes()
    - Queries
        - get_cluster_running_state()
        - get_cluster_info()
        - get_info()
        - get_block()
        - get_account()
        - get_protocol_features()
        - get_log()
    - Transactions
        - schedule_protocol_feature_activations()
        - set_contract()
        - push_action()
    - Check and Verify
        - verify()
        - check_sync()
        - check_production_round()
    - Miscellaneous
        - send_raw()
        - pause_node_production()
        - resume_node_production()
        - get_greylist()
        - add_greylist_accounts()
        - remove_greylist_accounts()
        - get_net_plugin_connections()
    """
    def __init__(self,
                 service,
                 cdir=None,
                 cluster_id=None,
                 node_count=None,
                 pnode_count=None,
                 producer_count=None,
                 unstarted_count=None,
                 topology=None,
                 center_node_id=None,
                 extra_args: str=None,
                 extra_configs: typing.List[str]=None,
                 special_log_levels=None,
                 dont_newacco=None,
                 dont_setprod=None,
                 http_retry=None,
                 http_sleep=None,
                 verify_async=None,
                 verify_retry=None,
                 verify_sleep=None,
                 sync_retry=None,
                 sync_sleep=None):
        """Create a Cluster object to launch a cluster of nodes.

        The node cluster is the platform on which a test will be performed.

        Parameters
        ----------
        service : Service
            Launcher service object on which the node cluster will run
        cdir : str
            Smart contracts directory.
            Can be either absolute or relative to service's working directory.
        cluster_id : int
            Cluster ID to launch with.
            Tests with different cluster IDs can be run in parallel.
            Valid range is [0, 30). Default is 0.
        node_count : int
            Number of nodes in the cluster.
            Default is 4.
        pnode_count : int
            Number of nodes with at least one producer.
            Default is 4.
        producer_count : int
            Number of producers in the cluster.
            Default is 4.
        unstarted_count : int
            Number of unstarted nodes.
            Default is 0.
        topology : str
            Cluster topology to launch with.
            Valid choices are "mesh", "bridge", "line", "ring", "star", "tree".
            Default is "mesh".
        center_node_id : int
            Center node ID (for bridge or star topology).
            If topology is bridge, center node ID cannot be 0 or last one.
            No default value.
        extra_args : str
            Extra arguments to pass to nodeos via launcher service.
            e.g. "--delete-all-blocks"
            Default is "".
        extra_configs : list
            Extra configs to pass to nodeos via launcher service.
            e.g. ["plugin=SOME_EXTRA_PLUGIN"]
            Default is [].
        dont_newacco : bool
            Do not create accounts after launch.
            Default is False (will create producer accounts).
        dont_setprod : bool
            Do not set producers after launch.
            Default is False (will set producers).
        http_retry : int
            Max number of retries in HTTP connection.
            Default is 100.
        http_sleep : float
            Sleep time (in seconds) between HTTP connection retries.
            Default is 0.25.
        verify_async : bool
            Verify transactions asynchronously.
            Start a separate thread for transaction verification. Do not wait
            for a transaction to be verified before making next transaction.
            Default is False (will wait for verification result).
        verify_retry : int
            Max number of retries in transaction verification.
            Default is 100.
        verify_sleep : float
            Sleep time (in seconds) between verification retries.
            Default is 0.25.
        sync_retry : int
            Max number of retries in checking if nodes are in sync.
            Default is 100.
        sync_sleep : float
            Sleep time (in seconds) between check-sync retries.
            Default is 0.25.
        """
        # register service
        self.service = service
        self.cla = service.cla
        self.logger = service.logger
        self.log = service.log
        self.trace = service.trace
        self.debug = service.debug
        self.info = service.info
        self.warn = service.warn
        self.error = service.error
        self.fatal = service.fatal
        self.flag = service.flag
        self.flush = service.flush
        self.print_begin = service.print_begin
        self.print_end = service.print_end
        self.print_header = service.print_header
        self.print_config_helper = service.print_config_helper
        self.verify_threads = []
        # configure cluster
        self.cdir            = helper.override(DEFAULT_CDIR,            cdir,            self.cla.cdir)
        self.cluster_id      = helper.override(DEFAULT_CLUSTER_ID,      cluster_id,      self.cla.cluster_id)
        self.node_count      = helper.override(DEFAULT_NODE_COUNT,      node_count,      self.cla.node_count)
        self.pnode_count     = helper.override(DEFAULT_PNODE_COUNT,     pnode_count,     self.cla.pnode_count)
        self.producer_count  = helper.override(DEFAULT_PRODUCER_COUNT,  producer_count,  self.cla.producer_count)
        self.unstarted_count = helper.override(DEFAULT_UNSTARTED_COUNT, unstarted_count, self.cla.unstarted_count)
        self.topology        = helper.override(DEFAULT_TOPOLOGY,        topology,        self.cla.topology)
        self.center_node_id  = helper.override(DEFAULT_CENTER_NODE_ID,  center_node_id,  self.cla.center_node_id)
        self.extra_configs   = helper.override(DEFAULT_EXTRA_CONFIGS,   extra_configs,   self.cla.extra_configs)
        self.extra_args      = helper.override(DEFAULT_EXTRA_ARGS,      extra_args,      self.cla.extra_args)
        self.dont_newacco    = helper.override(DEFAULT_DONT_NEWACCO,    dont_newacco,    self.cla.dont_newacco)
        self.dont_setprod    = helper.override(DEFAULT_DONT_SETPROD,    dont_setprod,    self.cla.dont_setprod)
        self.http_retry      = helper.override(DEFAULT_HTTP_RETRY,      http_retry,      self.cla.http_retry)
        self.http_sleep      = helper.override(DEFAULT_HTTP_SLEEP,      http_sleep,      self.cla.http_sleep)
        self.verify_async    = helper.override(DEFAULT_VERIFY_ASYNC,    verify_async,    self.cla.verify_async)
        self.verify_retry    = helper.override(DEFAULT_VERIFY_RETRY,    verify_retry,    self.cla.verify_retry)
        self.verify_sleep    = helper.override(DEFAULT_VERIFY_SLEEP,    verify_sleep,    self.cla.verify_sleep)
        self.sync_retry      = helper.override(DEFAULT_SYNC_RETRY,      sync_retry,      self.cla.sync_retry)
        self.sync_sleep      = helper.override(DEFAULT_SYNC_SLEEP,      sync_sleep,      self.cla.sync_sleep)
        self.special_log_levels = helper.override(DEFAULT_SPECIAL_LOG_LEVELS, special_log_levels)


        # ensure these extra config parameters are always passed
        additional_extra_configs = [ "abi-serializer-max-time-ms", "http-max-response-time-ms" ]
        for extra_config in self.extra_configs:
            for additional in additional_extra_configs:
                if extra_config.startswith(f"{additional}="):
                    additional_extra_configs.remove(additional)
                    break

        for additional in additional_extra_configs:
            self.extra_configs.append(f"{additional}=10000000")

        # ensure that net_plugin_impl and producer_plugin always have an explicitly provided logging level
        new_loggers = ["net_plugin_impl","producer_plugin"]
        for logger in self.special_log_levels:
            if logger[0] in new_loggers:
                new_loggers.remove(logger[0])
                if len(new_loggers) < 0:
                    break
        for new_logger in new_loggers:
            self.special_log_levels.append( [new_logger, "debug"] )
        # check for logical errors in config
        self.check_config()
        # establish mappings between nodes and producers
        # Example
        # -------
        # Given 4 nodes, 2 producer nodes and 3 (non-eosio) producers, the
        # information and mappings would be:
        # self.node_count = 4
        # self.pnode_count = 2
        # self.producer_count = 3
        # self.nodes = [[0, {"producers": ["eosio", "defproducera", "defproducerb"]}],
        #               [1, {"producers": ["defproducerc"]}],
        #               [2, {"producers": [""]}],
        #               [3, {"producers": [""]}]]
        # self.producers = ["defproducera", "defproducerb", "defproducerc"]
        # self.producer_to_node = {"defproducera": 0, "defproducerb": 0, "defproducerc": 1}
        # self.node_to_producers = {0: ["defproducera", "defproducerb"], 1: ["defproducerc"]}
        self.nodes = []
        self.producers = []
        self.producer_to_node = {}
        self.node_to_producers = {}
        q, r = divmod(self.producer_count, self.pnode_count)
        for i in range(self.pnode_count):
            self.nodes += [[i]]
            prod = []
            for j in range(i * q + r if i else 0, (i + 1) * q + r):
                name = self.make_defproducer_name(j)
                prod.append(name)
                self.producer_to_node[name] = i
            self.nodes[i] += [{"producers": (prod if i else ["eosio"] + prod)}]
            self.producers += prod
            self.node_to_producers[i] = prod
        self.launch(dont_newacco=self.dont_newacco, dont_setprod=self.dont_setprod)

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
        """Flush all buffered logging information in case a Cluster crashes."""
        self.cleanup()
        self.flush()

    def check_config(self):
        bassert(0 <= self.cluster_id < 30, f"Invalid cluster_id ({self.cluster_id}). Valid range is [0, 30).")
        bassert(self.node_count >= self.pnode_count + self.unstarted_count,
                f"node_count ({self.node_count}) must be greater than "
                f"pnode_count ({self.pnode_count}) + unstarted_count ({self.unstarted_count}).")
        if self.topology in ("bridge", "star"):
            bassert(self.center_node_id is not None, f"center_node_id is not specified when topology is \"{self.topology}\".")
        if self.topology == "bridge":
            bassert(self.center_node_id not in (0, self.node_count-1),
                    f"center_node_id ({self.center_node_id}) cannot be 0 or {self.node_count-1} "
                    f"when topology is \"bridge\" and node_count is {self.node_count}.")

    def print_config(self):
        self.print_header("cluster configuration")
        self.print_config_helper("-c, --cdir",             HELP_CDIR,            self.cdir,            DEFAULT_CDIR)
        self.print_config_helper("-i, --cluster-id",       HELP_CLUSTER_ID,      self.cluster_id,      DEFAULT_CLUSTER_ID)
        self.print_config_helper("-n, --node-count",       HELP_NODE_COUNT,      self.node_count,      DEFAULT_NODE_COUNT)
        self.print_config_helper("-p, --pnode-count",      HELP_PNODE_COUNT,     self.pnode_count,     DEFAULT_PNODE_COUNT)
        self.print_config_helper("-q, --producer-count",   HELP_PRODUCER_COUNT,  self.producer_count,  DEFAULT_PRODUCER_COUNT)
        self.print_config_helper("-u, --unstarted-count",  HELP_UNSTARTED_COUNT, self.unstarted_count, DEFAULT_UNSTARTED_COUNT)
        self.print_config_helper("-t, --topology",         HELP_TOPOLOGY,        self.topology,        DEFAULT_TOPOLOGY)
        self.print_config_helper("-r, --center-node-id",   HELP_CENTER_NODE_ID,  self.center_node_id,  DEFAULT_CENTER_NODE_ID)
        self.print_config_helper("--extra-args",           HELP_EXTRA_ARGS,      self.extra_args,      DEFAULT_EXTRA_ARGS)
        self.print_config_helper("--extra-configs",        HELP_EXTRA_CONFIGS,   self.extra_configs,   DEFAULT_EXTRA_CONFIGS)
        self.print_config_helper("--dont-newacco",         HELP_DONT_NEWACCO,    self.dont_newacco,    DEFAULT_DONT_NEWACCO)
        self.print_config_helper("--dont-setprod",         HELP_DONT_SETPROD,    self.dont_setprod,    DEFAULT_DONT_SETPROD)
        self.print_config_helper("--http-retry",           HELP_HTTP_RETRY,      self.http_retry,      DEFAULT_HTTP_RETRY)
        self.print_config_helper("--http-sleep",           HELP_HTTP_SLEEP,      self.http_sleep,      DEFAULT_HTTP_SLEEP)
        self.print_config_helper("-v, --verify-async",     HELP_VERIFY_ASYNC,    self.verify_async,    DEFAULT_VERIFY_ASYNC)
        self.print_config_helper("--verify-retry",         HELP_VERIFY_RETRY,    self.verify_retry,    DEFAULT_VERIFY_RETRY)
        self.print_config_helper("--verify-sleep",         HELP_VERIFY_SLEEP,    self.verify_sleep,    DEFAULT_VERIFY_SLEEP)
        self.print_config_helper("--sync-retry",           HELP_SYNC_RETRY,      self.sync_retry,      DEFAULT_SYNC_RETRY)
        self.print_config_helper("--sync-sleep",           HELP_SYNC_SLEEP,      self.sync_sleep,      DEFAULT_SYNC_SLEEP)

    def launch(self, dont_newacco=False, dont_setprod=False):
        """Steps to take for a launch
        ---------------------------------------
        0. print config
        1. launch a cluster
        2. wait for all the nodes to get ready
        3. schedule protocol feature activations
        4. set eosio.bios contract --> for setprod, not required for newaccount
        5. create accounts using eosio.bios
        6. set producers
        7. check if nodes are in sync
        8. if verification is done asynchronously, make sure all transactions
           have been verified
        """
        self.print_begin("Launch a Cluster")
        self.print_config()
        self.launch_cluster()
        self.wait_nodes_ready()
        self.schedule_protocol_feature_activations()
        self.set_bios_contract()
        if not dont_newacco:
            self.bios_create_accounts_in_parallel(accounts=self.producers)
            if not dont_setprod:
                self.set_producers()
        self.check_sync()
        for t in self.verify_threads:
            t.join()
        self.print_end("Launch a Cluster")

# --------------- start-up and shut-down ------------------------------------------------------------------------------

    def launch_cluster(self, **call_kwargs):
        return self.call("launch_cluster",
                         node_count=self.node_count,
                         nodes=self.nodes,
                         shape=self.topology,
                         center_node_id=self.center_node_id,
                         extra_configs=self.extra_configs,
                         extra_args=self.extra_args,
                         special_log_levels=self.special_log_levels,
                         **call_kwargs)

    def stop_cluster(self, **call_kwargs):
        return self.call("stop_cluster", **call_kwargs)

    def clean_cluster(self, **call_kwargs):
        return self.call("clean_cluster", **call_kwargs)

    def start_node(self, node_id, extra_args=None, **call_kwargs):
        return self.call("start_node", node_id=node_id, extra_args=extra_args, **call_kwargs)

    def stop_node(self, node_id, kill_sig=15, check=True, dont_raise=False, **call_kwargs):
        """kill_sig: 15 for soft kill, 9 for hard kill"""
        cx = self.call("stop_node", node_id=node_id, kill_sig=kill_sig, **call_kwargs)
        if check:
            for _ in range(10):
                if self.is_node_down(node_id=node_id):
                    return cx
                time.sleep(1)
            else:
                if not dont_raise:
                    raise BlockchainError(f"Node {node_id} cannot be properly stopped by signal {kill_sig}.")
        else:
            return cx

    def cleanup(self):
        self.print_begin("Shutdown cluster")
        result = self.stop_cluster()
        if len(result.response_dict["result"]):
            missed_intervals = result.response_dict["result"]
            intervals = [ "{}-{}".format(missed["start"], missed["stop"]) for missed in missed_intervals]
            self.error("EXCESSIVE SYSTEM DELAYS DURING THE FOLLOWING {} BEGINNING-ENDING PERIODS: [{}]".format(len(missed_intervals), ", ".join(intervals)))
        self.print_end("Shutdown cluster")

# --------------- simple queries: queries that are made directly via call() -------------------------------------------

    def get_cluster_running_state(self, **call_kwargs):
        return self.call("get_cluster_running_state", **call_kwargs)

    def get_cluster_info(self, **call_kwargs):
        return self.call("get_cluster_info", **call_kwargs)

    def get_info(self, node_id, **call_kwargs):
        return self.call("get_info", node_id=node_id, **call_kwargs)

    def get_block(self, block_num_or_id, node_id=0, **call_kwargs):
        ret = self.call("get_block", node_id=node_id, block_num_or_id=block_num_or_id, **call_kwargs)
        return ret

    def get_account(self, name, node_id=0, **call_kwargs):
        return self.call("get_account", node_id=node_id, name=name, **call_kwargs)

    def get_protocol_features(self, node_id=0, **call_kwargs):
        return self.call("get_protocol_features", node_id=node_id, **call_kwargs)

    def get_log_data(self, offset, node_id=0, length=10000, filename="stderr_0.txt", **call_kwargs):
        return self.call("get_log_data", node_id=node_id, offset=offset, length=length, filename=filename, **call_kwargs)

    def verify_transaction(self, transaction_id, verify_key=None, node_id=0, **call_kwargs):
        try:
            return self.call("verify_transaction", node_id=node_id, transaction_id=transaction_id,
                             verify_key=None, dont_flush=True, **call_kwargs).response_dict[verify_key]
        except KeyError:
            return False

# --------------- composite queries: queries that are dependent on simple queries -------------------------------------

    def log_response_error(func):
        @functools.wraps(func)
        def wrapper(self, *args, **kwargs):
            try:
                return func(self, *args, **kwargs)
            except (KeyError, IndexError, TypeError):
                self.error("Response dictionary may have changed format.")
                raise
        return wrapper

    @log_response_error
    def get_node_pid(self, node_id, **call_kwargs):
        return self.get_cluster_running_state(**call_kwargs).response_dict["result"]["nodes"][node_id][1]["pid"]

    def is_node_down(self, node_id, **call_kwargs):
        return self.get_node_pid(node_id, **call_kwargs) == 0

    @log_response_error
    def is_node_ready(self, node_id, **call_kwargs):
        return "error" not in self.get_cluster_info(**call_kwargs).response_dict["result"][node_id][1]

    @log_response_error
    def get_head_block_number(self, node_id=0, **call_kwargs):
        """Get head block number by node id."""
        return self.get_cluster_info(**call_kwargs).response_dict["result"][node_id][1]["head_block_num"]

    @log_response_error
    def get_head_block_producer(self, node_id=0, **call_kwargs):
        """Get head block producer by node id."""
        return self.get_cluster_info(**call_kwargs).response_dict["result"][node_id][1]["head_block_producer"]

    @log_response_error
    def get_running_nodes(self, **call_kwargs):
        cluster_result = self.get_cluster_info(**call_kwargs).response_dict["result"]
        count = 0
        for node_result in cluster_result:
            if "head_block_id" in node_result[1]:
                count += 1
        return count

    @log_response_error
    def get_log(self, node_id=0, length=10000, filename="stderr_0.txt", **call_kwargs):
        log = ""
        offset = 0
        while True:
            response = self.get_log_data(node_id=node_id, offset=offset, length=length, filename=filename, **call_kwargs).response_dict
            log += base64.b64decode(response["data"]).decode("utf-8")
            if response["offset"] + length > response["filesize"]:
                break
            offset += length
        return log

    def wait_nodes_ready(self, node_list:list=None, retry=100, sleep=1, dont_raise=False, level="debug", sublevel="trace"):
        if node_list is None: node_list = list(range(self.node_count))
        self.print_header("wait for nodes to get ready", level=level)
        max_wait_time = retry * sleep
        begin_time = time.time()
        self.log(f"Max wait time = {max_wait_time}s ({retry} * {sleep}s)", level=level)
        while True:
            check_time = time.time()
            result = self.get_cluster_info(level=sublevel).response_dict["result"]
            error_node_list = [x[0] for x in result if x[0] in node_list and "error" in x[1]]
            error_node_count = len(error_node_list)
            if error_node_count == 0:
                self.log(f"All {len(node_list)} nodes are ready.", level=level)
                return True
            retry = int((max_wait_time + begin_time - time.time()) / sleep)
            if retry > 0:
                time_to_sleep = sleep if time.time() - check_time < sleep else 0
                self.log(f"Nodes that are not ready: {error_node_list}. "
                         f"Max {retry} {helper.plural(['retry', 'retries'], retry)} remain. "
                         + (f"Sleep for {time_to_sleep}s." if time_to_sleep else ""), level=level)
                time.sleep(time_to_sleep)
            else:
                msg = f"After waiting for {max_wait_time}s, there still are nodes that are not ready: {error_node_list}."
                self.error(msg)
                if not dont_raise:
                    raise BlockchainError(msg)
                return False

    def wait_get_block(self, block_num, node_id=0, retry=10, dont_raise=False, level="debug", sublevel="trace"):
        """Get block information by block num. If that block has not been produced, wait for it."""
        for _ in range(10):
            head_block_num = self.get_head_block_number(level=sublevel)
            if head_block_num < block_num:
                time.sleep(0.5 * (block_num - head_block_num))
            else:
                return self.get_block(block_num_or_id=block_num, node_id=node_id, level=level).response_dict
        msg = f"Cannot get block num {block_num}. Current head block num is {head_block_num}."
        self.error(msg)
        if not dont_raise:
            raise BlockchainError(msg)

    @log_response_error
    def wait_get_producer_by_block(self, block_num, node_id=0, retry=10, dont_raise=False, level="debug", sublevel="trace"):
        """Get block producer by block num. If that block has not been produced, wait for it."""
        return self.wait_get_block(block_num=block_num, node_id=node_id, retry=retry, dont_raise=dont_raise,
                                   level=level, sublevel=sublevel)["producer"]

# --------------- transactions ----------------------------------------------------------------------------------------

    def schedule_protocol_feature_activations(self, features=[PREACTIVATE_FEATURE], **call_kwargs):
        return self.call("schedule_protocol_feature_activations",
                         protocol_features_to_activate=features,
                         **call_kwargs)

    def set_contract(self, account, contract_file, abi_file, node_id=0, verify_key="irreversible", name=None, **call_kwargs):
        return self.call("set_contract",
                         node_id=node_id,
                         account=account,
                         contract_file=contract_file,
                         abi_file=abi_file,
                         verify_key=verify_key,
                         header=f"set <{name}> contract" if name else None,
                         **call_kwargs)

    def push_actions(self, actions, node_id=0, verify_key="irreversible", **call_kwargs):
        return self.call("push_actions",
                         node_id=node_id,
                         actions=actions,
                         verify_key=verify_key,
                         **call_kwargs)

# --------------- send-raw --------------------------------------------------------------------------------------------

    def send_raw(self, url, node_id=0, string_data: str="", json_data: dict={}, **call_kwargs):
        return self.call("send_raw", url=url, node_id=node_id, string_data=string_data, json_data=json_data, **call_kwargs)

    def pause_node_production(self, node_id, **call_kwargs):
        return self.send_raw(url="/v1/producer/pause", node_id=node_id, **call_kwargs)

    def resume_node_production(self, node_id, **call_kwargs):
        return self.send_raw(url="/v1/producer/resume",node_id=node_id, **call_kwargs)

    def get_greylist(self, node_id=0, **call_kwargs):
        return self.send_raw(url="/v1/producer/get_greylist", node_id=node_id, **call_kwargs)

    def add_greylist_accounts(self, accounts:list, node_id=0, **call_kwargs):
        return self.send_raw(url="/v1/producer/add_greylist_accounts", node_id=node_id, json_data={"accounts": accounts}, **call_kwargs)

    def remove_greylist_accounts(self, accounts:list, node_id=0, **call_kwargs):
        return self.send_raw(url="/v1/producer/remove_greylist_accounts", node_id=node_id, json_data={"accounts": accounts}, **call_kwargs)

    def get_net_plugin_connections(self, node_id=0, **call_kwargs):
        return self.send_raw(url="/v1/net/connections", node_id=node_id, **call_kwargs)

# --------------- bios-launch-related ---------------------------------------------------------------------------------

    def set_bios_contract(self, **call_kwargs):
        contract = "eosio.bios"
        return self.set_contract(account="eosio",
                                 contract_file=self.make_wasm_name(contract),
                                 abi_file=self.make_abi_name(contract),
                                 name=contract,
                                 **call_kwargs)

    def bios_create_accounts(self, accounts: typing.Union[str, list], node_id=0, **call_kwargs):
        actions = []
        accounts = [accounts] if isinstance(accounts, str) else accounts
        for name in accounts:
            actions += [{"account": "eosio",
                         "action": "newaccount",
                         "permissions":[{"actor": "eosio",
                                         "permission": "active"}],
                          "data":{"creator": "eosio",
                                  "name": name,
                                  "owner": {"threshold": 1,
                                            "keys": [{"key": PRODUCER_KEY,
                                                      "weight":1}],
                                            "accounts": [],
                                            "waits": []},
                                  "active":{"threshold": 1,
                                            "keys": [{"key": PRODUCER_KEY,
                                                      "weight":1}],
                                            "accounts": [],
                                            "waits": []}}}]
        header = "bios create "
        header += f"\"{accounts[0]}\" account" if len(accounts) == 1 else f"{len(actions)} accounts"
        return self.push_actions(actions=actions, node_id=node_id, header=header, **call_kwargs)

    def bios_create_accounts_in_parallel(self, accounts, dont_raise=False, **call_kwargs):
        threads = []
        channel = {}
        def report(channel, thread_id, message):
            channel[thread_id] = message
        call_kwargs.update({"buffer": True})
        for ac in accounts:
            t = ExceptionThread(channel, report, target=self.bios_create_accounts, args=(ac,),
                                kwargs=call_kwargs)
            threads.append(t)
            t.start()
        for t in threads:
            t.join()
        error_count = len(channel)
        if error_count:
            msg = f"{error_count} {helper.plural('exception', error_count)} occurred in bios creating accounts."
            cnt = min(error_count, 10)
            msg += f"\nReporting first {cnt} {helper.plural('exception', cnt)}:"
            for i in range(cnt):
                msg += f"\n[{i}] {channel[i]}"
            self.error(msg)
            if not dont_raise:
                raise BlockchainError(msg)

    def set_producers(self, producers:list=None, **call_kwargs):
        if producers is None: producers = self.producers
        prod_keys = []
        for p in sorted(producers):
            prod_keys.append({"producer_name": p, "block_signing_key": PRODUCER_KEY})
        actions = [{"account": "eosio",
                    "action": "setprods",
                    "permissions": [{"actor": "eosio", "permission": "active"}],
                    "data": { "schedule": prod_keys}}]
        return self.push_actions(actions=actions, header="set producers", **call_kwargs)

# --------------- auxiliary -------------------------------------------------------------------------------------------

    def make_wasm_name(self, contract):
        """Make a wasm filename given the contract name."""
        return os.path.join(self.cdir, contract, contract + ".wasm")


    def make_abi_name(self, contract):
        """Make an abi filename given the contract name."""
        return os.path.join(self.cdir, contract, contract + ".abi")

    @staticmethod
    def make_defproducer_name(num):
        def base26_to_int(s: str):
            res = 0
            for c in s:
                res *= 26
                res += ord(c) - ord('a')
            return res

        def int_to_base26(x: int):
            res = ""
            while True:
                q, r = divmod(x, 26)
                res = chr(ord('a') + r) + res
                x = q
                if q == 0:
                    break
            return res
        # 8031810176 = 26 ** 7 is integer for "baaaaaaa" in base26
        return "defproducer" + string.ascii_lowercase[num] if num < 26 else "defpr" + int_to_base26(8031810176 + num)[1:]

# --------------- check and verify ------------------------------------------------------------------------------------

    def verify(self,
               transaction_id,
               verify_key,
               verify_async=False,
               dont_raise=False,
               retry=None,
               sleep=None,
               level=None,
               retry_level=None,
               error_level=None,
               buffer=None):

        retry = helper.override(self.verify_retry, retry)
        sleep = helper.override(self.verify_sleep, sleep)
        level = helper.override("debug", level)
        retry_level = helper.override("trace", retry_level)
        error_level = helper.override("error", error_level)
        buffer = True if verify_async else buffer

        def verify_helper():
            verified = False
            for _ in range(retry + 1):
                if self.verify_transaction(transaction_id=transaction_id, verify_key=verify_key, level=retry_level, buffer=buffer):
                    verified = True
                    break
                time.sleep(sleep)
            if verify_async:
                self.print_header("async verify transaction", level=level, buffer=True)
                if verified:
                    self.log(color.black_on_green(f"{verify_key.title()}") + f" {transaction_id}", level=level, buffer=True)
                else:
                    self.log(color.black_on_red(f"Not {verify_key.title()}") + f" {transaction_id}", level=error_level, buffer=True)
                self.print_header("", sep="", level=level, buffer=True)
                self.flush()
            else:
                if verified:
                    self.log(color.black_on_green(f"{verify_key.title()}"), level=level)
                else:
                    self.log(color.black_on_red(f"Not {verify_key.title()}"), level=error_level)
            if not verified and not dont_raise:
                raise BlockchainError(f"{transaction_id} cannot be verified")
            return verified

        if verify_async:
            t = threading.Thread(target=verify_helper)
            t.start()
            self.verify_threads.append(t)
        else:
            verify_helper()

    def check_sync(self, retry=None, sleep=None, min_sync_count=None, max_block_lag=None, expect_advance=True, dont_raise=False, level="debug"):
        class SyncResult:
            def __init__(self, in_sync: bool, sync_count: int, min_block_num: int, max_block_num: int = None):
                self.in_sync = in_sync
                self.sync_count = sync_count
                self.min_block_num = min_block_num
                self.max_block_num = max_block_num
                if self.in_sync:
                    self.block_num = self.max_block_num = self.min_block_num
                else:
                    assert self.min_block_num == math.inf or self.min_block_num <= self.max_block_num
                    self.block_num = -1
        def get_sync_result():
            class TimeTrack:
                def __init__(self, log, desc: str, always_report_over_this_time=None):
                    self.log = log
                    self.max = None
                    self.init_time = time.time()
                    self.start_time = self.init_time
                    self.last = None
                    self.desc = desc
                    self.always_report_over_this_time = always_report_over_this_time
                    self.last = None
                    self.total = None

                def start(self):
                    self.start_time = time.time()

                def stop(self, restart_interval=False):
                    current = time.time()
                    self.last = current - self.start_time
                    self.total = current - self.init_time
                    reported = self._set_max()
                    if not reported and self.always_report_over_this_time is not None and self.last > self.always_report_over_this_time:
                        self.log(f"{self.desc} took {TimeTrack._format(self.last)} seconds to be performed.",
                                 level=level)

                    if restart_interval:
                        self.start_time = current

                    return self.last

                @staticmethod
                def _format(val):
                    return "{:.2f}".format(val)

                def _set_max(self):
                    reported = False
                    if self.max is None:
                        self.max = self.last
                    elif self.max < self.last:
                        diff = self.last - self.max
                        # only report if new max is over 2.0 sec, and more than double the previous
                        if 2.0 < self.last and self.max < diff:
                            percent = int(100 * diff / self.max)
                            self.log(f"{self.desc} took {percent}% more time than the previous max time for this action (new max={TimeTrack._format(self.last)} seconds).",
                                     level=level)
                            reported = True

                        self.max = self.last

                    return reported

            # track the total time for this method
            total_time = TimeTrack(log = self.log, desc = "Getting sync results")
            limit_running_time = 10 * sleep * retry

            # track retrieving cluster info and calling sleep individually
            ci_time = TimeTrack(log = self.log, desc = "Retrieving cluster info")
            sleep_time = TimeTrack(log = self.log, desc = f"Fixed Sleep time of {sleep} seconds", always_report_over_this_time=sleep*5)

            for try_num in range(retry + 1):
                ci_time.start()
                cx = self.get_cluster_info(level="trace")
                ci_time.stop()

                has_head_block_id = lambda node_id: "head_block_id" in cx.response_dict["result"][node_id][1]
                extract_head_block_id = lambda node_id: cx.response_dict["result"][node_id][1]["head_block_id"]
                extract_head_block_num = lambda node_id: cx.response_dict["result"][node_id][1]["head_block_num"]
                counter = collections.defaultdict(int)
                headless = 0
                max_block_num, min_block_num = -1, math.inf
                max_block_node = min_block_node = -1
                sync_count = 0
                for node_id in range(self.node_count):
                    if has_head_block_id(node_id):
                        block_num = extract_head_block_num(node_id)
                        if block_num > max_block_num:
                            max_block_num, max_block_node = block_num, node_id
                        if block_num < min_block_num:
                            min_block_num, min_block_node = block_num, node_id
                        head_id = extract_head_block_id(node_id)
                        counter[head_id] += 1
                        if counter[head_id] > sync_count:
                            sync_count, sync_block, sync_head = counter[head_id], block_num, head_id
                    else:
                        headless += 1
                down_info = f"({headless} {helper.plural('node', headless)} down)"
                self.log(f"{sync_count}/{self.node_count} {helper.plural('node', sync_count)} in sync: "
                         f"max block num {max_block_num} from node {max_block_node}, "
                         f"min block num {min_block_num} from node {min_block_node} "
                         f"{down_info}", level=level)
                if sync_count >= min_sync_count:
                    self.log(color.green(f"<Sync Node Count> {sync_count}"), level=level)
                    self.log(color.green(f"<Sync Block Num> {sync_block}"), level=level)
                    self.log(color.green(f"<Sync Block ID> {sync_head}"), level=level)
                    return SyncResult(True, sync_count, min_block_num)
                if max_block_lag is not None and max_block_num - min_block_num > max_block_lag:
                    self.log(f"Lag between min and max block numbers (={max_block_num - min_block_num}) "
                             f"is larger than tolerance (={max_block_lag}).", level=level)
                    break

                total_time.stop(restart_interval=True)
                if total_time.total > limit_running_time:
                    msg = f"get_sync_result taking considerably longer than expected: {total_time.total} seconds," +\
                          f" after running {try_num} of {retry} retries"
                    self.error(color.black_on_red(msg))
                    raise SyncError(msg)
                sleep_time.start()
                time.sleep(sleep)
                sleep_time.stop()
            msg = "Nodes out of sync"
            if not dont_raise:
                self.error(color.black_on_red(msg))
                raise SyncError(msg)
            else:
                self.log(color.black_on_yellow(msg), level=level)
            return SyncResult(False, sync_count, min_block_num, max_block_num)
        retry = helper.override(self.sync_retry, retry, self.cla.sync_retry)
        sleep = helper.override(self.sync_sleep, sleep, self.cla.sync_sleep)
        min_sync_count = helper.override(self.node_count, min_sync_count)
        self.print_header("check_sync", level=level)
        self.log(f"Max {retry} retries. Sleep for {sleep}s between retries.", level=level)
        if max_block_lag:
            self.log(f"Max tolerable lag between min and max block numbers = {max_block_lag}.", level=level)
        self.log(f"Expect at least {min_sync_count} of {self.node_count} nodes in sync.", level=level)
        if expect_advance:
            self.log("Expect block numbers to be advancing.", level=level)
        res = get_sync_result()
        if not expect_advance:
            return res
        else:
            for _ in range(retry):
                if not res.in_sync:
                    return res
                else:
                    time.sleep(sleep)
                    old = res
                    res = get_sync_result()
                    if res.block_num > old.block_num:
                        self.log(f"Block number advanced from {old.block_num} to {res.block_num}.", level=level)
                        self.log(color.black_on_green(f"Node in sync"), level=level)
                        return res
        msg = "Nodes not advancing"
        if not dont_raise:
            self.error(color.black_on_red(msg))
            raise SyncError(msg)
        return res

    def check_production_round(self, expected_producers: typing.List[str], new_producers={}, level="debug", dont_raise=False, min_required_per_round=10):
        self.print_header("check production round", level=level)
        full_round = 12
        if min_required_per_round is None:
            min_required_per_round = full_round
        if min_required_per_round > full_round:
            msg = f"Cannot set min_required_per_round (={min_required_per_round}) greater than full_round (={full_round})"
            self.error(msg)
            raise BlockchainError(msg)

        # list expected producers
        self.log("Expected producers:", level=level)
        for i, v in enumerate(expected_producers):
            self.log(f"[{i}] {v}", level=level)
        # skip unexpected producers
        begin_block_num = self.get_head_block_number(level="trace")
        curr_prod = self.wait_get_producer_by_block(begin_block_num, level="trace")
        if len(new_producers) == 0:
           new_producers = expected_producers
        while curr_prod not in new_producers:
            self.log(f"Block #{begin_block_num}: {curr_prod} is not among new producers. "
                     "Waiting for schedule change.", level=level)
            begin_block_num += 1
            curr_prod = self.wait_get_producer_by_block(begin_block_num, level="trace")

        # when a new schedule has started, if the first producer is new, it will start after the first block in the slot
        # since the first block of the slot will provide the confirmation to make the new schedule be confirmed
        last_prod = curr_prod
        while curr_prod == last_prod:
            self.log(f"Block #{begin_block_num}: {curr_prod}, waiting for schedule change to ensure complete first round.",
                     level=level)
            begin_block_num += 1
            curr_prod = self.wait_get_producer_by_block(begin_block_num, level="trace")
        # formally start
        self.log(f">>> Production check formally starts, as expected producer \"{curr_prod}\" has come to produce.", level=level)
        rest = full_round * len(expected_producers) - 1
        self.log(f"Block #{begin_block_num}: {curr_prod} has produced 1 block in this round. "
                 f"{rest} {helper.plural('block', rest)} {helper.singular('remain', rest)} to to be checked.", level=level)
        counter = {x: 0 for x in expected_producers}
        counter[curr_prod] += 1
        last_prod = curr_prod
        producers_in_schedule = len(expected_producers)
        end_block_num = begin_block_num + full_round * producers_in_schedule
        for num in range(begin_block_num + 1, end_block_num):
            curr_prod = self.wait_get_producer_by_block(num, level="trace")
            if curr_prod not in expected_producers:
                msg = f"Unexpected producer \"{curr_prod}\" in block #{num}."
                self.error(msg)
                if not dont_raise:
                    raise BlockchainError(msg)
            if curr_prod != last_prod:
                num_identified_prod = 0
                for count in counter.values():
                    if count > 0:
                        num_identified_prod += 1
                if num_identified_prod == producers_in_schedule:
                    self.log(f"All {producers_in_schedule} producers were identified and "
                             f"each produced at least {min_required_per_round} blocks", level)
                    break

                if counter[curr_prod] > 0:
                    count = counter[curr_prod]
                    msg = (f"Producer changes to \"{curr_prod}\" after last producer \"{last_prod}\", but it was "
                           f"already identified earlier in the production round and only {num_identified_prod} of "
                           f"the expected {producers_in_schedule} producers had produced.")
                    self.error(msg)
                    if not dont_raise:
                        raise BlockchainError(msg)

                counter[curr_prod] += 1
                if counter[last_prod] < min_required_per_round or counter[last_prod] > full_round:
                    count = counter[last_prod]
                    msg = (f"Producer changes to \"{curr_prod}\" after last producer \"{last_prod}\" "
                           f"produces {count} {helper.plural('block', count)}.")
                    self.error(msg)
                    if not dont_raise:
                        raise BlockchainError(msg)
            else:
                counter[curr_prod] += 1
            rest = end_block_num - num - 1
            count = counter[curr_prod]
            self.log(f"Block #{num}: {curr_prod} has produced {count} {helper.plural('block', count)} in this round. "
                     f"{rest} {helper.plural('block', rest)} {helper.singular('remain', rest)} to to be checked.", level=level)
            last_prod = curr_prod

        # summarize
        expected_counter = {x: full_round for x in expected_producers}
        success = counter == expected_counter
        if not success:
            # check if we succeeded by the min_required_per_round requirements
            if min_required_per_round < full_round:
                success = True
                for key, value in counter.items():
                    if key not in expected_counter or value < min_required_per_round or value > full_round:
                        success = False
                        break
                    del expected_counter[key]
                if len(expected_counter) > 0:
                    success = False

        if success:
            self.log(">>> Production check succeeded.", level=level)
            return True

        msg = ">>> Production check failed."
        for prod in counter:
            msg += f"\n{prod} produced {counter[prod]} {helper.plural('block', counter[prod])}."
        self.error(msg)
        if not dont_raise:
            raise BlockchainError(msg)
        return False

# --------------- call ------------------------------------------------------------------------------------------------

    def call(self,
             api: str,
             retry=None,
             sleep=None,
             dont_raise=False,
             verify_key=None,
             verify_async=None,
             verify_retry=None,
             verify_sleep=None,
             verify_dont_raise=None,
             header=None,
             level=None,
             header_level=None,
             url_level=None,
             request_text_level=None,
             retry_code_level=None,
             retry_text_level=None,
             response_code_level=None,
             response_text_level=None,
             transaction_id_level=None,
             no_transaction_id_level=None,
             error_level=None,
             error_text_level=None,
             verify_level=None,
             verify_retry_level=None,
             verify_error_level=None,
             buffer=False,
             dont_flush=False,
             **data) -> Connection:
        """Call launcher service API. Post HTTP request and get response.

        Steps to take to make a call
        ----------------------------
        1. print header
        2. establish HTTP connection
        3. log url and request of connection
        4. retry connection if response not ok
        5. log response
        6. verify transaction
        7. return the result as a Connection object

        Launcher service API
        --------------------
        @ plugins/launcher_service_plugin/include/eosio/launcher_service_plugin/launcher_service_plugin.hpp
        --- cluster-related calls ---------------
        1. launch_cluster
        2. stop_cluster
        3. stop_all_clusters
        4. clean_cluster
        5. start_node
        6. stop_node
        --- wallet-related calls ----------------
        7. generate_key
        8. import_keys
        --- queries -----------------------------
        9. get_cluster_info
        10. get_cluster_running_state
        11. get_info
        12. get_block
        13. get_account
        14. get_code_hash
        15. get_protocol_features
        16. get_table_rows
        17. get_log_data
        18. verify_transaction
        --- transactions ------------------------
        19. schedule_protocol_feature_activations
        20. set_contract
        21. push_actions
        --- miscellaneous------------------------
        22. send_raw
        """
        header = helper.override(api.replace("_", " "), header)
        retry = helper.override(self.http_retry, retry, self.cla.http_retry)
        sleep = helper.override(self.http_sleep, sleep, self.cla.http_sleep)
        data.setdefault("cluster_id", self.cluster_id)
        data.setdefault("node_id", 0)
        level = helper.override("debug", level)
        header_level = helper.override(level, header_level)
        url_level = helper.override(level, url_level)
        request_text_level = helper.override(level, request_text_level)
        retry_code_level = helper.override("trace", retry_code_level)
        retry_text_level = helper.override("trace", retry_text_level)
        response_code_level = helper.override(level, response_code_level)
        response_text_level = helper.override("trace", response_text_level)
        transaction_id_level = helper.override(level, transaction_id_level)
        no_transaction_id_level = helper.override(transaction_id_level, no_transaction_id_level)
        error_level = helper.override("error", error_level)
        error_text_level = helper.override(error_level, error_text_level)
        self.print_header(header, level=header_level, buffer=buffer)

        # let logger know that we are communicating with launcher-service
        if self.service.launcher_maximum_idle_time is not None:
            self.logger.reset_launcher_maximum_idle_time(self.service.launcher_maximum_idle_time)

        # communication with launcher service
        cx = Connection(url=f"http://{self.service.addr}:{self.service.port}/v1/launcher/{api}", data=data)
        self.log(cx.url, level=url_level, buffer=buffer)
        self.log(cx.request_text, level=request_text_level, buffer=buffer)
        while not cx.ok and retry > 0:
            self.log(cx.response_code, level=retry_code_level, buffer=buffer)
            self.log(cx.response_text, level=retry_text_level, buffer=buffer)
            self.log(f"{retry} retries remain for http connection. Sleep for {sleep}s.", level=retry_code_level, buffer=buffer)
            time.sleep(sleep)
            cx.attempt()
            retry -= 1
        if cx.response.ok:
            self.log(cx.response_code, level=response_code_level, buffer=buffer)
            if cx.transaction_id:
                self.log(color.green(f"<Transaction ID> {cx.transaction_id}"), level=transaction_id_level, buffer=buffer)
            else:
                self.log(color.yellow("<No Transaction ID>"), level=no_transaction_id_level, buffer=buffer)
            self.log(cx.response_text, level=response_text_level, buffer=buffer)
        else:
            self.log(cx.response_code, level=error_level, buffer=buffer)
            self.log(cx.response_text, level=error_text_level, buffer=buffer)
            if not dont_raise:
                raise LauncherServiceError(cx.summation())
        # verification of transaction
        if verify_key:
            self.verify(transaction_id=cx.transaction_id,
                        verify_key=verify_key,
                        verify_async=helper.override(self.verify_async, verify_async),
                        dont_raise=helper.override(dont_raise, verify_dont_raise),
                        retry=helper.override(self.verify_retry, verify_retry),
                        sleep=helper.override(self.verify_sleep, verify_sleep),
                        level=helper.override(level, verify_level),
                        retry_level=helper.override(retry_code_level, verify_retry_level),
                        error_level=helper.override(error_level, verify_error_level),
                        buffer=True if verify_async else buffer)
        if buffer and not dont_flush:
            self.flush()
        return cx

# =============== END OF CLUSTER CLASS ================================================================================

def _init_cluster():
    logger = Logger(ScreenWriter(threshold="debug"),
                    FileWriter(filename="service-info.log", threshold="info", monochrome=True),
                    FileWriter(filename="service-debug.log", threshold="debug", monochrome=True),
                    FileWriter(filename="service-trace.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)
    cluster = Cluster(service=service)
    return cluster

def _main():
    with _init_cluster():
        pass

if __name__ == "__main__":
    _main()
