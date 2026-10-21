#include <stdio.h>


char strchr(char* str, char c)
{
	int i = 0;
	while (*str != '\0' && *str != c)
	{
		str++;
		i++;
	}
	return i;
}


int main(void)
{
	char str[20] = "dlcmsperg23";
	char c = 's';
	printf("%d", strchr(str, c));

}