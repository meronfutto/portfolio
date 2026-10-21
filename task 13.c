#include <stdio.h>
#include <string.h>

int bytes_podstroki_in_massive(char arr[3][100], char* podstroka)
{
	int res = 0;
	for (int i = 0; i < 3; i++)
	{
		int k = 0;
		int len = strlen(arr[i]);
		int len_podstroki = strlen(podstroka);
		for (int j = 0; j < len; j++)
		{
			if (arr[i][j] == podstroka[k])
			{
				k++;
				if (k == len_podstroki)
				{
					res += len_podstroki;
				}
			}
			else if (arr[i][j] == podstroka[0])
			{
				k = 0;
				k++;
			}
			else k = 0;

		}
	}
	return res;
}


int main(void)
{
	char arr[3][100];
	char podstroka[100];
	fgets(podstroka, 100, stdin);
	int lenght = strlen(podstroka);
	podstroka[--lenght] = '\0';
	for (int i = 0; i < 3; i++)
	{
		fgets(arr[i], 100, stdin);
		int lenght = strlen(arr[i]);
		arr[i][--lenght] = '\0';
	}
	printf("%d", bytes_podstroki_in_massive(arr, podstroka));

}