#include <stdio.h>

char* strcpy(char* str1, char* str2)
{
	char* save = str1;
	while (*str1++ = *str2++);
	return save;
}



int main(void)
{
	char s1[20];
	char s2[20] = "abudabi";
	printf("%s", strcpy(s1,s2));

}