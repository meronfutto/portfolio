#include <stdio.h>

void nepovt_sootv_bukvi(char* str1, char* str2)
{
	for (int i = 0; str1[i] || str2[i]; i++)
	{
		if (str1[i] == str2[i])
		{
			continue;
		}
		else printf("%c", str1[i]);
	}
}

int main(void)
{
	char str1[100] = "hello";
	char str2[100] = "world";
	nepovt_sootv_bukvi(str1, str2);
}