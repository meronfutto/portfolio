#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int strchr(char* str, char* symbol)
{
	int position = 0;
	while (*str != '\0')
	{
		if (*str == symbol)
		{
			printf("%d", position);
			return 0;
		}
		else
		{
			position++;
			str++;
		}
	}
	return 0;
}
int main(void)
{
	char stroka[10];
	char simvol;
	(void)scanf("%c", &simvol);
	(void)scanf("%s", &stroka);
	strchr(stroka, simvol);
	return 0;
}