#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define IDX(i, j, n) ((i) * (n) + (j))


/* Ввод матрицы с клавиатуры */
void read_matrix_from_input(double* A, int n) {
    printf("Enter matrix elements row by row:\n");

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("A[%d][%d] = ", i, j);
            scanf("%lf", &A[IDX(i, j, n)]);
        }
    }
}

/* Заполнение матрицы случайными числами */
void fill_random_matrix(double* A, int n) { 
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            A[IDX(i, j, n)] = rand() % 10;  // 0..9
        }
    }
}

/* Создание единичной матрицы */
void make_identity(double* M, int n) { 
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double value;
            if (i == j) {
                value = 1.0;
            }
            else {
                value = 0.0;
            }
            M[i * n + j] = value;
        }
    }
}

/* Сохранение матрицы в текстовый файл */
void save_matrix_to_file(const char* filename, double* M, int n) {
    FILE* f = fopen(filename, "w");
    if (f == NULL) {
        printf("Failed to open file '%s' for writing\n", filename);
        return;
    }

    // сначала можем записать размер, чтобы потом было удобнее читать
    fprintf(f, "%d\n", n);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fprintf(f, "%.10f ", M[IDX(i, j, n)]);
        }
        fprintf(f, "\n");
    }

    fclose(f);
    printf("Inverse matrix saved to '%s'\n", filename);
}

/* Алгоритм идёт по диагонали слева направо и на каждом шаге делает один столбец таким же, как в единичной матрице. */
int invert_gauss_jordan(double* A, double* Inv, int n) {
    const double eps = 1e-12; // Если число меньше eps, то это ноль

    for (int k = 0; k < n; k++) {
        int pivot_row = k;
        double max_val = fabs(A[IDX(k, k, n)]);

        // поиск опорного элемента(максимальное число в столбце)
        for (int i = k + 1; i < n; i++) {
            double val = fabs(A[IDX(i, k, n)]);
            if (val > max_val) {
                max_val = val;
                pivot_row = i; // строка с максимальным числом в столбце
            }
        }

        if (max_val < eps) {
            return 0; // матрица вырождена
        }

        // перестановка строк при необходимости
        if (pivot_row != k) {
            for (int j = 0; j < n; j++) {

                double tmpA = A[IDX(k, j, n)];
                A[IDX(k, j, n)] = A[IDX(pivot_row, j, n)];
                A[IDX(pivot_row, j, n)] = tmpA;

                double tmpI = Inv[IDX(k, j, n)];
                Inv[IDX(k, j, n)] = Inv[IDX(pivot_row, j, n)];
                Inv[IDX(pivot_row, j, n)] = tmpI;
            }
        }

        double pivot = A[IDX(k, k, n)];

        // нормировка опорной строки
        for (int j = 0; j < n; j++) {

            // элемент опорной строки k и столбца j
            double a_kj = A[IDX(k, j, n)];

            // делим его на опорный элемент, чтобы получить единицу на диагонали
            A[IDX(k, j, n)] = a_kj / pivot;


            // то же самое делаем для матрицы Inv
            double inv_kj = Inv[IDX(k, j, n)];

            Inv[IDX(k, j, n)] = inv_kj / pivot;
        }

        // зануление столбца k во всех остальных строках
        for (int i = 0; i < n; i++) {

            if (i == k) continue;

            double factor = A[IDX(i, k, n)];

            if (fabs(factor) < eps) continue;

            for (int j = 0; j < n; ++j) {

                // значение элемента в строке i и столбце j
                double a_ij = A[IDX(i, j, n)];

                // значение элемента в опорной строке k и столбце j
                double a_kj = A[IDX(k, j, n)];

                // обновляем элемент строки i
                A[IDX(i, j, n)] = a_ij - factor * a_kj;


                // то же самое для матрицы Inv
                double inv_ij = Inv[IDX(i, j, n)];
                double inv_kj = Inv[IDX(k, j, n)];

                Inv[IDX(i, j, n)] = inv_ij - factor * inv_kj;
            }
        }
    }

    return 1;
}

int main(void) {
    int n;
    printf("Enter matrix size n: ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        printf("Incorrect size\n");
        return 1;
    }

    double* A = malloc(n * n * sizeof(double));
    double* Inv = malloc(n * n * sizeof(double));

    if (A == NULL || Inv == NULL) {
        printf("Failed to allocate memory\n");
        free(A);
        free(Inv);
        return 1;
    }

    int choice;

    printf("Choose input method:\n");
    printf("1 - random matrix\n");
    printf("2 - manual input\n");
    printf("Your choice: ");
    scanf("%d", &choice);

    if (choice == 1) {
        fill_random_matrix(A, n);
    }
    else if (choice == 2) {
        read_matrix_from_input(A, n);
    }
    else {
        printf("Incorrect choice\n");
        free(A);
        free(Inv);
        return 1;
    }

    make_identity(Inv, n);

    
    clock_t start = clock();
    int ok = invert_gauss_jordan(A, Inv, n);
    clock_t end = clock();
    double time_sec = (double)(end - start) / CLOCKS_PER_SEC;
    if (!ok) {
        printf("Matrix is singular: determinant is zero, inverse does not exist.\n");
    }
    else {
        printf("Matrix successfully inverted.\n");
        printf("CPU time: %.3f seconds\n", time_sec);
        
        // сохраняем обратную в файл
        save_matrix_to_file("inverse.txt", Inv, n);
    }

    free(A);
    free(Inv);
    return 0;
}