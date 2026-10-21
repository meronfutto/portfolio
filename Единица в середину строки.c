#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

int main(void)
{
	char A[11] = { 0 };
	int i = 0;
	(void)scanf("%s", &A);
	while (i != (strlen(A) / 2-1)) i++;
	A[i] = '1';	
	for (i = 0; i < strlen(A); i++)
	{
		printf("%c", A[i]);
	}
	return 0;
}