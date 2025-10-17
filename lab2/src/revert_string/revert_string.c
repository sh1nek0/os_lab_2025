#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
	    if (!str) return;                 // на случай NULL
    size_t i = 0, j = strlen(str);
    if (j == 0) return;               // пустая строка
    j--;                              // последний индекс (без '\0')

    while (i < j) {
        char tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
        i++;
        j--;
    }
}

