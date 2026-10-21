#include <stdio.h>
#include <string.h>

int main(void)
{
	char arr[5][1000];
	for (int i = 0; i < 5; i++)
	{
		fgets(arr[i], 1000, stdin);
		int len = strlen(arr[i]);
		arr[i][len - 1] = '\0';
	}
}