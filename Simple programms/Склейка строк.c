#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

void main(void)
{
	char str_1[100] = { '\0' };
	char str_2[100] = { '\0' };
	int i = 0, g = 0;
	(void)scanf("%s", &str_1);
	(void)scanf("%s", &str_2);
	do
	{
		i++;
		
	} while (str_1[i] != '\0');
	for (g; str_2[g] != '\0'; g++)
	{
		str_1[i + g] = str_2[g];
	}
	printf("%s", str_1);
}