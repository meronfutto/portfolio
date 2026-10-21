#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int strlen(char* s)
{
	int len = 0;
	while (*s++ != '\0') len++;
	return(len);
}
int main(void)
{
	char str[11];
	(void)scanf("%s", &str);
	printf("%d", strlen(str));
}