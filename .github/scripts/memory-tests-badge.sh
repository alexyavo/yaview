#!/usr/bin/env bash
# Publishes the README's memory leak test badge: memory-tests.json, a shields.io endpoint file on the
# orphan `badges` branch, showing the result and date of the latest test run and how many commits
# main has gained since the tested commit. The branch also keeps that run's data for later updates:
# memory-tests-result, memory-tests-date and memory-tests-commit (which memory-tests.yml's `changes`
# job reads too).
#
# Runs in a checkout with main's full history and needs GH_TOKEN with contents: write. With RESULT
# (passing or failing) and TESTED_COMMIT set it records a new run dated today; without them it only
# recounts the commits behind for the recorded run.
set -euo pipefail

files=(memory-tests.json memory-tests-result memory-tests-date memory-tests-commit)
old=$(mktemp -d)
if git fetch -q origin badges; then
  for f in "${files[@]}"; do git show "FETCH_HEAD:$f" > "$old/$f" 2>/dev/null || true; done
fi

if [ -n "${RESULT:-}" ]; then
  result=$RESULT
  date=$(date -u +%Y-%m-%d)
  tested=$TESTED_COMMIT
else
  result=$(cat "$old/memory-tests-result" 2>/dev/null || true)
  date=$(cat "$old/memory-tests-date" 2>/dev/null || true)
  tested=$(cat "$old/memory-tests-commit" 2>/dev/null || true)
fi
if [ -z "$result" ] || [ -z "$date" ]; then
  echo "No memory test run recorded"
  exit 0
fi

message="$result · $date"
# the count is left out when the tested commit is no longer in the history
if [ -n "$tested" ] && git cat-file -e "$tested^{commit}" 2>/dev/null; then
  behind=$(git rev-list --count "$tested..origin/main")
  case $behind in
    0) message="$message · latest commit" ;;
    1) message="$message · 1 commit behind" ;;
    *) message="$message · $behind commits behind" ;;
  esac
fi
if [ "$result" = passing ]; then color=brightgreen; else color=red; fi

new=$(mktemp -d)
printf '{"schemaVersion": 1, "label": "memory leak tests", "message": "%s", "color": "%s"}\n' \
  "$message" "$color" > "$new/memory-tests.json"
echo "$result" > "$new/memory-tests-result"
echo "$date" > "$new/memory-tests-date"
echo "$tested" > "$new/memory-tests-commit"
if diff -rq "$old" "$new" > /dev/null; then
  echo "Badge unchanged: $message"
  exit 0
fi

cd "$new"
git init -q -b badges
git add "${files[@]}"
git -c user.name='github-actions[bot]' \
  -c user.email='41898282+github-actions[bot]@users.noreply.github.com' \
  commit -q -m "Memory leak tests $message: $GITHUB_SERVER_URL/$GITHUB_REPOSITORY/actions/runs/$GITHUB_RUN_ID"
git push -q -f "https://x-access-token:$GH_TOKEN@github.com/$GITHUB_REPOSITORY.git" badges
echo "Badge: $message"
