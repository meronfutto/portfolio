#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int strcmp(char* s1, char* s2)
{
	for (; *s1 == *s2; s1++, s2++)
	{
		if (*s1 == '\0')
		{
			printf("0");
			return 0;
		}
	}
	printf("%d", *s1 - *s2);
	return *s1 - *s2;
}
int main(void)
{
	char str1[10];
	char str2[10];
	(void)scanf("%s", str1);
	(void)scanf("%s", str2);
	strcmp(str1, str2);
	return 0;
}