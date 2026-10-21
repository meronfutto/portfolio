#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int factorial(int number)
{
	if (number == 0) return 1;
	else return number * factorial(number - 1);
}

int main(void)
{
	int number;
	(void)scanf("%d", &number);
	printf("%d", factorial(number));

}