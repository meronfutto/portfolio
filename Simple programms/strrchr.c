#include <stdio.h>

int strrchr(char* str, char s)
{
	int k, i=0;
	while (*str)
	{
		if (*str == s)
		{
			k = i;
		}
		str++;
		i++;
	}
	return k;
}



int main(void)
{
	char str[20] = "ckdmvlsocwww3";
	char s = 'w';
	printf("%d", strrchr(str, s));

}