#include <stdio.h>
#include <string.h>

void insertion_short(int* arr, int len)
{
	for (int i = 1; i < len; i++)
	{
		int x = arr[i];
		int j;
		for (j=i-1; (j >= 0) && (arr[j] > x); j--)
		{
			arr[j + 1] = arr[j];
		}
		arr[j + 1] = x;
	}
}


int main(void)
{
	int arr[] = { 10,9,8,7,6,5,4,3,2,1,5,2,8,9,4,3,6,8,2,1,192491,412512,62141,86431,3,123142,512 };
	
	int len = sizeof(arr) / sizeof(0);

	insertion_short(arr, len);

	for (int i = 0; i < len; i++) printf("%d ", arr[i]);
	
	
}