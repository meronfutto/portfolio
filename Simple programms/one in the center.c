#include <stdio.h>
#include <string.h>

void one_in_the_center(char* str)
{
	int lenght = strlen(str);
	int middle;
	if (lenght % 2 == 0) middle=lenght/ 2;
	else middle = lenght / 2 + 1;
	int i = lenght;
	while (i != middle)
	{
		str[i] = str[i-1];
		i--;
	}
	str[middle] = '1';
	str[lenght + 1] = '\0';
}

int main(void)
{
	char str[100] = "abcdef";   //abcddef
	one_in_the_center(str);
	printf("%s", str);
}