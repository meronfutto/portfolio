#include <stdio.h>
#include <string.h>

int poisk_podstroki_v_stroke(char* str1, char* str2)
{
	int len1 = strlen(str1);
	int len2 = strlen(str2);
	for (int i = 0; i < len1; i++)
	{
		int j = 0;
		if (str1[i] == str2[j])
		{
			int index = i;
			while (str1[i] == str2[j] && str1[i]!='\0')
			{
				i++;
				j++;
			}
			if (j == len2) return index;

		}
	}
	return 0;
}





int main(void)
{
	char str1[100] = "dimonchik";
	char str2[100] = "chi";
	int len2 = strlen(str2);
	printf("%d", poisk_podstroki_v_stroke(str1, str2));

}