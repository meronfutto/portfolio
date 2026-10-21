#include <stdio.h>

int find(char* arr, char c)        //Метод барьера в линейном поиске
{
	int i = 0;
	arr[25] = 'z';                 //Ставим последним элементом искомый элемент
	arr[26] = '\0';
	while (arr[i] != c) i++;       //Пока не дойдём до искомого элемента, проходимся по каждому элементу массива
	if (i < 25) return 1;          //Проверяем, не барьер ли это
	else return 0;
}



int main(void)
{
	char arr[100] = "dkowapdokjgijoirdvivojoid";
	char c = 'z';
	printf("%d", find(arr, c));

}