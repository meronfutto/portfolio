// Программа считает разложение числа N

#include <windows.h> // Нужно для Windows API
#include <stdio.h>

#define MAX_THREADS 1024

int g_threads_count = 0; // Кол-во потоков
int g_n = 0;             // Число N из input.txt

HANDLE g_start_event = NULL;  // Событие, через которое главный поток одновременно запускает рабочих
HANDLE g_task_mutex = NULL;   // Мьютекс для выдачи задач, чтобы два потока не взяли один и тот же first
HANDLE g_result_mutex = NULL; // Мьютекс для добавления результата в g_answer

int g_next_first = 0;             // Следующее первое слагаемое, которое надо выдать потоку
unsigned long long g_answer = 0;  // Общий ответ

unsigned long long CountParts(int sum, int maxPart)
{
    if (sum == 0) return 1;   // Если сумма уже набрана, то значит найден один готовый вариант

    unsigned long long count = 0; // кол-во вариантов

    int limit = maxPart; // наше число maxPart не должно быть больше sum
    if (limit>sum) limit = sum;

    for (int part = limit; part >=1 ; part--){ // Какое число поставим следующим в сумму
        count += CountParts(sum - part, part);
    }

    return count;
}

unsigned long long CountForFirst (int first) // Одна работа потока.
{
    return CountParts(g_n - first, first);
}

int GetNextFirst()
{
    int first = 0;

    WaitForSingleObject(g_task_mutex, INFINITE); // Захватываем замок, любой кто зайдёт, ждёт сколь угодно

    if (g_next_first>0)
    {
        first = g_next_first;
        g_next_first--;
    }

    ReleaseMutex(g_task_mutex);

    return first;
}

void AddAnswer(unsigned long long value)
{
    WaitForSingleObject(g_result_mutex, INFINITE);

    g_answer += value;

    ReleaseMutex(g_result_mutex);
}

DWORD WINAPI WorkerThread(void* param)
{
    unsigned long long localAnswer = 0;

    WaitForSingleObject(g_start_event, INFINITE);

    while (1)
    {
        int first = GetNextFirst();

        if (first <= 0)
            break;

        localAnswer += CountForFirst(first);
    }

    AddAnswer(localAnswer);

    return 0;
}

int main()
{
    FILE* input = fopen("input.txt", "r");
    /*  Входные данные: файл input.txt.
        Первая строка — натуральное число T, количество потоков рабочего пула для решения задачи.
        Вторая строка — натуральное число N (2 <= N <= 100) 
    */

    if (input == NULL)
    {
        return 1;
    }

    fscanf(input, "%d", &g_threads_count); // Считываем кол-во потоков
    fscanf(input, "%d", &g_n);             // Считываем само натуральное число

    fclose(input);

    g_next_first = g_n - 1;                // Начинаем с цифры меньше N

    g_start_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    /* CreateEvent(права_доступа, тип_сброса, начальное_состояние, имя);
       NULL значит: используем стандартные настройки.
       TRUE значит manual-reset event, то есть событие с ручным сбросом.
       FALSE значит: событие сначала не сигнальное.
       Четвёртый параметр — имя события
       Создать безымянное событие, сначала выключенное, с ручным сбросом, 
       чтобы потом одним SetEvent разбудить все потоки. */

    g_task_mutex = CreateMutex(NULL, FALSE, NULL); // Создать свободный безымянный мьютекс.
    // Чтобы два потока не взяли одну и ту же задачу.
    /* CreateMutex(права_доступа, захватить_сразу, имя);
       NULL - Стандартные настройки безопасности.
       FALSE - Создавший поток не захватывает мьютекс сразу. */
    g_result_mutex = CreateMutex(NULL, FALSE, NULL);
    // Чтобы два потока одновременно не прибавляли к общему ответу.

    if (g_start_event == NULL || g_task_mutex == NULL || g_result_mutex == NULL)
    {
        return 1;
    }

    HANDLE* threads = new HANDLE[g_threads_count]; // массив описателей потоков

    for (int i = 0; i < g_threads_count; i++)
    {
        threads[i] = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);

        if (threads[i] == NULL)
        {
            return 1;
        }
    }

    DWORD startTime = GetTickCount(); // Запоминаем время старта алгоритма

    SetEvent(g_start_event); // Будим рабочие потоки

    for (int i = 0; i < g_threads_count; i++)
    {
        WaitForSingleObject(threads[i], INFINITE);
    }
    

    DWORD finishTime = GetTickCount(); // Останавлививаем таймер и считаем время работы алгоритма
    DWORD workTime = finishTime - startTime;
    
    FILE* output = fopen("output.txt", "w");
    if (output != NULL)
    {
        fprintf(output, "%d\n", g_threads_count);
        fprintf(output, "%d\n", g_n);
        fprintf(output, "%llu\n", g_answer);
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

    CloseHandle(g_start_event);
    CloseHandle(g_task_mutex);
    CloseHandle(g_result_mutex);

    delete[] threads;

    return 0;
}