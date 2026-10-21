#include <stdio.h>

char* strncpy(char* str1, char* str2,int* n)
{
	char* save = str1;
	int i = 0;
	while (*str2 && i != *n)
	{
		*str1 = *str2;
		str1++;
		str2++;
		i++;
	}
	*str1 = '\0';
	return save;
}



int main(void)
{
	char s1[20];
	char s2[20] = "dasdagg1241";
	int n = 11;
	printf("%s", strncpy(s1, s2,&n));

}