#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <cuda_runtime.h>

#define IDX(i, j, n) ((i) * (n) + (j))

/* ================= CPU helpers ================= */

/* Сохранение матрицы в текстовый файл */
void save_matrix_to_file(const char* filename, double* M, int n) {
    FILE* f = fopen(filename, "w");
    if (f == NULL) {
        printf("Failed to open file '%s'\n", filename);
        return;
    }

    fprintf(f, "%d\n", n);

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fprintf(f, "%.10f ", M[i * n + j]);
        }
        fprintf(f, "\n");
    }

    fclose(f);
    printf("Matrix saved to '%s'\n", filename);
}

void fill_random_matrix(double* A, int n) {
    srand((unsigned)time(NULL));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            A[IDX(i, j, n)] = rand() % 10;
}

void read_matrix_from_input(double* A, int n) {
    printf("Enter matrix elements:\n");
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            printf("A[%d][%d] = ", i, j);
            scanf("%lf", &A[IDX(i, j, n)]);
        }
}

void make_identity(double* M, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            M[IDX(i, j, n)] = (i == j) ? 1.0 : 0.0;
}

/* ================= CUDA kernels ================= */

/* Нормировка опорной строки k */
__global__ void normalize_row(double* A, double* Inv, int n, int k, double pivot) {
    int j = blockIdx.x * blockDim.x + threadIdx.x;
    /* blockIdx - номер блока
       blockDim - размер блока
       threadIdx - номер потока */
    if (j < n) {
        A[k * n + j] /= pivot;
        Inv[k * n + j] /= pivot;
    }
}

/* Зануление столбца k во всех строках */
__global__ void eliminate_column(double* A, double* Inv, int n, int k) {
    int i = blockIdx.y * blockDim.y + threadIdx.y;
    int j = blockIdx.x * blockDim.x + threadIdx.x;
    /* blockIdx - номер блока
       blockDim - размер блока
       threadIdx - номер потока */

    if (i < n && j < n && i != k) {
        double factor = A[i * n + k];
        A[i * n + j] -= factor * A[k * n + j];
        Inv[i * n + j] -= factor * Inv[k * n + j];
    }
}

/* ================= GPU Gauss–Jordan ================= */

int invert_gauss_jordan_gpu(double* A, double* Inv, int n) {
    const double eps = 1e-12;

    double* d_A, * d_Inv;
    cudaMalloc(&d_A, n * n * sizeof(double)); // Выделяем память на видеокарте
    cudaMalloc(&d_Inv, n * n * sizeof(double));

    cudaMemcpy(d_A, A, n * n * sizeof(double), cudaMemcpyHostToDevice); /* Скопировать матрицу A из оперативной памяти(CPU) в память видеокарты(GPU) */
    cudaMemcpy(d_Inv, Inv, n * n * sizeof(double), cudaMemcpyHostToDevice);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);

    for (int k = 0; k < n; k++) {

        /* ---- поиск опорного элемента (CPU) ---- */
        int pivot_row = k;
        double max_val = fabs(A[IDX(k, k, n)]);

        for (int i = k + 1; i < n; i++) {
            double val = fabs(A[IDX(i, k, n)]);
            if (val > max_val) {
                max_val = val;
                pivot_row = i;
            }
        }

        if (max_val < eps) {
            cudaFree(d_A);
            cudaFree(d_Inv);
            return 0;
        }

        /* ---- перестановка строк (CPU) ---- */
        if (pivot_row != k) {
            for (int j = 0; j < n; j++) {
                double tmp = A[IDX(k, j, n)];
                A[IDX(k, j, n)] = A[IDX(pivot_row, j, n)];
                A[IDX(pivot_row, j, n)] = tmp;

                tmp = Inv[IDX(k, j, n)];
                Inv[IDX(k, j, n)] = Inv[IDX(pivot_row, j, n)];
                Inv[IDX(pivot_row, j, n)] = tmp;
            }

            cudaMemcpy(d_A, A, n * n * sizeof(double), cudaMemcpyHostToDevice);
            cudaMemcpy(d_Inv, Inv, n * n * sizeof(double), cudaMemcpyHostToDevice);
        }

        double pivot = A[IDX(k, k, n)];

        /* ---- нормировка строки k (GPU) ---- */
        int threads = 512; // каждый поток - элемент строки
        int blocks = (n + threads - 1) / threads; // сколько нужно блоков для n размера матрицы

        normalize_row <<<blocks, threads >>> (d_A, d_Inv, n, k, pivot); /* Запустить функцию normalize_row на видеокарте,
                                                                        создав blocks блоков, в каждом по threads потоков*/
        cudaDeviceSynchronize(); // CPU ждёт GPU

        /* ---- зануление столбца k (GPU) ---- */
        dim3 block2D(32, 32); // 1 квадрат  
        dim3 grid2D((n + 31) / 32, (n + 31) / 32); // сколько таких квадратов

        eliminate_column <<< grid2D, block2D >>> (d_A, d_Inv, n, k);
        cudaDeviceSynchronize();

        /* ---- обновляем A на CPU для следующего pivot ---- */
        cudaMemcpy(A, d_A, n * n * sizeof(double), cudaMemcpyDeviceToHost);
    }
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float ms = 0.0f;
    cudaEventElapsedTime(&ms, start, stop);

    printf("GPU computation time: %.3f seconds\n", ms / 1000.0f);
    cudaMemcpy(Inv, d_Inv, n * n * sizeof(double), cudaMemcpyDeviceToHost);

    cudaFree(d_A);
    cudaFree(d_Inv);
    return 1;
}

/* ================= main ================= */

int main() {
    int n;
    printf("Enter matrix size n: ");
    scanf("%d", &n);

    double* A = (double*)malloc(n * n * sizeof(double));
    double* Inv = (double*)malloc(n * n * sizeof(double));

    int choice;
    printf("1 - random matrix\n");
    printf("2 - manual input\n");
    printf("Choose input method: ");
    scanf("%d", &choice);

    if (choice == 1)
        fill_random_matrix(A, n);
    else
        read_matrix_from_input(A, n);

    make_identity(Inv, n);

    if (invert_gauss_jordan_gpu(A, Inv, n))
    {
        printf("Matrix successfully inverted on GPU.\n");
        save_matrix_to_file("inverse_gpu.txt", Inv, n);
    }
    else
        printf("Matrix is singular.\n");

    free(A);
    free(Inv);
    return 0;
}