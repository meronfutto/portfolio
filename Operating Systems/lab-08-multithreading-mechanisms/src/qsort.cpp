#include <windows.h>
#include <stdio.h>

#define SMALL_SIZE 1000   // Если массив <=1000, поток отсортирует его сам
#define MAX_THREADS 1024

struct SortTask // Структура задачи. Отсортировать массив от left до right
{
    int left;
    int right;
};

int g_threads_count = 0; // Кол-во рабочих потоков
int g_n = 0;             // Размер массива
int* g_array = NULL;     // Массив чисел

SortTask* g_tasks = NULL; // массив задач
int g_task_head = 0;      // откуда забираем задачу
int g_task_tail = 0;      // куда кладём новую задачу
int g_task_count = 0;     // сколько задач сейчас в очереди и никем не взяты

int g_work_count = 0;     // кол-во задач, которые ещё не завершены
int g_finish = 0;         // Флаг завершения

CRITICAL_SECTION g_cs;
HANDLE g_task_sem = NULL;

void PushTask(int left, int right) // Функция добавления задачи в очередь
{
    if (left >= right) // Если в диапазоне 0 или 1 элемент, сортировать нечего.
        return;

    EnterCriticalSection(&g_cs); // Заходим в крит. секцию, так как меняем общую очередь

    // Кладём новую задачу в конец очереди.
    g_tasks[g_task_tail].left = left;
    g_tasks[g_task_tail].right = right;

    g_task_tail++;  
    g_task_count++; 
    g_work_count++;

    LeaveCriticalSection(&g_cs);

    // Сообщаем семафору: появиалсь одна задача, можно разбудить поток
    ReleaseSemaphore(g_task_sem, 1, NULL); 
}

int PopTask(SortTask* task)
{
    WaitForSingleObject(g_task_sem, INFINITE); // Поток спит, пока нет задач.

    EnterCriticalSection(&g_cs); // Читаем/меняем очередь

    if (g_finish) // Если программа уже закончила сортировку, поток выходит
    {
        LeaveCriticalSection(&g_cs);
        return 0;
    }

    if (g_task_count == 0)
    {
        LeaveCriticalSection(&g_cs);
        return 0;
    }

    // Берём задачу из очереди.
    task->left = g_tasks[g_task_head].left;
    task->right = g_tasks[g_task_head].right;

    g_task_head++;
    g_task_count--;

    LeaveCriticalSection(&g_cs);

    return 1;
}

void FinishTask()
{
    EnterCriticalSection(&g_cs);

    g_work_count--;

    if (g_work_count == 0) 
    {
        g_finish = 1;
        ReleaseSemaphore(g_task_sem, g_threads_count, NULL); // Будим все рабочие потоки, которые могут спать на семафоре:
    }

    LeaveCriticalSection(&g_cs);
}

void Swap(int* a, int* b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

void SimpleQuickSort(int left, int right)
{
    int i = left;
    int j = right;
    int pivot = g_array[(left + right) / 2];

    while (i <= j)
    {
        while (g_array[i] < pivot)
            i++;

        while (g_array[j] > pivot)
            j--;

        if (i <= j)
        {
            Swap(&g_array[i], &g_array[j]);
            i++;
            j--;
        }
    }

    if (left < j)
        SimpleQuickSort(left, j);

    if (i < right)
        SimpleQuickSort(i, right);
}

void ProcessTask(SortTask task)
{
    int left = task.left;
    int right = task.right;

    if (right - left + 1 <= SMALL_SIZE)
    {
        SimpleQuickSort(left, right);
        return;
    }

    int i = left;
    int j = right;
    int pivot = g_array[(left + right) / 2];

    while (i <= j)
    {
        while (g_array[i] < pivot)
            i++;

        while (g_array[j] > pivot)
            j--;

        if (i <= j)
        {
            Swap(&g_array[i], &g_array[j]);
            i++;
            j--;
        }
    }

    PushTask(left, j);
    PushTask(i, right);
}

DWORD WINAPI WorkerThread(void* param)
{
    while (1)
    {
        SortTask task;

        if (!PopTask(&task))
            break;

        ProcessTask(task);

        FinishTask();
    }

    return 0;
}

int main()
{
    FILE* input = fopen("input.txt", "r");
    if (input == NULL)
    {
        return 1;
    }

    fscanf(input, "%d", &g_threads_count); // Кол-во потоков
    fscanf(input, "%d", &g_n);             // Размер массива

    if (g_threads_count <= 0 || g_threads_count > MAX_THREADS || g_n <= 0)
    {
        fclose(input);
        return 1;
    }

    g_array = new int[g_n]; // Выделяем память под массив

    for (int i = 0; i < g_n; i++) // Читаем все числа
    {
        fscanf(input, "%d", &g_array[i]);
    }

    fclose(input);

    g_tasks = new SortTask[g_n]; // задач сортировки не должно быть больше, чем элементов массива

    // Создаём крит. секцию. Она будет защищать очередь задач, g_work_count, g_finish
    InitializeCriticalSection(&g_cs); 


    int sem_max = g_n; // Значение семафора равно кол-ву элементов задач, к-ое может быть
    if (sem_max < g_threads_count) // Если оно меньше кол-ва потоков, приравниваем к кол-ву потоков
        sem_max = g_threads_count;

    g_task_sem = CreateSemaphore(NULL, 0, sem_max, NULL); // Создаём семафор
    /* NULL - стандартные настройки безопасности
        0 - начальное значение семафора ( сначала задач нет, потоки будут спать)
        sem_max - максимальное значение семафора
        NULL - семафор без имени
        Когда появляется задача, мы увеличиваем семафор через ReleaseSemaphore, и один поток просыпается.

    */
    if (g_task_sem == NULL)
    {
        return 1;
    }

    HANDLE* threads = new HANDLE[g_threads_count];

    for (int i = 0; i < g_threads_count; i++)
    {
        threads[i] = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);

        if (threads[i] == NULL)
        {
            return 1;
        }
    }

    DWORD startTime = GetTickCount();

    if (g_n > 1)
    {
        PushTask(0, g_n - 1);
    }
    else
    {
        EnterCriticalSection(&g_cs);
        g_finish = 1;
        LeaveCriticalSection(&g_cs);
        ReleaseSemaphore(g_task_sem, g_threads_count, NULL);
    }

    for (int i = 0; i < g_threads_count; i++)
    {
        WaitForSingleObject(threads[i], INFINITE);
    }

    DWORD finishTime = GetTickCount();
    DWORD workTime = finishTime - startTime;

    FILE* output = fopen("output.txt", "w");
    if (output != NULL)
    {
        fprintf(output, "%d\n", g_threads_count);
        fprintf(output, "%d\n", g_n);

        for (int i = 0; i < g_n; i++)
        {
            if (i > 0)
                fprintf(output, " ");

            fprintf(output, "%d", g_array[i]);
        }

        fprintf(output, "\n");

        fclose(output);
    }

    FILE* timeFile = fopen("time.txt", "w");
    if (timeFile != NULL)
    {
        fprintf(timeFile, "%lu\n", (unsigned long)workTime);
        fclose(timeFile);
    }

    for (int i = 0; i < g_threads_count; i++)
    {
        CloseHandle(threads[i]);
    }

    CloseHandle(g_task_sem);
    DeleteCriticalSection(&g_cs);


    delete[] threads;
    delete[] g_tasks;
    delete[] g_array;

    return 0;
}
