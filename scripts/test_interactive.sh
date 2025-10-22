#!/bin/bash
cd /Users/aleksejstarodubcev/Downloads/SystemMonitor
export SYSMON_DEBUG=1
# Force interactive mode by ensuring we have a TTY
exec 3<&0
exec 0</dev/tty
./sysmon
exec 0<&3
