#!/usr/bin/env python3
"""Memory leak tests for yaview on macOS.

Each scenario starts a separate yaview in a throwaway home folder, drives it through
`--execute-command` like a user would (opening and closing tabs, popups, searching, theme
changes, ...), and repeats one iteration of the scenario many times. After a warm-up, Apple's
`leaks` tool saves a baseline memory graph; after the repetitions it reports only the leaks that
are new since that baseline, and `footprint` measures how much physical memory grew. Memory that
leaks on every repetition shows up as new leaks that grow with the number of repetitions; one-off
leaks in system frameworks do not.

See README.md in this folder for usage and for how to read the results.
"""

import argparse
import json
import os
import plistlib
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import uuid
from dataclasses import dataclass, field
from typing import List, Optional

REPO_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_APP = os.path.join(REPO_DIR, "build", "local-macos", "yaview.app")
DOCUMENT_COUNT = 6


@dataclass
class Step:
    # A macro: one or more yaview commands separated by ';'. Ignored when open_document is set.
    command: str = ""
    # Passed as --execute-command-data, for commands that take text (search, setconfig_*).
    data: Optional[str] = None
    # Open test document number N (0 is the document yaview starts with) instead of running a command.
    open_document: Optional[int] = None
    # Wait until yaview has run the step. Popups never answer (they wait for input), so steps that
    # open one pass wait=False and pause briefly instead. Opening a document never answers either;
    # those steps are followed by a waited noop instead.
    wait: bool = True
    pause: float = 0.0


@dataclass
class Scenario:
    name: str
    description: str
    iteration: List[Step]
    setup: List[Step] = field(default_factory=list)


def open_docs(numbers):
    return [Step(open_document=n) for n in numbers]


SCENARIOS = [
    Scenario(
        "tabs_open_close",
        "Open four documents as new tabs, then close them again with close_tab.",
        iteration=open_docs([1, 2, 3, 4]) + [Step("close_tab;close_tab;close_tab;close_tab")],
    ),
    Scenario(
        "tab_switching",
        "Cycle through six open tabs; every switch rebuilds the tab bar and its close buttons.",
        setup=open_docs([1, 2, 3, 4, 5]),
        iteration=[Step(";".join(["goto_next_tab"] * DOCUMENT_COUNT))],
    ),
    Scenario(
        "popups",
        "Open the command palette, table of contents, file browser, bookmark list and recent documents; "
        "each popup replaces the previous.",
        setup=open_docs([1]) + [Step("add_bookmark", data="memory test bookmark")],
        iteration=[
            Step("command", wait=False, pause=0.3),
            Step("goto_toc", wait=False, pause=0.3),
            Step("open_document_embedded_from_current_path", wait=False, pause=0.3),
            Step("goto_bookmark", wait=False, pause=0.3),
            Step("open_prev_doc", wait=False, pause=0.3),
        ],
    ),
    Scenario(
        "search",
        "Search the document for two terms and step through the matches.",
        iteration=[
            Step("search", data="the"),
            Step("wait_for_search_to_finish;next_item;next_item;previous_item"),
            Step("search", data="portal"),
            Step("wait_for_search_to_finish;next_item;previous_item"),
        ],
    ),
    Scenario(
        "navigation",
        "Page, scroll and zoom through the document, which renders and caches pages.",
        iteration=[
            Step("goto_end;goto_beginning;zoom_in;zoom_in;zoom_out;zoom_out"),
            Step("screen_down;screen_down;screen_up;screen_up;next_page;previous_page;wait_for_renders_to_finish"),
        ],
    ),
    Scenario(
        "theme_switch",
        "Switch the popup and tab bar theme between light and dark, with a popup open.",
        setup=open_docs([1]) + [Step("command", wait=False, pause=0.3)],
        iteration=[
            Step("setconfig_ui_theme", data="light", pause=0.2),
            Step("setconfig_ui_theme", data="dark", pause=0.2),
        ],
    ),
]


class TestError(Exception):
    pass


def write_test_pdf(path, pages=6, lines_per_page=40):
    """Writes a plain PDF to test with: pages of text (with matches for the search scenario) and a
    table of contents with one entry per page, for goto_toc."""
    page_ids = [5 + 3 * i for i in range(pages)]
    objects = {
        1: "<< /Type /Catalog /Pages 2 0 R /Outlines 3 0 R >>",
        2: f"<< /Type /Pages /Kids [{' '.join(f'{p} 0 R' for p in page_ids)}] /Count {pages} >>",
        3: f"<< /Type /Outlines /First {page_ids[0] + 2} 0 R /Last {page_ids[-1] + 2} 0 R /Count {pages} >>",
        4: "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
    }
    for i, page in enumerate(page_ids):
        text = [f"BT /F1 18 Tf 72 740 Td (Section {i + 1}) Tj ET", "BT /F1 11 Tf 14 TL 72 710 Td"]
        for line in range(lines_per_page):
            text.append(f"(Line {line + 1}: the portal on page {i + 1} links to the next section of the document.) Tj T*")
        text.append("ET")
        stream = "\n".join(text)
        outline = [f"/Title (Section {i + 1}) /Parent 3 0 R /Dest [{page} 0 R /XYZ 0 792 0]"]
        if i > 0:
            outline.append(f"/Prev {page - 1} 0 R")
        if i < pages - 1:
            outline.append(f"/Next {page + 5} 0 R")
        objects[page] = (f"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
                         f"/Resources << /Font << /F1 4 0 R >> >> /Contents {page + 1} 0 R >>")
        objects[page + 1] = f"<< /Length {len(stream)} >>\nstream\n{stream}\nendstream"
        objects[page + 2] = f"<< {' '.join(outline)} >>"
    data = b"%PDF-1.4\n"
    offsets = []
    for number in range(1, len(objects) + 1):
        offsets.append(len(data))
        data += f"{number} 0 obj\n{objects[number]}\nendobj\n".encode("latin-1")
    xref = len(data)
    data += f"xref\n0 {len(objects) + 1}\n0000000000 65535 f \n".encode("latin-1")
    data += "".join(f"{offset:010d} 00000 n \n" for offset in offsets).encode("latin-1")
    data += f"trailer\n<< /Size {len(objects) + 1} /Root 1 0 R >>\nstartxref\n{xref}\n%%EOF\n".encode("latin-1")
    with open(path, "wb") as f:
        f.write(data)


def run(cmd, env=None, timeout=None, check=True):
    result = subprocess.run(cmd, env=env, timeout=timeout, capture_output=True, text=True)
    if check and result.returncode != 0:
        raise TestError(f"{' '.join(cmd)} failed ({result.returncode}): {result.stderr.strip()[:500]}")
    return result


TEST_BUNDLE_ID = "info.yaview.yaview.memorytest"
LSREGISTER = ("/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework"
              "/Support/lsregister")


def prepare_app(app, work_dir):
    """Copies the app, adapts the copy for testing, and signs it as debuggable (get-task-allow).

    Without get-task-allow, macOS lets `leaks` count leaks but not save memory graphs or read the
    leaked memory, so the baseline comparison and allocation stacks would not work. The copy also
    gets its own bundle id, so macOS never mistakes it for the installed yaview, and is marked as an
    agent app (no Dock icon or menu bar, never activated) with App Nap off, so a test running in the
    background is neither in the way nor throttled.
    """
    copy = os.path.join(work_dir, "yaview.app")
    run(["ditto", app, copy])
    info_path = os.path.join(copy, "Contents", "Info.plist")
    with open(info_path, "rb") as f:
        info = plistlib.load(f)
    info.update({"CFBundleIdentifier": TEST_BUNDLE_ID, "LSUIElement": True, "LSAppNapIsDisabled": True})
    with open(info_path, "wb") as f:
        plistlib.dump(info, f)
    entitlements = os.path.join(work_dir, "debuggable.entitlements")
    with open(entitlements, "w") as f:
        f.write('<?xml version="1.0" encoding="UTF-8"?>\n'
                '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n'
                '<plist version="1.0"><dict><key>com.apple.security.get-task-allow</key><true/></dict></plist>\n')
    run(["codesign", "--force", "--deep", "--sign", "-", "--entitlements", entitlements, copy])
    return copy


def prepare_home(work_dir, name):
    """A throwaway home folder, so the tests never read or write the user's yaview data."""
    home = os.path.join(work_dir, name, "home")
    docs = os.path.join(home, "docs")
    os.makedirs(docs)
    config_dir = os.path.join(home, ".config", "yaview")
    os.makedirs(config_dir)
    # A fixed window size keeps footprints comparable: without it the window opens screen-sized,
    # and page renders and GPU buffers grow with the window (several GB on a large display).
    with open(os.path.join(config_dir, "yaview.config"), "w") as f:
        f.write("single_main_window_size 1000 800\nsingle_main_window_move 60 60\n")
    paths = []
    for i in range(DOCUMENT_COUNT):
        path = os.path.join(docs, f"doc-{i}.pdf")
        write_test_pdf(path)
        paths.append(path)
    return home, paths


class Yaview:
    """One yaview process with its own home folder, temp folder and instance name.

    window: "background" launches through LaunchServices without activating the app, so it never
    takes keyboard focus (its window still shows); "hidden" also hides the app, so no window shows
    and pages aren't rendered; "foreground" starts the executable directly, as before.
    """

    def __init__(self, app, home, log_path, stack_logging, window):
        self.app = app
        self.window = window
        self.log_path = log_path
        self.executable = os.path.join(app, "Contents", "MacOS", "yaview")
        self.instance = "yvmem" + uuid.uuid4().hex[:8]
        # Short on purpose: the instance's local socket lives here, and macOS limits socket paths
        # to 104 characters.
        self.tmp_dir = tempfile.mkdtemp(prefix="yv", dir="/tmp")
        self.env = dict(os.environ)
        self.env.update({"HOME": home, "CFFIXED_USER_HOME": home, "TMPDIR": self.tmp_dir + "/"})
        self.app_env = dict(self.env)
        if window != "foreground":
            # Qt activates the app once it has launched unless this is set, and macOS allows that when
            # the launch came from the app the user is working in (the terminal running the tests).
            self.app_env["QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM"] = "1"
        if stack_logging:
            # records where live allocations were made, so leak reports include allocation stacks
            self.app_env["MallocStackLogging"] = "lite"
        self.pid = None
        self.process = None  # only in foreground mode

    def find_pid(self):
        # only the instance itself is started with --new-instance; clients come later
        found = run(["pgrep", "-f", "--", f"--new-instance --instance-name {self.instance}"], check=False)
        return int(found.stdout.split()[0]) if found.stdout.strip() else None

    def is_running(self):
        if self.pid is None:
            return False
        if self.process is not None:
            return self.process.poll() is None
        try:
            os.kill(self.pid, 0)
            return True
        except ProcessLookupError:
            return False

    def start(self, document):
        app_args = ["--new-instance", "--instance-name", self.instance, "--nofocus", document]
        if self.window == "foreground":
            with open(self.log_path, "w") as log:
                self.process = subprocess.Popen([self.executable] + app_args, env=self.app_env,
                                                stdout=log, stderr=subprocess.STDOUT)
            self.pid = self.process.pid
        else:
            cmd = ["open", "-n", "-g", "-F", "-a", self.app, "--stdout", self.log_path, "--stderr", self.log_path]
            if self.window == "hidden":
                cmd.append("-j")
            for name in ("HOME", "CFFIXED_USER_HOME", "TMPDIR", "MallocStackLogging",
                         "QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM"):
                if name in self.app_env:
                    cmd += ["--env", f"{name}={self.app_env[name]}"]
            run(cmd + ["--args"] + app_args, timeout=60)
            deadline = time.time() + 30
            while self.pid is None:
                self.pid = self.find_pid()
                if self.pid is None:
                    if time.time() > deadline:
                        raise TestError("yaview did not start within 30 seconds")
                    time.sleep(0.2)
        socket_path = os.path.join(self.tmp_dir, self.instance)
        deadline = time.time() + 60
        while not os.path.exists(socket_path):
            if not self.is_running():
                raise TestError("yaview exited during startup; see its log")
            if time.time() > deadline:
                raise TestError("yaview did not start within 60 seconds")
            time.sleep(0.2)
        # the server answers once the window has opened the document
        self.send(Step("noop"), timeout=60)

    def send(self, step, documents=None, timeout=30):
        if not self.is_running():
            raise TestError("yaview exited; see its log and ~/Library/Logs/DiagnosticReports")
        # --nofocus on every message: without it yaview raises and activates its window for each one
        # (focus_on_widget in main.cpp), which took the keyboard away from whatever the user was doing.
        cmd = [self.executable, "--instance-name", self.instance, "--nofocus"]
        if step.open_document is not None:
            assert documents is not None
            cmd.append(documents[step.open_document])
        else:
            cmd += ["--execute-command", step.command]
            if step.data is not None:
                cmd += ["--execute-command-data", step.data]
            if step.wait:
                cmd.append("--wait-for-response")
        try:
            run(cmd, env=self.env, timeout=timeout)
            if step.open_document is not None and step.wait:
                # messages are handled in order, so this answers once the document is open
                run([self.executable, "--instance-name", self.instance, "--nofocus", "--execute-command", "noop",
                     "--wait-for-response"], env=self.env, timeout=timeout)
        except subprocess.TimeoutExpired:
            raise TestError(f"no response within {timeout}s to {step}")
        if step.pause:
            time.sleep(step.pause)

    def stop(self):
        if self.pid is None and self.process is None:
            self.pid = self.find_pid()  # startup failed after LaunchServices started it
        if self.is_running():
            # Quit normally first: the single-instance guard's System V shared memory segment is only
            # released on a normal exit. macOS allows 32 segments system-wide, and once killed test
            # instances have used them up, every new yaview (the user's too) exits at startup.
            try:
                run([self.executable, "--instance-name", self.instance, "--nofocus", "--execute-command", "quit"],
                    env=self.env, timeout=10, check=False)
            except subprocess.TimeoutExpired:
                pass
            self.wait_for_exit(10)
        if self.is_running():
            os.kill(self.pid, signal.SIGTERM)
            if not self.wait_for_exit(10):
                os.kill(self.pid, signal.SIGKILL)
                self.wait_for_exit(5)
        if self.process is not None:
            self.process.wait()
        if self.pid is not None:
            remove_shared_memory_created_by(self.pid)
        shutil.rmtree(self.tmp_dir, ignore_errors=True)

    def wait_for_exit(self, seconds):
        deadline = time.time() + seconds
        while self.is_running() and time.time() < deadline:
            time.sleep(0.2)
        return not self.is_running()


def remove_shared_memory_created_by(pid):
    """Removes System V shared memory segments that the (exited) process created and left behind."""
    listing = run(["ipcs", "-m", "-a"], check=False).stdout.splitlines()
    header = next((line.split() for line in listing if line.startswith("T ")), None)
    if header is None:
        return
    for line in listing:
        if line.startswith("m "):
            segment = dict(zip(header, line.split()))
            if segment.get("CPID") == str(pid) and segment.get("NATTCH") == "0":
                run(["ipcrm", "-m", segment["ID"]], check=False)


LEAKS_SUMMARY = re.compile(r"Process \d+: (\d+) leaks? for (\d+) total leaked bytes")
STACK_GROUP = re.compile(r"^STACK OF (\d+) INSTANCES? OF ", re.MULTILINE)


def count_leak_roots(report):
    """The number of separately leaked allocations in a report with allocation stacks.

    `leaks` counts every leaked memory block, so one leaked object that owned 50 blocks counts as 50;
    the report groups the leaked roots by allocation stack ("STACK OF 11 INSTANCES OF ..."). Returns
    None when the report has no stacks (stack logging off).
    """
    groups = STACK_GROUP.findall(report)
    return sum(int(n) for n in groups) if groups else None


def leaks(pid, args, report_path):
    """Runs `leaks` on the process; returns (leaked blocks, leaked bytes, leak roots or None)."""
    # leaks exits with 1 when it found leaks, which is not an error here
    result = run(["leaks", str(pid)] + args, timeout=600, check=False)
    with open(report_path, "w") as f:
        f.write(result.stdout)
        f.write(result.stderr)
    match = LEAKS_SUMMARY.search(result.stdout)
    if not match:
        if result.returncode not in (0, 1):
            raise TestError(f"leaks failed ({result.returncode}), see {report_path}")
        return 0, 0, None
    blocks = int(match.group(1))
    roots = count_leak_roots(result.stdout)
    return blocks, int(match.group(2)), (roots if roots is not None or blocks == 0 else None)


def physical_footprint(pid, report_path):
    """The process's physical footprint in bytes, as Activity Monitor's "Memory" column shows it.

    The report lists it by category (malloc zones, graphics memory, ...), which shows what grew.
    """
    result = run(["footprint", "-p", str(pid), "-f", "bytes"], timeout=120)
    with open(report_path, "w") as f:
        f.write(result.stdout)
    match = re.search(r"Footprint:\s*(\d+)\s*B", result.stdout)
    if not match:
        raise TestError("could not read footprint output")
    return int(match.group(1))


@dataclass
class Result:
    scenario: str
    iterations: int
    # separately leaked allocations (each may own many leaked blocks); blocks when there are no stacks
    new_leaks: int = 0
    new_leaked_blocks: int = 0
    new_leaked_bytes: int = 0
    baseline_footprint: int = 0
    final_footprint: int = 0
    failed: bool = False
    warning: bool = False
    error: Optional[str] = None
    reports: List[str] = field(default_factory=list)

    @property
    def footprint_growth_per_iteration(self):
        return (self.final_footprint - self.baseline_footprint) / max(self.iterations, 1)


def run_scenario(scenario, args, app, work_dir, results_dir):
    result = Result(scenario.name, args.iterations)
    home, documents = prepare_home(work_dir, scenario.name)
    yaview = Yaview(app, home, os.path.join(results_dir, f"{scenario.name}.log"), args.stack_logging, args.window)
    print(f"\n== {scenario.name}: {scenario.description}", flush=True)
    try:
        yaview.start(documents[0])
        for step in scenario.setup:
            yaview.send(step, documents)
        for _ in range(args.warmup):
            for step in scenario.iteration:
                yaview.send(step, documents)
        time.sleep(1)

        pid = yaview.pid
        baseline_graph = os.path.join(results_dir, f"{scenario.name}-baseline.memgraph")
        baseline_report = os.path.join(results_dir, f"{scenario.name}-baseline-leaks.txt")
        leaks(pid, [f"--outputGraph={baseline_graph}"], baseline_report)
        result.baseline_footprint = physical_footprint(pid, os.path.join(results_dir, f"{scenario.name}-baseline-footprint.txt"))
        print(f"   baseline footprint {result.baseline_footprint / 2**20:.1f} MB; "
              f"running {args.iterations} iterations", flush=True)

        start = time.time()
        for i in range(args.iterations):
            for step in scenario.iteration:
                yaview.send(step, documents)
            if (i + 1) % 10 == 0:
                print(f"   {i + 1}/{args.iterations} ({time.time() - start:.0f}s)", flush=True)
        time.sleep(1)

        final_report = os.path.join(results_dir, f"{scenario.name}-new-leaks.txt")
        if os.path.exists(baseline_graph):
            blocks, leaked, roots = leaks(pid, [f"--diffFrom={baseline_graph}"], final_report)
            result.new_leaked_blocks, result.new_leaked_bytes = blocks, leaked
            result.new_leaks = roots if roots is not None else blocks
        else:
            # no memory graph (the app copy could not be made debuggable): compare block totals instead
            base_blocks, base_bytes, _ = leaks(pid, [], baseline_report)
            blocks, leaked, _ = leaks(pid, [], final_report)
            result.new_leaked_blocks, result.new_leaked_bytes = blocks - base_blocks, leaked - base_bytes
            result.new_leaks = result.new_leaked_blocks
        result.final_footprint = physical_footprint(pid, os.path.join(results_dir, f"{scenario.name}-final-footprint.txt"))
        result.reports = [final_report]

        # Leaks that recur grow with the iteration count; a few one-off leaks don't. One-off leaks are
        # still listed in the report and mentioned in the summary.
        result.failed = result.new_leaks >= max(args.min_new_leaks, args.iterations / 4)
        result.warning = result.footprint_growth_per_iteration > args.max_footprint_growth_mb * 2**20
    except TestError as e:
        result.error = str(e)
        result.failed = True
    finally:
        yaview.stop()
    print(f"   {describe(result)}", flush=True)
    return result


def describe(result):
    if result.error:
        return f"ERROR: {result.error}"
    status = "FAIL" if result.failed else ("WARN" if result.warning else "ok")
    if status == "ok" and result.new_leaks > 0:
        status = "ok, one-off leaks"
    return (f"{status}: {result.new_leaks} new leaks ({result.new_leaked_blocks} blocks, "
            f"{result.new_leaked_bytes} bytes) after "
            f"{result.iterations} iterations; footprint {result.baseline_footprint / 2**20:.1f} -> "
            f"{result.final_footprint / 2**20:.1f} MB "
            f"({result.footprint_growth_per_iteration / 2**10:+.0f} KB per iteration)")


def main():
    parser = argparse.ArgumentParser(description="Memory leak tests for yaview on macOS (see README.md).")
    parser.add_argument("--app", default=DEFAULT_APP, help="yaview.app to test (default: the local build)")
    parser.add_argument("--iterations", type=int, default=40, help="repetitions measured per scenario (default 40)")
    parser.add_argument("--warmup", type=int, default=3, help="repetitions before the baseline (default 3)")
    parser.add_argument("--scenario", action="append", help="run only this scenario (repeatable)")
    parser.add_argument("--list", action="store_true", help="list the scenarios and exit")
    parser.add_argument("--window", choices=["background", "hidden", "foreground"], default="background",
                        help="background (default): test windows show but never take focus; hidden: no "
                             "windows (pages aren't rendered, so rendering isn't covered); foreground: "
                             "start yaview directly, as before")
    parser.add_argument("--no-stack-logging", dest="stack_logging", action="store_false",
                        help="faster, but leak reports have no allocation stacks")
    parser.add_argument("--min-new-leaks", type=int, default=3,
                        help="fail at this many new leaks, if also at least one per four iterations (default 3)")
    parser.add_argument("--max-footprint-growth-mb", type=float, default=1.0,
                        help="warn when the footprint grows more than this per iteration (default 1 MB)")
    parser.add_argument("--strict", action="store_true", help="treat footprint warnings as failures")
    parser.add_argument("--results", help="folder for logs and leak reports (default: tests/memory/results/<time>)")
    parser.add_argument("--keep-work-dir", action="store_true", help="keep the app copy and test home folder")
    args = parser.parse_args()

    if args.list:
        for s in SCENARIOS:
            print(f"{s.name:18} {s.description}")
        return 0
    if sys.platform != "darwin":
        print("These tests use macOS's leaks and footprint tools.", file=sys.stderr)
        return 2
    scenarios = [s for s in SCENARIOS if not args.scenario or s.name in args.scenario]
    unknown = set(args.scenario or []) - {s.name for s in SCENARIOS}
    if unknown:
        print(f"Unknown scenario(s): {', '.join(sorted(unknown))}. Use --list.", file=sys.stderr)
        return 2
    if not os.path.isdir(args.app):
        print(f"{args.app} not found; build it with scripts/build_local_macos.sh or pass --app.", file=sys.stderr)
        return 2

    results_dir = args.results or os.path.join(os.path.dirname(os.path.abspath(__file__)), "results",
                                               time.strftime("%Y%m%d-%H%M%S"))
    os.makedirs(results_dir, exist_ok=True)
    work_dir = tempfile.mkdtemp(prefix="yaview-memory-tests-")
    print(f"Results: {results_dir}")
    try:
        try:
            app = prepare_app(args.app, work_dir)
        except TestError as e:
            print(f"Could not make a debuggable copy ({e}); testing {args.app} directly, "
                  "without baseline memory graphs.", file=sys.stderr)
            app = args.app
        results = [run_scenario(s, args, app, work_dir, results_dir) for s in scenarios]
    finally:
        if os.path.isdir(os.path.join(work_dir, "yaview.app")):
            # launching through LaunchServices registered the copy
            run([LSREGISTER, "-u", os.path.join(work_dir, "yaview.app")], check=False)
        if args.keep_work_dir:
            print(f"Work folder kept: {work_dir}")
        else:
            shutil.rmtree(work_dir, ignore_errors=True)

    print("\nSummary")
    for r in results:
        print(f"  {r.scenario:18} {describe(r)}")
    with open(os.path.join(results_dir, "summary.json"), "w") as f:
        json.dump([dict(r.__dict__, footprint_growth_per_iteration=r.footprint_growth_per_iteration)
                   for r in results], f, indent=2)
    failed = any(r.failed or (args.strict and r.warning) for r in results)
    if failed:
        print(f"\nNew leaks are listed in the *-new-leaks.txt reports in {results_dir}.")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
