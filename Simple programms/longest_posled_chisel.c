#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

void max_posled(char* str)
{
	int number = 0;
	int max_number = 0;
	for (int i = 0; str[i] != '\0'; i++)
	{
		if (number == 0 || str[i] >= str[i - 1])
		{
			number = number * 10 + (str[i] - '0');
			if (max_number < number) max_number = number;
		}
		else number = str[i]-'0';
		
	}
	char res[100];
	sprintf(res, "%d", max_number);
	printf("%s", res);
}


int main(void)
{
	char arr[100];
	fgets(arr, sizeof(arr), stdin);
	max_posled(arr);
}