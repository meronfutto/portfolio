#include <stdio.h>
#include <string.h>

void nepovt_bukvi(char* str1, char* str2)
{
	int flag = 0;
	for (int i = 0; str1[i] != '\0'; i++)
	{
		for (int j = 0; str2[j] != '\0'; j++)
		{
			if (str1[i] == str2[j])
			{
				flag = 1;
				break;
			}
		}
		if (flag == 1) {
			flag = 0;
			continue;
		}
		else printf("%c", str1[i]);

	}
}

int main(void)
{
	char str1[100] = "sosiska";
	char str2[100] = "killer";
	nepovt_bukvi(str1, str2);


}