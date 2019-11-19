#! /usr/bin/env python3

# standard libraries
from typing import List, Optional, Union
from dataclasses import dataclass
import argparse
import collections
import base64
import json
import math
import os
import platform
import shlex
import string
import subprocess
import threading
import time
import typing

# third-party libraries
import requests

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


PROGRAM = "launcher-service"
PROGRAM_LOG = "launcher-service.log"
PRODUCER_KEY = "EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
PREACTIVATE_FEATURE = "0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"
PACKAGE_DIR = os.path.dirname(os.path.abspath(__file__))

# service-related defaults
DEFAULT_ADDR = "127.0.0.1"
DEFAULT_PORT = 1234
DEFAULT_WDIR = os.path.join(PACKAGE_DIR, "../../build")
DEFAULT_FILE = os.path.join(".", "programs", PROGRAM, PROGRAM)
DEFAULT_START = False
DEFAULT_KILL = False
# cluster-related defaults
DEFAULT_CONTRACTS_DIR = "../unittests/contracts"
DEFAULT_CLUSTER_ID = 0
DEFAULT_NODE_COUNT = 4
DEFAULT_PNODE_COUNT = 4
DEFAULT_PRODUDCER_COUNT = 4
DEFAULT_UNSTARTED_COUNT = 0
DEFAULT_SHAPE = "mesh"
DEFAULT_CENTER_NODE_ID = None
DEFAULT_EXTRA_CONFIGS = []
DEFAULT_EXTRA_ARGS = ""
DEFAULT_TOTAL_SUPPLY = 1e9
DEFAULT_LAUNCH_MODE = "bios"
DEFAULT_REGULAR_LAUNCH = False
DEFAULT_DONT_NEWACCOUNT = False
DEFAULT_DONT_SETPROD = False
DEFAULT_DONT_VOTE = False
DEFAULT_HTTP_RETRY = 10
DEFAULT_HTTP_SLEEP = 0.25
DEFAULT_VERIFY_ASYNC = False
DEFAULT_VERIFY_RETRY = 10
DEFAULT_VERIFY_SLEEP = 0.25
DEFAULT_SYNC_RETRY = 100
DEFAULT_SYNC_SLEEP = 1
DEFAULT_PRODUCER_RETRY = 100
DEFAULT_PRODUCER_SLEEP = 1
# logger-related defaults
DEFAULT_MONOCHROME = False
DEFAULT_BUFFERED = True
DEFAULT_SHOW_CLOCK_TIME = True
DEFAULT_SHOW_ELAPSED_TIME = True
DEFAULT_SHOW_FILENAME = True
DEFAULT_SHOW_LINENO = True
DEFAULT_SHOW_FUNCTION = True
DEFAULT_SHOW_THREAD = True
DEFAULT_SHOW_LOG_LEVEL = True
DEFAULT_HIDE_ALL = False
# service-related help
HELP_HELP = "Show this message and exit"
HELP_ADDR = "IP address of launcher service"
HELP_PORT = "Listening port of launcher service"
HELP_WDIR = "Working directory"
HELP_FILE = "Path to local launcher service file"
HELP_START = "Always start a new launcher service"
HELP_KILL = "Kill existing launcher services (if any)"
# cluster-related help
HELP_CONTRACTS_DIR = "Directory of eosio smart contracts"
HELP_CLUSTER_ID = "Cluster ID to launch with"
HELP_NODE_COUNT = "Number of nodes"
HELP_PNODE_COUNT = "Number of nodes with producers"
HELP_PRODUDCER_COUNT = "Number of producers"
HELP_UNSTARTED_COUNT = "Number of unstarted nodes"
HELP_SHAPE = "Cluster topology to launch with"
HELP_CENTER_NODE_ID = "Center node ID for star or bridge"
HELP_EXTRA_CONFIGS = "Extra configs to pass to launcher service"
HELP_EXTRA_ARGS = "Extra arguments to pass to launcher service"
HELP_TOTAL_SUPPLY = "Total supply of tokens"
HELP_REGULAR_LAUNCH = "Launch cluster in a regular (not BIOS) mode"
HELP_DONT_NEWACCOUNT = "Do not create accounts"
HELP_DONT_SETPROD = "Do not set producers in bios launch"
HELP_DONT_VOTE = "Do not vote for producers in regular launch"
HELP_HTTP_RETRY = "HTTP connection: max num of retries"
HELP_HTTP_SLEEP = "HTTP connection: sleep time between retries"
HELP_VERIFY_ASYNC = "Verify transaction: verify asynchronously"
HELP_VERIFY_RETRY = "Verify transaction: max num of retries"
HELP_VERIFY_SLEEP = "Verify transaction: sleep time between retries"
HELP_SYNC_RETRY = "Check nodes in sync: max num of retries"
HELP_SYNC_SLEEP = "Check nodes in sync: sleep time between retries"
HELP_PRODUCER_RETRY = "Max retries for head block producer"
HELP_PRODUCER_SLEEP = "Sleep time for head block producer retries"
# logger-related help
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
HELP_HIDE_CLOCK_TIME = "Hide clock time in stdout logging"
HELP_HIDE_ELAPSED_TIME = "Hide elapsed time in stdout logging"
HELP_HIDE_FILENAME = "Hide filename in stdout logging"
HELP_HIDE_LINENO = "Hide line number in stdout logging"
HELP_HIDE_FUNCTION = "Hide function name in stdout logging"
HELP_HIDE_THREAD = "Hide thread name in stdout logging"
HELP_HIDE_LOG_LEVEL = "Hide log level in stdout logging"
HELP_HIDE_ALL = "Hide all the above in stdout logging"


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


class LauncherServiceError(RuntimeError):
    def __init__(self, message):
        super().__init__(message)


class BlockchainError(RuntimeError):
    def __init__(self, message):
        super().__init__(message)


def bassert(cond, message=None):
    if not cond:
        raise BlockchainError(message=message)


class SyncError(BlockchainError):
    def __init__(self, message):
        super().__init__(message)


class CommandLineArguments:
    def __init__(self):
        cla = self.parse()
        self.addr = cla.addr
        self.port = cla.port
        self.wdir = cla.wdir
        self.file = cla.file
        self.start = cla.start
        self.kill = cla.kill

        self.contracts_dir = cla.contracts_dir
        self.total_supply = cla.total_supply
        self.cluster_id = cla.cluster_id
        self.node_count = cla.node_count
        self.pnode_count = cla.pnode_count
        self.producer_count = cla.producer_count
        self.unstarted_count = cla.unstarted_count
        self.shape = cla.shape
        self.center_node_id = cla.center_node_id
        self.launch_mode = cla.launch_mode
        self.dont_newaccount = cla.dont_newaccount
        self.dont_setprod = cla.dont_setprod
        self.dont_vote = cla.dont_vote
        self.http_retry = cla.http_retry
        self.http_sleep = cla.http_sleep
        self.verify_async = cla.verify_async
        self.verify_retry = cla.verify_retry
        self.verify_sleep = cla.verify_sleep
        self.sync_retry = cla.sync_retry
        self.sync_sleep = cla.sync_sleep
        self.producer_retry = cla.producer_retry
        self.producer_sleep = cla.producer_sleep

        self.threshold = cla.threshold
        self.buffered = cla.buffered
        self.monochrome = cla.monochrome
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
        desc = color.decorate("Launcher Service-based EOSIO Testing Framework", style="underline", fcolor="green")
        left = 5
        form = lambda text, value=None: "{} ({})".format(helper.pad(text, left=left, total=55), value)
        parser = argparse.ArgumentParser(description=desc, add_help=False, formatter_class=lambda prog: argparse.RawTextHelpFormatter(prog, max_help_position=50))
        parser.add_argument("-h", "--help", action="help", help=" " * left + HELP_HELP)
        # service-related options
        parser.add_argument("-a", "--addr", type=str, metavar="IP", help=form(HELP_ADDR, DEFAULT_ADDR))
        parser.add_argument("-o", "--port", type=int, help=form(HELP_PORT, DEFAULT_PORT))
        parser.add_argument("-w", "--wdir", type=str, metavar="PATH", help=form(HELP_WDIR, DEFAULT_WDIR))
        parser.add_argument("-f", "--file", type=str, metavar="PATH", help=form(HELP_FILE, DEFAULT_FILE))
        parser.add_argument("-s", "--start", action="store_true", default=None, help=form(HELP_START, DEFAULT_START))
        parser.add_argument("-k", "--kill", action="store_true", default=None, help=form(HELP_KILL, DEFAULT_KILL))
        # cluster-related options
        parser.add_argument("-d", "--contracts-dir", metavar="PATH", help=form(HELP_CONTRACTS_DIR, DEFAULT_CONTRACTS_DIR))
        parser.add_argument("-i", "--cluster-id", dest="cluster_id", type=int, metavar="ID", help=form(HELP_CLUSTER_ID, DEFAULT_CLUSTER_ID))
        parser.add_argument("-n", "--node-count", type=int, metavar="NUM", help=form(HELP_NODE_COUNT, DEFAULT_NODE_COUNT))
        parser.add_argument("-p", "--pnode-count", type=int, metavar="NUM", help=form(HELP_PNODE_COUNT, DEFAULT_PNODE_COUNT))
        parser.add_argument("-q", "--producer-count", type=int, metavar="NUM", help=form(HELP_PRODUDCER_COUNT, DEFAULT_PRODUDCER_COUNT))
        parser.add_argument("-u", "--unstarted-count", type=int, metavar="NUM", help=form(HELP_UNSTARTED_COUNT, DEFAULT_UNSTARTED_COUNT))
        parser.add_argument("-t", "--shape", type=str, metavar="TOPOLOGY", help=form(HELP_SHAPE, DEFAULT_SHAPE), choices={"mesh", "star", "bridge", "line", "ring", "tree"})
        parser.add_argument("-c", "--center-node-id", type=int, metavar="ID", help=form(HELP_CENTER_NODE_ID, DEFAULT_CENTER_NODE_ID))
        parser.add_argument("-g", "--total-supply", metavar="NUM", help=form(HELP_TOTAL_SUPPLY, "{:g}".format(DEFAULT_TOTAL_SUPPLY)))
        parser.add_argument("-r", "--regular-launch", dest="launch_mode", action="store_const", const="regular", default=None, help=form(HELP_REGULAR_LAUNCH, DEFAULT_REGULAR_LAUNCH))
        parser.add_argument("-xn", "--dont-newaccount", action="store_true", default=None, help=form(HELP_DONT_NEWACCOUNT, DEFAULT_DONT_NEWACCOUNT))
        parser.add_argument("-xs", "--dont-setprod", action="store_true", default=None, help=form(HELP_DONT_SETPROD, DEFAULT_DONT_SETPROD))
        parser.add_argument("-xv", "--dont-vote", action="store_true", default=None, help=form(HELP_DONT_VOTE, DEFAULT_DONT_VOTE))
        parser.add_argument("--http-retry", type=int, metavar="NUM", help=form(HELP_HTTP_RETRY, DEFAULT_HTTP_RETRY))
        parser.add_argument("--http-sleep", type=float, metavar="TIME", help=form(HELP_HTTP_SLEEP, DEFAULT_HTTP_SLEEP))
        parser.add_argument("--verify-async", action="store_true", default=None, help=form(HELP_VERIFY_ASYNC, DEFAULT_VERIFY_ASYNC))
        parser.add_argument("--verify-retry", type=int, metavar="NUM", help=form(HELP_VERIFY_RETRY, DEFAULT_VERIFY_RETRY))
        parser.add_argument("--verify-sleep", type=float, metavar="TIME", help=form(HELP_VERIFY_SLEEP, DEFAULT_VERIFY_SLEEP))
        parser.add_argument("--sync-retry", type=int, metavar="NUM", help=form(HELP_SYNC_RETRY, DEFAULT_SYNC_RETRY))
        parser.add_argument("--sync-sleep", type=float, metavar="TIME", help=form(HELP_SYNC_SLEEP, DEFAULT_SYNC_SLEEP))
        parser.add_argument("--producer-retry", type=int, metavar="NUM", help=form(HELP_PRODUCER_RETRY, DEFAULT_PRODUCER_RETRY))
        parser.add_argument("--producer-sleep", type=float, metavar="TIME", help=form(HELP_PRODUCER_SLEEP, DEFAULT_PRODUCER_SLEEP))
        # logger-related options
        threshold = parser.add_mutually_exclusive_group()
        threshold.add_argument("-l", "--log-level", dest="threshold", type=int, metavar="LEVEL", action="store", help=form(HELP_LOG_LEVEL))
        threshold.add_argument("--all", dest="threshold", action="store_const", const="all", help=form(HELP_LOG_ALL))
        threshold.add_argument("--trace", dest="threshold", action="store_const", const="trace", help=form(HELP_TRACE))
        threshold.add_argument("--debug", dest="threshold", action="store_const", const="debug", help=form(HELP_DEBUG))
        threshold.add_argument("--info", dest="threshold", action="store_const", const="info", help=form(HELP_INFO))
        threshold.add_argument("--warn", dest="threshold", action="store_const", const="warn", help=form(HELP_WARN))
        threshold.add_argument("--error", dest="threshold", action="store_const", const="error", help=form(HELP_ERROR))
        threshold.add_argument("--fatal", dest="threshold", action="store_const", const="fatal", help=form(HELP_FATAL))
        threshold.add_argument("--flag", dest="threshold", action="store_const", const="flag", help=form(HELP_FLAG))
        threshold.add_argument("--off", dest="threshold", action="store_const", const="off", help=form(HELP_LOG_OFF))
        parser.add_argument("-m", "--monochrome", action="store_true", default=None, help=form(HELP_MONOCHROME, DEFAULT_MONOCHROME))
        parser.add_argument("-dbu", "--dont-buffer", dest="buffered", action="store_false", default=None, help=form(HELP_DONT_BUFFER, not DEFAULT_BUFFERED))
        parser.add_argument("-hct", "--hide-clock-time", dest="show_clock_time", action="store_false", default=None, help=form(HELP_HIDE_CLOCK_TIME, not DEFAULT_SHOW_CLOCK_TIME))
        parser.add_argument("-het", "--hide-elapsed-time", dest="show_elapsed_time", action="store_false", default=None, help=form(HELP_HIDE_ELAPSED_TIME, not DEFAULT_SHOW_ELAPSED_TIME))
        parser.add_argument("-hfi", "--hide-filename", dest="show_filename", action="store_false", default=None, help=form(HELP_HIDE_FILENAME, not DEFAULT_SHOW_FILENAME))
        parser.add_argument("-hln", "--hide-lineno", dest="show_lineno", action="store_false", default=None, help=form(HELP_HIDE_LINENO, not DEFAULT_SHOW_LINENO))
        parser.add_argument("-hfn", "--hide-function", dest="show_function", action="store_false", default=None, help=form(HELP_HIDE_FUNCTION, not DEFAULT_SHOW_FUNCTION))
        parser.add_argument("-hth", "--hide-thread", dest="show_thread", action="store_false", default=None, help=form(HELP_HIDE_THREAD, not DEFAULT_SHOW_THREAD))
        parser.add_argument("-hll", "--hide-log-level", dest="show_log_level", action="store_false", default=None, help=form(HELP_HIDE_LOG_LEVEL, not DEFAULT_SHOW_LOG_LEVEL))
        parser.add_argument("-hall", "--hide-all", action="store_true", default=False, help=form(HELP_HIDE_ALL, DEFAULT_HIDE_ALL))

        return parser.parse_args()


class Service:
    def __init__(self, addr=None, port=None, wdir=None, file=None, start=None, kill=None, logger=None, dont_connect=False):
        # read command-line arguments
        self.cla = CommandLineArguments()
        # configure service
        self.addr  = helper.override(DEFAULT_ADDR,  addr,  self.cla.addr)
        self.port  = helper.override(DEFAULT_PORT,  port,  self.cla.port)
        self.wdir  = helper.override(DEFAULT_WDIR,  wdir,  self.cla.wdir)
        self.file  = helper.override(DEFAULT_FILE,  file,  self.cla.file)
        self.start = helper.override(DEFAULT_START, start, self.cla.start)
        self.kill  = helper.override(DEFAULT_KILL,  kill,  self.cla.kill)
        # change working dir
        os.chdir(self.wdir)
        # register logger
        self.register_logger(logger)
        # connect
        if not dont_connect:
            self.connect()

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, exception_traceback):
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
                      w.show_clock_time = w.show_elapsed_time = w.show_filename = w.show_lineno = w.show_function = w.show_thread = w.show_log_level = False
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
        self.info(">>> [Connect to Service] ---------------- BEGIN ----------------------------------------------------")
        self.print_working_dir()
        self.print_system_info()
        self.print_config()
        if self.addr == "127.0.0.1":
            self.connect_to_local_service()
        else:
            self.connect_to_remote_service()
        self.info(">>> [Connect to Service] ---------------- END ------------------------------------------------------")

    def print_working_dir(self):
        self.print_header("working directory")
        self.debug("{:22}{}".format("Working Directory", os.getcwd()))

    def print_system_info(self):
        self.print_header("system info")
        self.debug("{:22}{}".format("UTC Time", time.strftime("%Y-%m-%d %H:%M:%S %Z", time.gmtime())))
        self.debug("{:22}{}".format("Local Time", time.strftime("%Y-%m-%d %H:%M:%S %Z", time.localtime())))
        self.debug("{:22}{}".format("Platform", platform.platform()))

    def print_config(self):
        self.print_header("service configuration")
        # print service config
        self.print_formatted_config("-a: addr", HELP_ADDR,  self.addr,  DEFAULT_ADDR)
        self.print_formatted_config("-o: port", HELP_PORT,  self.port,  DEFAULT_PORT)
        self.print_formatted_config("-w: wdir", HELP_WDIR,  self.wdir,  DEFAULT_WDIR)
        self.print_formatted_config("-f: file", HELP_FILE,  self.file,  DEFAULT_FILE)
        self.print_formatted_config("-s: start",HELP_START, self.start, DEFAULT_START)
        self.print_formatted_config("-k: kill", HELP_KILL,  self.kill,  DEFAULT_KILL)
        # print stdout logger config
        name = str(self.threshold)
        ival = str(int(self.threshold))
        text = "{} ({})".format(name, ival) if name != ival else "{}".format(name)
        self.print_formatted_config("-l: log-level", HELP_LOG_LEVEL, text)
        self.print_formatted_config("-m: monochrome", HELP_MONOCHROME, self.monochrome, DEFAULT_MONOCHROME)
        self.print_formatted_config("-dbu: dont-buffer", HELP_DONT_BUFFER, not self.buffered, not DEFAULT_BUFFERED)

    def connect_to_local_service(self):
        self.print_header("connect to local service")
        pid_list = self.get_local_services()
        if self.kill:
            self.kill_local_services(pid_list)
            pid_list.clear()
        if pid_list and not self.start:
            self.connect_to_existing_local_service(pid_list[0])
        else:
            self.start_local_service()

    # TO DO IN FUTURE
    def connect_to_remote_service(self):
        self.print_header("connect to remote service")
        self.warn("WARNING: File setting (file={}) is ignored.".format(helper.squeeze(self.file, maxlen=30)))
        if self.start:
            self.warn("WARNING: Setting to always start a new launcher service (start={}) is ignored.".format(self.start))
        if self.kill:
            self.warn("WARNING: Setting to kill existing launcher services (kill={}) is ignored.".format(self.kill))
        msg = "Connecting to a remote service is a feature in future."
        self.fatal("FATAL: {}".format(msg))
        raise LauncherServiceError(msg)

    def print_header(self, text, level: typing.Union[int, str, LogLevel]="debug", buffer=False):
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
        self.log(helper.pad(colorize(text), total=100, left=20, char=fillchar, sep=" ", textlen=len(text)), level=level, buffer=buffer)

    def print_formatted_config(self, label, help, value, default_value=None):
        different = value is not None and value != default_value
        squeezed = helper.squeeze(str(value if value is not None else default_value), maxlen=30)
        highlighted = color.blue(squeezed) if different else squeezed
        self.debug("{:31}{:48}{}".format(color.yellow(label), help, highlighted))

    def get_local_services(self) -> typing.List[int]:
        """Returns a list of 0, 1, or more process IDs"""
        pid_list = helper.get_pid_list_by_pattern(PROGRAM)
        if len(pid_list) == 0:
            self.debug(color.yellow("No launcher is running currently."))
        elif len(pid_list) == 1:
            self.debug(color.green("Launcher service is running with process ID [{}].".format(pid_list[0])))
        else:
            self.debug(color.green("Multiple launcher services are running with process IDs {}".format(pid_list)))
        return pid_list

    def kill_local_services(self, pid_list):
        for x in pid_list:
            self.debug(color.yellow("Killing exisiting launcher service with process ID [{}].".format(x)))
            helper.terminate(x)

    def connect_to_existing_local_service(self, pid):
        cmd_and_args = helper.get_cmd_and_args_by_pid(pid)
        for ind, val in enumerate(shlex.split(cmd_and_args)):
            if ind == 0:
                existing_file = val
            elif val.startswith("--http-server-address"):
                existing_port = int(val.split(":")[-1])
                break
        else:
            self.error("ERROR: Failed to extract \"--http-server-address\" from \"{}\" (process ID {})!".format(cmd_and_args, pid))
        self.debug(color.green("Connecting to existing launcher service with process ID [{}].".format(pid)))
        self.debug(color.green("No new launcher service will be started."))
        self.debug("Configuration of existing launcher service:")
        self.debug("--- Listening port: [{}]".format(color.blue(existing_port)))
        self.debug("--- Path to file: [{}]".format(color.blue(existing_file)))
        if self.port != existing_port:
            self.warn("WARNING: Port setting (port={}) is ignored.".format(self.port))
            self.port = existing_port
        if self.file != existing_file:
            self.warn("WARNING: File setting (file={}) is ignored.".format(self.file))
            self.file = existing_file
        self.debug("To always start a new launcher service, pass {} or {}.".format(color.yellow("-s"), color.yellow("--start")))
        self.debug("To kill existing launcher services, pass {} or {}.".format(color.yellow("-k"), color.yellow("--kill")))

    def start_local_service(self):
        self.debug(color.green("Starting a new launcher service."))
        with open(PROGRAM_LOG, "w") as f:
            pass
        os.system("{} --http-server-address=0.0.0.0:{} --http-threads=4 >{} 2>&1 &".format(self.file, self.port, PROGRAM_LOG))
        time.sleep(1)
        with open(PROGRAM_LOG, "r") as f:
            msg = ""
            for line in f:
                if line.startswith("error"):
                    msg = line[line.find("]")+2:]
                    self.error("ERROR: {}".format(msg))
            if msg:
                raise LauncherServiceError(msg)
        if not self.get_local_services():
            msg = "ERROR: Launcher service is not started properly!"
            self.error(msg)
            raise LauncherServiceError(msg)


class Cluster:
    def __init__(self,
                 service,
                 contracts_dir=None,
                 cluster_id=None,
                 node_count=None,
                 pnode_count=None,
                 producer_count=None,
                 unstarted_count=None,
                 shape=None,
                 center_node_id=None,
                 extra_configs: typing.List[str]=None,
                 extra_args: str=None,
                 launch_mode=None,
                 dont_newaccount=None,
                 dont_setprod=None,
                 dont_vote=None,
                 total_supply=None,
                 http_retry=None,
                 http_sleep=None,
                 verify_async=None,
                 verify_retry=None,
                 verify_sleep=None,
                 sync_retry=None,
                 sync_sleep=None,
                 producer_retry=None,
                 producer_sleep=None):
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
        self.print_header = service.print_header
        self.print_formatted_config= service.print_formatted_config
        self.verify_threads = []

        # configure cluster
        self.contracts_dir   = helper.override(DEFAULT_CONTRACTS_DIR,   contracts_dir,   self.cla.contracts_dir)
        self.cluster_id      = helper.override(DEFAULT_CLUSTER_ID,      cluster_id,      self.cla.cluster_id)
        self.node_count      = helper.override(DEFAULT_NODE_COUNT,      node_count,      self.cla.node_count)
        self.pnode_count     = helper.override(DEFAULT_PNODE_COUNT,     pnode_count,     self.cla.pnode_count)
        self.producer_count  = helper.override(DEFAULT_PRODUDCER_COUNT, producer_count,  self.cla.producer_count)
        self.unstarted_count = helper.override(DEFAULT_UNSTARTED_COUNT, unstarted_count, self.cla.unstarted_count)
        self.shape           = helper.override(DEFAULT_SHAPE,           shape,           self.cla.shape)
        self.center_node_id  = helper.override(DEFAULT_CENTER_NODE_ID,  center_node_id,  self.cla.center_node_id)
        self.extra_configs   = helper.override(DEFAULT_EXTRA_CONFIGS,   extra_configs)
        self.extra_args      = helper.override(DEFAULT_EXTRA_ARGS,      extra_args)
        self.total_supply    = helper.override(DEFAULT_TOTAL_SUPPLY,    total_supply,    self.cla.total_supply)
        self.launch_mode     = helper.override(DEFAULT_LAUNCH_MODE,     launch_mode,     self.cla.launch_mode)
        self.dont_newaccount = helper.override(DEFAULT_DONT_NEWACCOUNT, dont_newaccount, self.cla.dont_newaccount)
        self.dont_setprod    = helper.override(DEFAULT_DONT_SETPROD,    dont_setprod,    self.cla.dont_setprod)
        self.dont_vote       = helper.override(DEFAULT_DONT_VOTE,       dont_vote,       self.cla.dont_vote)
        self.http_retry      = helper.override(DEFAULT_HTTP_RETRY,      http_retry,      self.cla.http_retry)
        self.http_sleep      = helper.override(DEFAULT_HTTP_SLEEP,      http_sleep,      self.cla.http_sleep)
        self.verify_async    = helper.override(DEFAULT_VERIFY_ASYNC,    verify_async,    self.cla.verify_async)
        self.verify_retry    = helper.override(DEFAULT_VERIFY_RETRY,    verify_retry,    self.cla.verify_retry)
        self.verify_sleep    = helper.override(DEFAULT_VERIFY_SLEEP,    verify_sleep,    self.cla.verify_sleep)
        self.sync_retry      = helper.override(DEFAULT_SYNC_RETRY,      sync_retry,      self.cla.sync_retry)
        self.sync_sleep      = helper.override(DEFAULT_SYNC_SLEEP,      sync_sleep,      self.cla.sync_sleep)
        self.producer_retry  = helper.override(DEFAULT_PRODUCER_RETRY,  producer_retry,  self.cla.producer_retry)
        self.producer_sleep  = helper.override(DEFAULT_PRODUCER_SLEEP,  producer_sleep,  self.cla.producer_sleep)

        # check for logical errors in config
        self.check_config()

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

        # establish mappings between nodes and producers
        self.nodes = []
        self.producers = []
        self.producer_to_node = {}
        self.node_to_producers = {}
        q, r = divmod(self.producer_count, self.pnode_count)
        for i in range(self.pnode_count):
            self.nodes += [[i]]
            prod = []
            for j in range(i * q + r if i else 0, (i + 1) * q + r):
                name = self.get_defproducer_name(j)
                prod.append(name)
                self.producer_to_node[name] = i
            self.nodes[i] += [{"producers": (prod if i else ["eosio"] + prod)}]
            self.producers += prod
            self.node_to_producers[i] = prod


        # launch cluster
        if self.launch_mode == "bios":
            self.bios_launch(dont_newaccount=self.dont_newaccount, dont_setprod=self.dont_setprod)
        elif self.launch_mode == "regular":
            self.regular_launch(dont_newaccount=self.dont_newaccount, dont_vote=self.dont_vote)
        else:
            raise ValueError(f"Unknown launch_mode {self.launch_mode}")


    def __enter__(self):
        return self


    def __exit__(self, exception_type, exception_value, exception_traceback):
        self.flush()


    def check_config(self):
        bassert(self.cluster_id >= 0, f"Invalid cluster_id ({self.cluster_id}). Valid range is [0, 29].")
        bassert(self.node_count >= self.pnode_count + self.unstarted_count,
                f"node_count ({self.node_count}) must be greater than "
                f"pnode_count ({self.pnode_count}) + unstarted_count ({self.unstarted_count}).")
        if self.shape in ("star", "bridge"):
            bassert(self.center_node_id is not None, f"center_node_id is not specified when shape is \"{self.shape}\".")
        if self.shape == "bridge":
            bassert(self.center_node_id not in (0, self.node_count-1),
                    f"center_node_id ({self.center_node_id}) cannot be 0 or {self.node_count-1} "
                    f"when shape is \"bridge\" and node_count is {self.node_count}.")


    def bios_launch(self, dont_newaccount=False, dont_setprod=False):
        """
        Launch without Bootstrap
        ---------
        0. print config
        1. launch a cluster
        2. get cluster info
        3. schedule protocol feature activations
        4. set eosio.bios contract ---> for setprod, not required for newaccount
        5. bios-create accounts
        6. set producers
        7. check if nodes are in sync
        """
        self.info(">>> [BIOS Launch] ----------------------- BEGIN ----------------------------------------------------")
        self.print_config()
        self.launch_cluster()
        bassert(self.wait_nodes_ready())
        self.schedule_protocol_feature_activations()
        self.set_bios_contract()
        if not dont_newaccount:
            self.bios_create_accounts_in_parallel(accounts=self.producers)
            if not dont_setprod:
                self.set_producers()
        self.check_sync()
        for t in self.verify_threads:
            t.join()
        self.info(">>> [BIOS Launch] ----------------------- END ------------------------------------------------------")


    def regular_launch(self, dont_newaccount=False, dont_vote=False):
        """
        Bootstrap
        ---------
        0. print config
        1. launch a cluster
        2. get cluster info
        3. schedule protocol feature activations
        4. bios-create eosio.token, eosio.system accounts
        5. set eosio.token contract    <--- depends on 4
        6. create tokens               <--- depends on 5
        7. issue tokens                <--- depends on 5
        8. set eosio.system contract   <--- depends on 4
        9. init eosio.system contract  <--- depends on 6,7
        10. create producer accounts   <--- depends on 8,9
        11. register producers         <--- depends on 8,9
        12. vote for producers         <--- depends on 8,9
        13. check if nodes are in sync
        14. verify production schedule change
        """
        self.info(">>> [Regular Launch] -------------------- BEGIN ------------------------------------------")
        self.print_config()
        self.launch_cluster()
        self.get_cluster_info(level="debug", response_text_level="debug")
        self.schedule_protocol_feature_activations()
        self.bios_create_accounts_in_parallel(accounts=["eosio.bpay",
                                            "eosio.msig",
                                            "eosio.names",
                                            "eosio.ram",
                                            "eosio.ramfee",
                                            "eosio.rex",
                                            "eosio.saving",
                                            "eosio.stake",
                                            "eosio.token",
                                            "eosio.upay"])
        self.set_token_contract()
        self.create_tokens(maximum_supply=self.total_supply)
        self.issue_tokens(quantity=self.total_supply)
        self.set_system_contract()
        self.init_system_contract()
        if not dont_newaccount:
            self.create_and_register_producers_in_parallel()
            if not dont_vote:
                self.vote_for_producers(voter="defproducera", voted_producers=list(self.producer_to_node.keys())[:min(21, len(self.producer_to_node))])
                self.check_head_block_producer()
        self.check_sync()
        for t in self.verify_threads:
            t.join()
        self.info(">>> [Regular Launch] -------------------- END --------------------------------------------")


    def print_config(self):
        self.print_header("cluster configuration")
        self.print_formatted_config("-d: contracts_dir",   HELP_CONTRACTS_DIR,   self.contracts_dir,    DEFAULT_CONTRACTS_DIR)
        self.print_formatted_config("-i: cluster_id",      HELP_CLUSTER_ID,      self.cluster_id,       DEFAULT_CLUSTER_ID)
        self.print_formatted_config("-n: node_count",      HELP_NODE_COUNT,      self.node_count,       DEFAULT_NODE_COUNT)
        self.print_formatted_config("-m: pnode_count",     HELP_PNODE_COUNT,     self.pnode_count,      DEFAULT_PNODE_COUNT)
        self.print_formatted_config("-p: producer_count",  HELP_PRODUDCER_COUNT, self.producer_count,   DEFAULT_PRODUDCER_COUNT)
        self.print_formatted_config("-u: unstarted_count", HELP_UNSTARTED_COUNT, self.unstarted_count,  DEFAULT_UNSTARTED_COUNT)
        self.print_formatted_config("-t: shape",           HELP_SHAPE,           self.shape,            DEFAULT_SHAPE)
        self.print_formatted_config("-c: center_node_id",  HELP_CENTER_NODE_ID,  self.center_node_id,   DEFAULT_CENTER_NODE_ID)
        self.print_formatted_config("... extra_configs",   HELP_EXTRA_CONFIGS,   self.extra_configs,    DEFAULT_EXTRA_CONFIGS)
        self.print_formatted_config("-r: regular_launch",  HELP_REGULAR_LAUNCH,  self.launch_mode == "regular", DEFAULT_REGULAR_LAUNCH)
        self.print_formatted_config("-xn: dont_newaccount",HELP_DONT_NEWACCOUNT, self.dont_newaccount,  DEFAULT_DONT_NEWACCOUNT)
        self.print_formatted_config("-xs: dont_setprod",   HELP_DONT_SETPROD,    self.dont_setprod,     DEFAULT_DONT_SETPROD)
        self.print_formatted_config("-xv: dont_vote",      HELP_DONT_VOTE,       self.dont_vote,        DEFAULT_DONT_VOTE)
        self.print_formatted_config("--http-retry",        HELP_HTTP_RETRY,      self.http_retry,       DEFAULT_HTTP_RETRY)
        self.print_formatted_config("--verify-async",      HELP_VERIFY_ASYNC,    self.verify_async,     DEFAULT_VERIFY_ASYNC)
        self.print_formatted_config("--verify-retry",      HELP_VERIFY_RETRY,    self.verify_retry,     DEFAULT_VERIFY_RETRY)
        self.print_formatted_config("--verify-sleep",      HELP_VERIFY_SLEEP,    self.verify_sleep,     DEFAULT_VERIFY_SLEEP)
        self.print_formatted_config("--sync-retry",        HELP_SYNC_RETRY,      self.sync_retry,       DEFAULT_SYNC_RETRY)
        self.print_formatted_config("--producer-retry",    HELP_PRODUCER_RETRY,  self.producer_retry,   DEFAULT_PRODUCER_RETRY)
        self.print_formatted_config("--http-sleep",        HELP_HTTP_SLEEP,      self.http_sleep,       DEFAULT_HTTP_SLEEP)
        self.print_formatted_config("--sync-sleep",        HELP_SYNC_SLEEP,      self.sync_sleep,       DEFAULT_SYNC_SLEEP)
        self.print_formatted_config("--producer-sleep",    HELP_PRODUCER_SLEEP,  self.producer_sleep,   DEFAULT_PRODUCER_SLEEP)


    """
    =====================
    cluster-related calls
    =====================
    """

    def launch_cluster(self, **call_kwargs):
        return self.call("launch_cluster",
                         nodes=self.nodes,
                         node_count=self.node_count,
                         shape=self.shape,
                         center_node_id=self.center_node_id,
                         extra_configs=self.extra_configs,
                         extra_args=self.extra_args,
                         **call_kwargs)


    def stop_cluster(self, **call_kwargs):
        return self.call("stop_cluster", **call_kwargs)


    def clean_cluster(self, **call_kwargs):
        return self.call("clean_cluster")


    def start_node(self, node_id, extra_args=None, **call_kwargs):
        return self.call("start_node", node_id=node_id, extra_args=extra_args, **call_kwargs)


    def stop_node(self, node_id, kill_sig=15, **call_kwargs):
        """kill_sig: 15 for soft kill, 9 for hard kill"""
        return self.call("stop_node", node_id=node_id, kill_sig=kill_sig, **call_kwargs)


    def stop_all_nodes(self, kill_sig=15, **call_kwargs):
        for node_id in self.node_count:
            return self.call("stop_nodes", node_id=node_id, kill_sig=kill_sig, **call_kwargs)


    """
    =====================
    queries
    =====================
    """
    def get_cluster_info(self, **call_kwargs):
        return self.call("get_cluster_info", **call_kwargs)


    def get_cluster_running_state(self, **call_kwargs):
        return self.call("get_cluster_running_state", **call_kwargs)


    def get_info(self, node_id, **call_kwargs):
        return self.call("get_info", node_id=node_id, **call_kwargs)


    def get_block(self, block_num_or_id, node_id=0, **call_kwargs):
        return self.call("get_block", node_id=node_id, block_num_or_id=block_num_or_id, **call_kwargs)


    def get_account(self, name, node_id=0, **call_kwargs):
        return call("get_account", node_id=node_id, name=name, **call_kwargs)


    def get_protocol_features(self, node_id=0, **call_kwargs):
        return call("get_protocol_features", node_id=node_id, **call_kwargs)


    def get_log(self, node_id) -> str:
        log = ""
        offset = 0
        length = 10000
        while True:
            response = self.get_log_data(node_id=node_id, offset=offset, length=length).response_dict
            log += base64.b64decode(response["data"]).decode("utf-8")
            if response["offset"] + length > response["filesize"]:
                break
            offset += length
        return log


    def get_log_data(self, node_id, offset, length=10000, filename="stderr_0.txt", **call_kwargs):
        return self.call("get_log_data",
                         node_id=node_id,
                         offset=offset,
                         length=length,
                         filename=filename)


    def verify(self,
               transaction_id,
               verify_key=None,
               verify_async=False,
               retry=None,
               sleep=None,
               assertive=True,
               level=None,
               retry_level=None,
               error_level=None,
               buffer=None):
        verify_async = helper.override(self.verify_async, verify_async, self.cla.verify_async)
        retry = helper.override(self.verify_retry, retry, self.cla.verify_retry)
        sleep = helper.override(self.verify_sleep, sleep, self.cla.verify_sleep)
        verified = False
        while retry >= 0:
            if self.verify_transaction(transaction_id=transaction_id, verify_key=verify_key, level=retry_level, buffer=buffer):
                verified = True
                break
            time.sleep(sleep)
            retry -= 1
        if verify_async:
            self.log(helper.make_header("async verify transaction", level=level), level=level, buffer=True)
            if verified:
                self.log(color.black_on_green(f"{verify_key.title()}") + f" {transaction_id}", level=level, buffer=True)
            else:
                self.log(color.black_on_red(f"Not {verify_key.title()}") + f" {transaction_id}", level=error_level, buffer=True)
            self.flush()
        else:
            if verified:
                self.log(color.black_on_green(f"{verify_key.title()}"), level=level)
            else:
                self.log(color.black_on_red(f"Not {verify_key.title()}"), level=error_level)
        if assertive:
            bassert(verified, f"{transaction_id} cannot be verified")
        return verified


    def verify_transaction(self, transaction_id, verify_key=None, **call_kwargs):
        bassert(verify_key in ["irreversible", "contained"], "Verify key must be either \"irreversible\" or \"contained\".")
        cx = self.call("verify_transaction", transaction_id=transaction_id, verify_key=None, dont_flush=True, **call_kwargs)
        return helper.extract(cx.response, key=verify_key, fallback=False)


    """
    =====================
    transactions
    =====================
    """
    def schedule_protocol_feature_activations(self, **call_kwargs):
        return self.call("schedule_protocol_feature_activations",
                         protocol_features_to_activate=[PREACTIVATE_FEATURE],
                         **call_kwargs)


    def set_contract(self,
                     account,
                     contract_file,
                     abi_file,
                     node_id=0,
                     verify_key="irreversible",
                     name=None,
                     **call_kwargs):
        return self.call("set_contract",
                         node_id=node_id,
                         account=account,
                         contract_file=contract_file,
                         abi_file=abi_file,
                         verify_key=verify_key,
                         header=f"set <{name}> contract" if name else None,
                         **call_kwargs)


    def push_actions(self,
                     actions,
                     node_id=0,
                     verify_key="irreversible",
                     **call_kwargs):
        return self.call("push_actions",
                         node_id=node_id,
                         actions=actions,
                         verify_key=verify_key,
                         **call_kwargs)

    """
    =====================
    miscellaneous
    =====================
    """
    def send_raw(self,
                 url,
                 string_data: str="",
                 json_data: dict={},
                 node_id=0,
                 **call_kwargs):
        return self.call("send_raw",
                         url=url,
                         string_data=string_data,
                         json_data=json_data,
                         **call_kwargs)


    def pause_node_production(self, node_id):
        return send_raw(url="/v1/producer/pause",
                        node_id=node_id)


    def resume_node_production(self, node_id):
        return send_raw(url="/v1/producer/resume",
                        node_id=node_id)


    def get_greylist(self, node_id=0):
        return send_raw(url="/v1/producer/get_greylist",
                        node_id=node_id)


    def add_greylist_accounts(self, accounts:list, node_id=0):
        return send_raw(url="/v1/producer/add_greylist_accounts",
                        node_id=node_id,
                        json_data={"accounts": accounts})


    def remove_greylist_accounts(self, accounts:list, node_id=0):
        return send_raw(url="/v1/producer/remove_greylist_accounts",
                        node_id=node_id,
                        json_data={"accounts": accounts})



    """
    ======================
    composite
    ======================
    """
    def get_node_pid(self, node_id, **call_kwargs) ->bool:
        return self.get_cluster_running_state(**call_kwargs).response_dict["result"]["nodes"][node_id][1]["pid"]


    def is_node_down(self, node_id, **call_kwargs):
        return self.get_node_pid(node_id, **call_kwargs) == 0


    def is_node_ready(self, node_id, **call_kwargs):
        return "error" not in self.get_cluster_info(**call_kwargs).response_dict["result"][node_id][1]


    def wait_nodes_ready(self, nodes=None, retry=10, sleep=1, level="debug", sublevel="trace"):
        if nodes is None:
            nodes = list(range(self.node_count))
        self.print_header("wait for nodes to get ready", level=level)
        while True:
            result = self.get_cluster_info(level=sublevel).response_dict["result"]
            error_nodes = [x[0] for x in result if x[0] in nodes and "error" in x[1]]
            if len(error_nodes) == 0:
                self.log("All {} nodes are ready.".format(len(nodes)), level=level)
                return True
            if retry > 0:
                self.log("Nodes that are not ready: {}. {} retries remain. Sleep for {}s.".format(error_nodes, retry, sleep), level=level)
                time.sleep(sleep)
                retry -= 1
            else:
                return False


    def get_head_block_number(self, node_id=0):
        """Get head block number by node id."""
        return self.get_cluster_info().response_dict["result"][node_id][1]["head_block_num"]


    def get_head_block_producer(self, node_id=0):
        """Get head block producer by node id."""
        return self.get_cluster_info().response_dict["result"][node_id][1]["head_block_producer"]


    def get_running_nodes(self):
        cluster_result = self.get_cluster_info().response_dict["result"]
        count = 0
        for node_result in cluster_result:
            if "head_block_id" in node_result[1]:
                count += 1
        return count


    def set_bios_contract(self):
        contract = "eosio.bios"
        return self.set_contract(account="eosio",
                                 contract_file=self.get_wasm_file(contract),
                                 abi_file=self.get_abi_file(contract),
                                 name=contract)


    def set_token_contract(self):
        contract = "eosio.token"
        return self.set_contract(account=contract,
                                 contract_file=self.get_wasm_file(contract),
                                 abi_file=self.get_abi_file(contract),
                                 name=contract)


    def set_system_contract(self):
        contract = "eosio.system"
        self.set_contract(contract_file=self.get_wasm_file(contract),
                          abi_file=self.get_abi_file(contract),
                          account="eosio",
                          name=contract)


    def bios_create_accounts(self, accounts: typing.Union[str, typing.List[str]], node_id=0, verify_key="irreversible", buffer=False):
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
        return self.push_actions(actions=actions, node_id=node_id, header=header, verify_key=verify_key, buffer=buffer)


    def bios_create_accounts_in_parallel(self, accounts, verify_key="irreversible"):
        threads = []
        channel = {}
        def report(channel, thread_id, message):
            channel[thread_id] = message
        for ac in accounts:
            t = ExceptionThread(channel, report, target=self.bios_create_accounts, args=(ac,), kwargs={"buffer": True, "verify_key": verify_key})
            threads.append(t)
            t.start()
        for t in threads:
            t.join()
        if len(channel) != 0:
            self.error(f"{len(channel)} expcetions occurred in bios-creating accounts")


    def set_producers(self, producers=None, verify_key="irreversible", verify_retry=None):
        producers = self.producers if producers is None else producers
        prod_keys = []
        for p in sorted(producers):
            prod_keys.append({"producer_name": p, "block_signing_key": PRODUCER_KEY})
        actions = [{"account": "eosio",
                    "action": "setprods",
                    "permissions": [{"actor": "eosio", "permission": "active"}],
                    "data": { "schedule": prod_keys}}]
        return self.push_actions(actions=actions, header="set producers", verify_key=verify_key, verify_retry=verify_retry)


    def create_tokens(self, maximum_supply):
        formatted = helper.format_tokens(maximum_supply)
        actions = [{"account": "eosio.token",
                    "action": "create",
                    "permissions": [{"actor": "eosio.token",
                                     "permission": "active"}],
                    "data": {"issuer": "eosio",
                             "maximum_supply": formatted,
                             "can_freeze": 0,
                             "can_recall": 0,
                             "can_whitelist":0}}]
        return self.push_actions(actions=actions, header="create tokens")


    def issue_tokens(self, quantity):
        formatted = helper.format_tokens(quantity)
        actions = [{"account": "eosio.token",
                    "action": "issue",
                    "permissions": [{"actor": "eosio",
                                    "permission": "active"}],
                    "data": {"to": "eosio",
                             "quantity": formatted,
                             "memo": "hi"}}]
        return self.push_actions(actions=actions, header="issue tokens")


    def init_system_contract(self):
        actions = [{"account": "eosio",
                    "action": "init",
                    "permissions": [{"actor": "eosio",
                                     "permission": "active"}],
                    "data": {"version": 0,
                             "core": "4,SYS"}}]
        return self.push_actions(actions=actions, header="init system contract")


    def create_account(self, creator, name, stake_cpu, stake_net, buy_ram_bytes, transfer, node_id=0, verify_key="irreversible", buffer=False):
        newaccount  = {"account": "eosio",
                       "action": "newaccount",
                       "permissions": [{"actor": "eosio",
                                        "permission": "active"}],
                       "data": {"creator": "eosio",
                                "name": name,
                                "owner": {"threshold": 1,
                                          "keys": [{"key": PRODUCER_KEY,
                                                    "weight": 1}],
                                          "accounts": [],
                                          "waits": []},
                                "active": {"threshold": 1,
                                           "keys": [{"key": PRODUCER_KEY,
                                                     "weight": 1}],
                                           "accounts": [],
                                           "waits": []}}}
        buyrambytes = {"account": "eosio",
                       "action": "buyrambytes",
                       "permissions": [{"actor": "eosio",
                                        "permission": "active"}],
                       "data": {"payer": "eosio",
                                "receiver": name,
                                "bytes": buy_ram_bytes}}
        delegatebw  = {"account": "eosio",
                       "action": "delegatebw",
                       "permissions": [{"actor": "eosio",
                                        "permission": "active"}],
                       "data": {"from": "eosio",
                                "receiver": name,
                                "stake_cpu_quantity": stake_cpu,
                                "stake_net_quantity": stake_net,
                                "transfer": transfer}}
        actions = [newaccount, buyrambytes, delegatebw]
        return self.push_actions(actions=actions, header=f"create \"{name}\" account", verify_key=verify_key, buffer=buffer)


    def register_producer(self, producer, buffer=False):
        actions = [{"account": "eosio",
                    "action": "regproducer",
                    "permissions": [{"actor": "{}".format(producer),
                                    "permission": "active"}],
                    "data": {"producer": "{}".format(producer),
                             "producer_key": PRODUCER_KEY,
                             "url": "www.test.com",
                             "location": 0}}]
        return self.push_actions(actions=actions, header="register \"{}\" producer".format(producer), buffer=buffer)


    def create_and_register_producers_in_parallel(self):
        amount = self.total_supply * 0.075
        threads = []
        channel = {}
        def report(channel, thread_id, message):
            channel[thread_id] = message
        for p in self.producer_to_node:
            def create_and_register_producers(amount):
                # CAUTION
                # -------
                # Must keep p's value in a local variable (producer),
                # since p may change in multithreading
                producer = p
                formatted = helper.format_tokens(amount)
                self.create_account(creator="eosio",
                                    name=producer,
                                    stake_cpu=formatted,
                                    stake_net=formatted,
                                    buy_ram_bytes=1048576,
                                    transfer=True,
                                    buffer=True)
                self.register_producer(producer=producer, buffer=True)
            t = ExceptionThread(channel, report, target=create_and_register_producers, args=(amount,))
            amount = max(amount / 2, 100)
            threads.append(t)
            t.start()
        for t in threads:
            t.join()
        if len(channel) != 0:
            self.logger.error("{} exception(s) occurred in creating and registering producers.".format(len(channel)))
            count = 0
            for thread_id in channel:
                self.logger.error(channel[thread_id])
                count += 1
                if count == 5:
                    break


    def vote_for_producers(self, voter, voted_producers: List[str],  buffer=False):
        assert len(voted_producers) <= 30, "An account cannot votes for more than 30 producers. {} voted for {} producers.".format(voter, len(voted_producers))
        actions = [{"account": "eosio",
                    "action": "voteproducer",
                    "permissions": [{"actor": "{}".format(voter),
                                     "permission": "active"}],
                    "data": {"voter": "{}".format(voter),
                             "proxy": "",
                             "producers": voted_producers}}]
        return self.push_actions(actions=actions, header="votes for producers", buffer=buffer)


    def check_sync(self, retry=None, sleep=None, min_sync_nodes=None, max_block_lag=None, level="DEBUG", dont_raise=False):

        @dataclass
        class SyncResult:
            in_sync: bool
            sync_nodes: int
            min_block_num: int
            max_block_num: int = None

            def __post_init__(self):
                if self.in_sync:
                    self.block_num = self.max_block_num = self.min_block_num
                else:
                    assert self.min_block_num == math.inf or self.min_block_num <= self.max_block_num
                    self.block_num = -1

        # set arguments
        retry = self.sync_retry if retry is None else retry
        sleep = self.sync_sleep if sleep is None else sleep
        min_sync_nodes = min_sync_nodes if min_sync_nodes else self.node_count
        # print head
        self.print_header("check if nodes are in sync", level=level)
        while retry >= 0:
            ix = self.get_cluster_info(level="trace")
            has_head_block_id = lambda node_id: "head_block_id" in ix.response_dict["result"][node_id][1]
            extract_head_block_id = lambda node_id: ix.response_dict["result"][node_id][1]["head_block_id"]
            extract_head_block_num = lambda node_id: ix.response_dict["result"][node_id][1]["head_block_num"]
            counter = collections.defaultdict(int)
            no_head_nodes = set()
            max_block_num, min_block_num = -1, math.inf
            max_block_node = min_block_node = -1
            sync_nodes = 0
            for node_id in range(self.node_count):
                if has_head_block_id(node_id):
                    block_num = extract_head_block_num(node_id)
                    if block_num > max_block_num:
                        max_block_num, max_block_node = block_num, node_id
                    elif block_num < min_block_num:
                        min_block_num, min_block_node = block_num, node_id
                    head_id = extract_head_block_id(node_id)
                    counter[head_id] += 1
                    if counter[head_id] > sync_nodes:
                        sync_nodes, sync_block, sync_head = counter[head_id], block_num, head_id
                else:
                    no_head_nodes.add(node_id)
            down = f" ({len(no_head_nodes)} nodes down)" if len(no_head_nodes) else ""
            self.log(f"{sync_nodes}/{self.node_count} nodes in sync{down}: max block number {max_block_num} from node {max_block_node} │ min block number {min_block_num} from node {min_block_node}", level=level)
            if sync_nodes >= min_sync_nodes:
                self.log(color.green(f"<Num of Nodes In Sync> {sync_nodes}"), level=level)
                self.log(color.green(f"<Head Block Num> {sync_block}"), level=level)
                self.log(color.green(f"<Head Block ID> {sync_head}"), level=level)
                self.log(color.black_on_green("Nodes In Sync"), level=level)
                return SyncResult(True, sync_nodes, min_block_num)
            if max_block_lag and max_block_num - min_block_num > max_block_lag:
                self.log(f"Gap between max and min block numbers ({max_block_num - min_block_num}) is larger than tolerance ({max_block_lag}).", level=level)
                break
            time.sleep(sleep)
            retry -= 1
        msg = "Nodes Out of Sync"
        if not dont_raise:
            self.error(color.black_on_red(msg))
            raise SyncError(msg)
        else:
            self.log(color.black_on_yellow(msg), level=level)
        return SyncResult(False, sync_nodes, min_block_num, max_block_num)


    def check_production_round(self, expected_producers: List[str], level="debug"):
        self.print_header("verify production round", level=level)

        # list expected producers
        self.log("Expected producers:", level=level)
        for i, v in enumerate(expected_producers):
            self.logger.log(f"[{i}] {v}", level=level)

        # skip unexpected producers
        begin_block_num = self.get_head_block_number()
        curr_prod = self.wait_get_producer_by_block(begin_block_num)
        while curr_prod not in expected_producers:
            self.logger.log(f"Block # {begin_block_num}: {curr_prod} is not among expected producers. "
                             "Waiting for schedule change.", level=level)
            begin_block_num += 1
            curr_prod = self.wait_get_producer_by_block(begin_block_num)

        # verification
        self.log(f">>> Verification starts, as expected producer \"{curr_prod}\" has come to produce.", level=level)
        self.log(f"Block # {begin_block_num}: {curr_prod} has produced 1 blocks in this round. "
                 f"{12 * len(expected_producers) - 1} blocks remain to verify.", level=level)
        counter = {x: 0 for x in expected_producers}
        counter[curr_prod] += 1
        entr_prod = last_prod = curr_prod
        end_block_num = begin_block_num + 12 * len(expected_producers)
        for num in range(begin_block_num + 1, end_block_num):
            curr_prod = self.wait_get_producer_by_block(num)
            counter[curr_prod] += 1
            if curr_prod not in expected_producers:
                msg = f"Unexpected producer \"{curr_prod}\" in block #{num}."
                self.error(msg)
                raise BlockchainError(msg)
            if curr_prod != last_prod and last_prod != entr_prod and counter[last_prod] != 12:
                msg = (f"Producer changes to \"{curr_prod}\" after last producer \"{last_prod}\" "
                       f"produces {counter[last_prod]} blocks.")
                self.error(msg)
                raise BlockchainError(msg)
            self.logger.log(f"Block # {num}: {curr_prod} has produced {counter[curr_prod]} blocks in this round. "
                            f"{end_block_num - num - 1} blocks remain to verify.", level=level)
            last_prod = curr_prod

        # conclusion
        expected_counter = {x: 12 for x in expected_producers}
        if counter == expected_counter:
            self.logger.log(">>> Verification succeeded.", level=level)
            return True
        else:
            msg = ">>> Verification failed."
            for prod in counter:
                msg += f"\n{prod} produced {counter[prod]} blocks."
            self.error(msg)
            raise BlockchainError(msg)
            return False

    def wait_get_block(self, block_num, retry=5) -> dict:
        """Get block information by block num. If that block has not been produced, wait for it."""
        while retry >= 0:
            head_block_num = self.get_head_block_number()
            if head_block_num < block_num:
                time.sleep(0.5 * (block_num - head_block_num))
            else:
                return self.get_block(block_num).response_dict
            retry -= 1
        msg = f"Cannot get block # {block_num}. Current head block num is {head_block_num}."
        self.error(msg)
        raise BlockchainError(msg)


    def wait_get_producer_by_block(self, block_num, retry=5) -> str:
        """Get block producer by block num. If that block has not been produced, wait for it."""
        return self.wait_get_block(block_num, retry=retry)["producer"]


    def check_head_block_producer(self, retry=None, sleep=None):
        retry = self.producer_retry if retry is None else retry
        sleep = self.producer_sleep if sleep is None else sleep
        self.print_header("get head block producer")
        cx = self.get_cluster_info(level="TRACE")
        extract_head_block_producer = lambda cx: cx.response_dict["result"][0][1]["head_block_producer"]
        while retry >= 0:
            cx = self.get_cluster_info(level="TRACE")
            head_block_producer = extract_head_block_producer(cx)
            if head_block_producer == "eosio":
                self.logger.debug(color.yellow("Head block producer is still \"eosio\"."))
            else:
                self.logger.debug(color.green("Head block producer is now \"{}\", no longer eosio.".format(head_block_producer)))
                break
            self.logger.trace("{} {} for head block producer verification...".format(retry, "retries remain" if retry > 1 else "retry remains"))
            self.logger.trace("Sleep for {}s before next retry...".format(sleep))
            time.sleep(sleep)
            retry -= 1
        assert head_block_producer != "eosio", "Head block producer is still \"eosio\"."


    def get_wasm_file(self, contract):
        return os.path.join(self.contracts_dir, contract, contract + ".wasm")


    def get_abi_file(self, contract):
        return os.path.join(self.contracts_dir, contract, contract + ".abi")


    @staticmethod
    def get_defproducer_name(num):

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




    """
    ====================
    Launcher Service API
    ====================
    @ plugins/launcher_service_plugin/include/eosio/launcher_service_plugin/launcher_service_plugin.hpp

    -- cluster-related calls ----------------
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

    def call(self,
             api: str,
             http_retry=None,
             http_sleep=None,
             verify_async=None,
             verify_key=None,
             verify_sleep=None,
             verify_retry=None,
             verify_assert=True,
             header=None,
             level=None,
             header_level=None,
             url_level=None,
             request_text_level=None,
             retry_info_level=None,
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
        """
        call
        ----
        1. print header
        2. establish connection
        3. log url and request of connection
        4. retry connection if response not ok
        5. log response
        6. verify transaction
        """
        http_retry = helper.override(self.http_retry, http_retry, self.cla.http_retry)
        http_sleep = helper.override(self.http_sleep, http_sleep, self.cla.http_sleep)
        header = helper.override(api.replace("_", " "), header)
        data.setdefault("cluster_id", self.cluster_id)
        data.setdefault("node_id", 0)

        verify_async = helper.override(self.verify_async, verify_async, self.cla.verify_async)

        level = helper.override("debug", level)
        header_level = helper.override(level, header_level)
        url_level = helper.override(level, url_level)
        request_text_level = helper.override(level, request_text_level)
        retry_info_level = helper.override("trace", retry_info_level)
        retry_text_level = helper.override("trace", retry_text_level)
        response_code_level = helper.override(level, response_code_level)
        response_text_level = helper.override("trace", response_text_level)
        transaction_id_level = helper.override(level, transaction_id_level)
        no_transaction_id_level = helper.override(transaction_id_level, no_transaction_id_level)
        error_level = helper.override("error", error_level)
        error_text_level = helper.override(error_level, error_text_level)
        verify_level = helper.override(level, verify_level)
        verify_retry_level = helper.override(retry_info_level, verify_retry_level)
        verify_error_level = helper.override(error_level, verify_error_level)

        # self.log(helper.make_header(header, level=header_level), level=header_level, buffer=buffer)
        self.print_header(header, level=header_level, buffer=buffer)
        cx = Connection(url=f"http://{self.service.addr}:{self.service.port}/v1/launcher/{api}", data=data)
        self.log(cx.url, level=url_level, buffer=buffer)
        self.log(cx.request_text, level=request_text_level, buffer=buffer)
        while not cx.ok and http_retry > 0:
            self.log(cx.response_code, level=retry_info_level, buffer=buffer)
            self.log(cx.response_text, level=retry_text_level, buffer=buffer)
            self.log(f"{http_retry} retries remain for http connection...", level=retry_info_level, buffer=buffer)
            self.log(f"Sleep for {http_sleep}s before next retry...", level=retry_info_level, buffer=buffer)
            time.sleep(http_sleep)
            cx.attempt()
            http_retry -= 1
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
        if verify_key:
            if verify_async:
                t = threading.Thread(target=self.verify,
                                     kwargs={"transaction_id": cx.transaction_id,
                                             "verify_key": verify_key,
                                             "verify_async": True,
                                             "retry": verify_retry,
                                             "sleep":verify_sleep,
                                             "assertive": verify_assert,
                                             "level": verify_level,
                                             "retry_level": verify_retry_level,
                                             "error_level": verify_error_level,
                                             "buffer": True})
                t.start()
                self.verify_threads.append(t)
            else:
                self.verify(transaction_id=cx.transaction_id,
                            verify_key=verify_key,
                            verify_async=False,
                            retry=verify_retry,
                            sleep=verify_sleep,
                            assertive=verify_assert,
                            level=verify_level,
                            retry_level=verify_retry_level,
                            error_level=verify_error_level,
                            buffer=buffer)
        if buffer and not dont_flush:
            self.flush()
        return cx



def main():
    logger = Logger(ScreenWriter(threshold="debug"),
                    FileWriter(filename="service-info.log", threshold="info", monochrome=True),
                    FileWriter(filename="service-debug.log", threshold="debug", monochrome=True),
                    FileWriter(filename="service-trace.log", threshold="trace", monochrome=True))
    service = Service(logger=logger)
    cluster = Cluster(service=service)


if __name__ == "__main__":
    main()




    # def bios_create_accounts(self, name, verify_key="irreversible", buffer=False):
    #     actions = [{"account": "eosio",
    #                 "action": "newaccount",
    #                 "permissions":[{"actor": "eosio",
    #                                 "permission": "active"}],
    #                 "data":{"creator": "eosio",
    #                         "name": name,
    #                         "owner": {"threshold": 1,
    #                                   "keys": [{"key": PRODUCER_KEY,
    #                                             "weight":1}],
    #                                   "accounts": [],
    #                                   "waits": []},
    #                         "active":{"threshold": 1,
    #                                   "keys": [{"key": PRODUCER_KEY,
    #                                             "weight":1}],
    #                                   "accounts": [],
    #                                   "waits": []}}}]
    #     return self.push_actions(actions=actions, header=f"bios create \"{name}\" account", verify_key=verify_key, buffer=buffer)


    # def are_all_nodes_ready(self, node_id_list=None, **call_kwargs):
    #     node_id_list = helper.override(list(range(self.node_count)), node_id_list)
    #     result = self.get_cluster_info(**call_kwargs).response_dict["result"]
    #     error_nodes = [x[0] for x in result if x[0] in node_id_list and "error" in x[1]]
    #     return len(error_nodes) == 0, error_nodes

