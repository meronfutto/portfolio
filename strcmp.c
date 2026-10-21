#include <stdio.h>

int strcmp(char* str1, char* str2)
{
	while (*str1 == *str2 && *str1 && *str2)
	{
		str1++;
		str2++;
	}
	return (*str2 - *str1);
}

int strncmp(char* str1, char* str2, int n)
{
	int i = 0;
	while (*str1 == *str2 && *str1 && *str2 && i!=n)
	{
		i++;
		str1++;
		str2++;
	}
	return (*str2 - *str1);
}



int main(void)
{
	int n = 3;
	char str1[20] = "abcd";
	char str2[20] = "abcde";
	printf("%d", strncmp(str1, str2,n));
}