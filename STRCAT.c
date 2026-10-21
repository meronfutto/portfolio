#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

void strcat(char* s1, char* s2)
{
	while (*s1 != '\0') s1++;
	while (*s2 != '\0')
	{
		*s1++ = *s2++;
	}
	*s1 = '\0';
}
int main(void)
{
	char str_1[100] = { '\0' };
	char str_2[100] = { '\0' };
	(void)scanf("%s", &str_1);
	(void)scanf("%s", &str_2);
	strcat(str_1, str_2);
	printf("%s", str_1);
}