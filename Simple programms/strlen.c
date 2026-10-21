#include <stdio.h>

int strlen(char* str)
{
	int len = 0;
	while (*str++)len++;
	return len;
}



int main(void)
{
	char* s = "dasdasd";
	printf("%d", strlen(s));
}