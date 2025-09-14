#!/bin/bash
# Генерируем 150 случайных чисел от 0 до 999
od -An -N300 -t u1 /dev/urandom | awk '
{
    for(i=1; i<=NF; i++) {
        if(count < 150) {
            print $i
            count++
        }
    }
}' > numbers.txt