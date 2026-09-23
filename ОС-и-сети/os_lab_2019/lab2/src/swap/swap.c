#include "swap.h"

void Swap(char *left, char *right)
{
	// Задание 1: меняем местами два символа через указатели
	char temp = *left;
	*left = *right;
	*right = temp;
}
