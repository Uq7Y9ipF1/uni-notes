#!/bin/bash

# Задание 4* (повышенной сложности)
# Скрипт выводит количество и среднее арифметическое своих входных аргументов.
# Пример: ./average.sh 1 2 3  ->  count: 3, average: 2

count=$#

if [ "$count" -eq 0 ]; then
    echo "No arguments given. Usage: $0 num1 num2 ..."
    exit 1
fi

sum=0
for arg in "$@"; do
    sum=$(echo "$sum + $arg" | bc)
done

average=$(echo "scale=4; $sum / $count" | bc)

echo "count: $count"
echo "average: $average"
