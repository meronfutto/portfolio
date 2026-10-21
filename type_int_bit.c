#include <stdio.h>

int main(void)
{
	int bit = 0;
	signed int number = 1;
	signed int past_number=0;
	while (past_number < number)
	{
		bit++;
		past_number = number;
		number *= 2;
		
	}
	printf("%d", bit);
}