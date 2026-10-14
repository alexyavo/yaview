# Memory leak tests

`memory_leak_tests.py` looks for memory that yaview leaks again and again during normal use.
It needs macOS: it uses Apple's `leaks` and `footprint` tools.

```sh
./scripts/build_local_macos.sh            # the tests use build/local-macos/yaview.app
python3 tests/memory/memory_leak_tests.py # all scenarios, about 5 to 10 minutes
python3 tests/memory/memory_leak_tests.py --list
python3 tests/memory/memory_leak_tests.py --scenario popups --iterations 100
```

The exit status is 1 when a scenario fails, so the script can run in CI or before a release.

You can keep using the Mac while it runs. By default (`--window background`) the test copies are
started through LaunchServices without being activated, and are marked as agent apps (no Dock
icon or menu bar), so they never take keyboard focus, and every message to them carries
`--nofocus` (otherwise yaview raises and activates its window for each one). Their windows
still appear, usually behind yours. macOS doesn't redraw windows that are covered, so in this
mode pages are mostly not rendered, and memory growth that only happens while drawing (page
renders, GPU textures) goes unnoticed: a run in which the test windows were frontmost showed
graphics memory growing with the bookmark list, which background runs don't reproduce. With
`--window hidden` no windows appear at all and nothing is rendered. For full coverage use
`--window foreground` (the window comes to the front and takes focus) when you're away from the
Mac, or the CI workflow, which uses it.

To run without your Mac at all, use the GitHub Actions workflow
`.github/workflows/memory-tests.yml`. It builds yaview on a GitHub-hosted Mac, runs the
suite, and uploads the reports as the `memory-test-results` artifact. It never runs on pushes:
start it from the repository's Actions tab, or with
`gh workflow run memory-tests.yml -f iterations=40 -f scenarios=popups`. It also runs every Sunday
at 06:00 UTC. The badge at the top of the repository's README shows the latest run's result and date.

## What it does

Each scenario starts its own yaview and repeats one iteration of user actions through
`--execute-command`:

| Scenario | Repeats |
| --- | --- |
| `tabs_open_close` | open four documents as tabs, close them with `close_tab` |
| `tab_switching` | cycle through six tabs (rebuilds the tab bar and its close buttons) |
| `popups` | command palette, table of contents, file browser, bookmark list and recent documents, each replacing the previous |
| `search` | two searches and stepping through their matches |
| `navigation` | paging, scrolling and zooming (page rendering and its cache) |
| `theme_switch` | switching `ui_theme` between light and dark with a popup open |

For each scenario the runner:

1. Runs a few warm-up iterations (`--warmup`), so caches and lazily created objects exist.
2. Saves a baseline memory graph with `leaks --outputGraph` and measures the physical footprint.
3. Runs `--iterations` iterations (default 40).
4. Lists only the leaks that are new since the baseline (`leaks --diffFrom`) and measures the
   footprint again.

A leak here is one leaked allocation, such as an object that nothing references anymore,
together with all the memory blocks it owned (`leaks` reports the blocks; the runner counts the
roots, grouped by allocation stack). A scenario **fails** when the new leaks reach
`--min-new-leaks` (default 3) and at least one per four iterations: memory leaked on every
iteration grows with the iteration count. Leaks that happened only once or twice are reported as
"ok, one-off leaks"; they are real leaks too, and the report shows where they were allocated. It **warns** when the footprint grows
by more than `--max-footprint-growth-mb` per iteration (default 1 MB). With `--strict`, warnings
fail too. Footprint growth isn't necessarily a leak: it also covers memory that is still
referenced but never released, such as unbounded caches, which `leaks` can't see.

The test window is 1000x800 points (set in the test home folder's `prefs_user.config`), so
footprints are comparable between runs; a screen-sized window has a footprint of several GB.

Your own yaview data is never touched. Each scenario runs with its own throwaway home folder
(`HOME` and `CFFIXED_USER_HOME`), a private `TMPDIR` for the instance socket, a unique instance
name, and `--nofocus`. Test instances are quit normally, falling back to SIGTERM/SIGKILL. A
killed yaview leaves its single-instance guard's System V shared memory segment behind; macOS
allows only 32 system-wide, and once they're used up every new yaview, including your own, exits
at startup with "Secondary application cannot attach to shared memory block". The runner
removes the segments of instances it had to kill.

## Reading the results

Results go to `tests/memory/results/<date-time>/`, which git ignores, or to the folder given
with `--results`:

- `summary.json`: the numbers for every scenario.
- `<scenario>-new-leaks.txt`: the `leaks` report of leaks that appeared during the measured
  iterations, with the call stack of each allocation. Look for yaview functions (`MainWidget::`,
  `BaseSelectorWidget::`, ...) near the top of the stacks.
- `<scenario>-baseline-footprint.txt`, `<scenario>-final-footprint.txt`: the physical footprint by
  category (malloc, graphics memory, ...). Compare them to see what grew.
- `<scenario>-baseline.memgraph`: the baseline graph. Open it in Xcode's memory graph debugger,
  or inspect it with `leaks`, `heap` or `vmmap` (for example `heap <file>.memgraph`).
- `<scenario>.log`: yaview's output.

The runner tests a copy of the app, ad-hoc signed with the `get-task-allow` entitlement, which
marks it as debuggable. Without that, macOS lets `leaks` count leaks in a running app but not
save memory graphs or show what leaked. If the copy can't be signed, the runner compares leak
totals instead. Allocation stacks come from `MallocStackLogging=lite`;
`--no-stack-logging` is a little faster but the reports then lack stacks.

## Adding a scenario

Add a `Scenario` to `SCENARIOS` in `memory_leak_tests.py`. Each `Step` is a macro (yaview
commands separated by `;`), optional `--execute-command-data`, or `open_document=N` to open one
of the test documents (copies of `tutorial.pdf`). Steps that open a popup must pass
`wait=False`: a popup waits for input, so yaview doesn't answer until it closes.
