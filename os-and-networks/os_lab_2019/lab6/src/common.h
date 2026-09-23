#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

// Умножение по модулю без переполнения 64-битного типа.
// Дублировалось в client.c и server.c -> вынесено в общую библиотеку (задание 3).
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

// Разбор строки в uint64 с проверкой переполнения. Тоже общий для клиента
// и сервера.
bool ConvertStringToUI64(const char *str, uint64_t *val);

#endif
