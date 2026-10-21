#include <stdio.h>

char* strncat(char* str1, char* str2, int* n)
{
	char* save = str1;
	int i = 0;
	while (*str1)str1++;
	while (*str2 && i != *n)
	{
		*str1 = *str2;
		str1++;
		str2++;
		i++;
	}
	str1 = '\0';
	return save;
}

int main(void)
{
	char s1[20] = "dlvkfir82";
	char s2[20] = "lkdivw";
	int n = 3;
	printf("%s", strncat(s1, s2, &n));
}