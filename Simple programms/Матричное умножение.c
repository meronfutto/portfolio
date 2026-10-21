#include <stdio.h>

int main()
{
	int kol1_str, kol1_stlb, el1_str, el1_stlb, kol2_str, kol2_stlb;     //kol1_str,kol1-stlb - количество строк и столбцов
	int el2_str, el2_stlb, rez = 0, el3_str, el3_stlb;                                               //el1_str,el1_stlb - номер строки,столбца
	int matr1[10][10], matr2[10][10], rez_matr[10][10];
	printf("Enter the number of rows and columns for first matrixe\n");
	scanf_s("%d %d", &kol1_str, &kol1_stlb);       //Пользователь вводит количество строк и столбцов в матрице
	printf("Assign a value to each matrix element\n");
	for (el1_str = 0; el1_str < kol1_str; el1_str++)
		for (el1_stlb = 0; el1_stlb < kol1_stlb ; el1_stlb++)
			scanf_s("%d", &matr1[el1_str][el1_stlb]);     //Пользователь вводит число в определённую ячейку матрицы
	/*
	for (int q = 0; q < c; q++)
	{
		for (int w = 0; w < d; w++)
		{
			printf("%d\t", matr1[q][w]);           //Вывести матрицу А
		}
		printf("\n");
	}
	return 0;
	*/
	printf("Enter the number of rows and columns for second matrix\n");  //Аналогичная операция, что и выше.Создаём матрицу B
	scanf_s("%d %d", &kol2_str, &kol2_stlb);
	if (kol1_stlb != kol2_str)
	{
		printf("Error");
		return 0;
	}
	else
	{
		printf("Assign a value to each matrix element\n");
		for (el2_str = 0; el2_str < kol2_str; el2_str++)
			for (el2_stlb = 0; el2_stlb < kol2_stlb; el2_stlb++)
				scanf_s("%d", &matr2[el2_str][el2_stlb]);
	}

	/*
	for (int q = 0; q < kol2_str; q++)
	{
		for (int w = 0; w < kol2_stlb; w++)
		{
			printf("%d\t", matr1[q][w]);           //Вывести матрицу B
		}
		printf("\n");
	}
	return 0;
	*/

	for (el3_str = 0; el3_str < kol1_str; el3_str++)
	{
		for (el3_stlb = 0; el3_stlb < kol2_stlb; el3_stlb++)
		{
			for (int k = 0; k < kol2_str; k++)
			{
				rez = rez + matr1[el3_str][k] * matr2[k][el3_stlb];
			}
			rez_matr[el3_str][el3_stlb]=rez;
			rez = 0;
		}
	
	}

	for (int q = 0; q < kol1_str; q++)
	{
		for (int w = 0; w < kol2_stlb; w++)
		{
			printf("%d\t", rez_matr[q][w]);           //Вывести матрицу B
		}
		printf("\n");
	}
	return 0;

}