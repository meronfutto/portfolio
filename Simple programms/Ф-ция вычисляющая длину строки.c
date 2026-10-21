#include <stdio.h>

int str_len1(char* s)
{
	int i;
	for (i = 0; *(s + i) != '\0'; i++)
		;
	return i;
}

int main(void)
{
	char str[] = "GOALACTION";
	printf("len = %d", str_len1(str));
	return 0;
}