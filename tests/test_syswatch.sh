#!/bin/sh
set -eu

program=${1:-./syswatch}

help_output=$($program --help)
printf '%s\n' "$help_output" | grep -q 'terminate <pid>'

list_output=$($program list)
printf '%s\n' "$list_output" | grep -q 'PID'

search_output=$($program search sh)
printf '%s\n' "$search_output" | grep -q 'PID'

$program run /bin/sh -c 'exit 0' | grep -q 'status 0'

sleep 30 &
child=$!
trap 'kill "$child" 2>/dev/null || true' EXIT
$program terminate "$child" >/dev/null
wait "$child" || true
if kill -0 "$child" 2>/dev/null; then
    echo 'temporary process was not terminated' >&2
    exit 1
fi
trap - EXIT

if $program terminate not-a-pid >/dev/null 2>&1; then
    echo 'invalid PID was accepted' >&2
    exit 1
fi

if $program watch 0 >/dev/null 2>&1; then
    echo 'invalid watch duration was accepted' >&2
    exit 1
fi

echo 'All SysWatch smoke tests passed.'
