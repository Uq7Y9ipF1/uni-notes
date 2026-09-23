# Отчёт: Лабораторная работа №2 — Язык C, указатели, библиотеки

## ТЗ (что нужно было сделать по сути)
1. Реализовать функцию `Swap(char *left, char *right)` — обмен двух символов **через указатели** (передать в функцию аргументы-указатели и поменять значения по адресам).
2. Реализовать `RevertString(char *str)` — переворот строки «на месте» (in-place), не выделяя новую память. Разобраться, как устроен `main.c`: проверка числа аргументов, `malloc` копии строки, `strcpy`, вызов переворота, `free`.
3. Собрать из кода `revert_string` **статическую** (`librevstr.a`) и **динамическую** (`librevstr.so`) библиотеки, слинковать с ними программу и научиться запускать динамически слинкованный бинарник (`LD_LIBRARY_PATH`).
4. Написать юнит-тесты на фреймворке **CUnit**, слинковать тесты с динамической библиотекой, прогнать.

## Как реализовано

### Задание 1 — Swap (`lab2/src/swap/swap.c`)
```c
#include "swap.h"

void Swap(char *left, char *right) {
    char temp = *left;   // читаем по адресу left
    *left = *right;      // записываем по адресу left значение по адресу right
    *right = temp;       // возвращём сохранённое
}
```
Без указателей такой обмен невозможен: параметры в C передаются по значению, поэтому нужны именно адреса переменных. Сборка: `gcc main.c swap.c -o swap_app && ./swap_app` → вывод `b a`.

### Задание 2 — RevertString (`lab2/src/revert_string/revert_string.c`)
```c
#include "revert_string.h"
#include <string.h>

void RevertString(char *str) {
    if (!str) return;
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}
```
Два индекса встречаются в середине — каждого символа достаточно поменять один раз, сложность O(n/2), доп. память не нужна.

Разбор `main.c`:
1. проверяется, что передан ровно 1 аргумент командной строки;
2. `malloc(strlen(argv[1]) + 1)` — копия строки в куче (+1 под `\0`);
3. `strcpy` копирует аргумент, `RevertString` переворачивает in-place;
4. результат печатается, память освобождается `free` (иначе — утечка).

### Задание 3 — статическая и динамическая библиотеки
Статическая (код библиотек встраивается в исполняемый файл на линковке):
```bash
gcc -c revert_string.c -o revert_string.o
ar rcs librevstr.a revert_string.o
gcc main.c -L. -lrevstr -o static_app
./static_app "Hello"        # -> olleH
```
Динамическая (подгружается рантайм-линковщиком ld.so при запуске):
```bash
gcc -c -fPIC revert_string.c -o revert_string.o   # позиционно-независимый код
gcc -shared revert_string.o -o librevstr.so
gcc main.c -L. -lrevstr -o dynamic_app
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH          # путь для рантайм-поиска
./dynamic_app "World"       # -> dlroW
```
Ключевое отличие: `.a` увеличивается размер бинарника и библиотека «замораживается»; `.so` переиспользуется многими процессами и обновляется без пересборки приложений, но требует поиска в рантайме (`LD_LIBRARY_PATH` или `/etc/ld.so.cache`).

### Задание 4 — CUnit-тесты (`lab2/tests`)
```bash
gcc tests.c -L../revert_string -lrevstr -lcunit -o tests_app
export LD_LIBRARY_PATH=../revert_string:$LD_LIBRARY_PATH
./tests_app
```
Тесты проверяют переворот пустой строки, строки чётной/нечётной длины и одиночного символа.

## Проверка (фактические запуски)
- `swap_app` → `b a`;
- `static_app "Hello"` и `dynamic_app "World"` → `olleH`, `dlroW`;
- `tests_app`: **CUnit — Run Summary: asserts 4, 4 passed, 0 failed**.

## Вывод
Освоены передача по указателю, ручное управление памятью кучи, различие статической и динамической линковки (`-fPIC`, `-shared`, `ar rcs`, `LD_LIBRARY_PATH`) и модульное тестирование на CUnit.
