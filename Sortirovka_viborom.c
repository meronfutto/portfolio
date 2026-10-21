#include <stdio.h>

void sort_of_choosing(int* arr, int len)
{
	for (int i = 0; i < len; i++)
	{
		int min = i;
		for (int j = i + 1; j < len; j++)
		{
			if (arr[j] < arr[min])
			{
				min = j;
			}
		}
		int t = arr[i];
		arr[i] = arr[min];
		arr[min] = t;
	}
	return;
}




int main(void)
{
	int arr[] = { 10,9,8,7,6,5,4,3,2,1,5,2,8,9,4,3,6,8,2,1,192491,412512,62141,86431,3,123142,512 };

	int len = sizeof(arr) / sizeof(0);

	sort_of_choosing(arr, len);

	for (int i = 0; i < len; i++) printf("%d ", arr[i]);
}