#!/bin/bash

echo "Тестирование SystemMonitor..."
echo "================================="

echo "1. Проверка переменных окружения:"
echo "TERM: $TERM"
echo "PWD: $PWD"
echo "Is stdout TTY: $(test -t 1 && echo 'YES' || echo 'NO')"
echo "Is stdin TTY: $(test -t 0 && echo 'YES' || echo 'NO')"

echo ""
echo "2. Запуск с различными опциями:"

echo "   Обычный режим:"
TERM=xterm-256color timeout 5 ./sysmon

echo "   Режим демона:"
./sysmon --daemon &
DAEMON_PID=$!
sleep 2
kill $DAEMON_PID 2>/dev/null

echo "   Prometheus режим:"
./sysmon --prometheus &
PROM_PID=$!
sleep 2
kill $PROM_PID 2>/dev/null

echo ""
echo "3. Проверка запущенных процессов:"
ps aux | grep sysmon | grep -v grep

echo ""
echo "Тестирование завершено."