#include <stdio.h>
#include <string.h>

void quick_bubble_sort(int* arr, int l, int r)
{
	if (l >= r) return;
	int M = (l + r) / 2;
	int pivot = arr[M];
	int left = l, right = r;
	while (left <= right)
	{
		while (arr[left] < pivot)
		{
			left++;
		}
		while (arr[right] > pivot)
		{
			right--;
		}
		if (left <= right)
		{
			int temp = arr[left];
			arr[left] = arr[right];
			arr[right] = temp;
			left++;
			right--;
		}

	}
	if (l < right) {
		quick_bubble_sort(arr, l, right);
	}
	if (left < r) {
		quick_bubble_sort(arr, left, r);
	}
}


int main(void)
{
	int arr[] = { 5,4,3,2,1 };

	int len = sizeof(arr) / sizeof(0);

	quick_bubble_sort(arr, 0, len - 1);

	for (int i = 0; i < len; i++) printf("%d ", arr[i]);
}

