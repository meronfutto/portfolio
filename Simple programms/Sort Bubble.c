#include <stdio.h>

int main()
{
	int arr[100], i = 0, j=0, element;

	printf("Lenght massive:");

	int lenght;

	scanf_s("%d", &lenght);

	for (i = 0; i < lenght; i++)
	{
		scanf_s("%d", &element);
		arr[i] = element;
	}
	//Sortirovka
	for (i = 0; i < lenght; i++)
	{
		for (j = i; j < lenght; j++)
		{
			if (arr[i] > arr[j])
			{
				int temp = arr[i];

				arr[i] = arr[j];

				arr[j] = temp;
			}

		}
	}
	for (i = 0; i < lenght; i++)
	{
		printf("%d ", arr[i]);
	}
	
	
}