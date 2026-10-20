#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SMALL_SIZE 1000
#define MAX_THREADS 1024

struct SortTask
{
    int left;
    int mid;
    int right;
    int type; // 0 - sort small block, 1 - merge two sorted blocks
};

int g_threads_count = 0;
int g_n = 0;

int* g_array = NULL;
int* g_temp = NULL;

SortTask* g_tasks = NULL;
int g_task_head = 0;
int g_task_tail = 0;
int g_task_count = 0;

int g_finish = 0;
int g_stage_tasks = 0;

pthread_t* g_threads = NULL;
pthread_mutex_t g_mutex;
pthread_cond_t g_cond;
pthread_cond_t g_stage_cond;

void PushTask(int left, int mid, int right, int type)
{
    g_tasks[g_task_tail].left = left;
    g_tasks[g_task_tail].mid = mid;
    g_tasks[g_task_tail].right = right;
    g_tasks[g_task_tail].type = type;

    g_task_tail++;
    g_task_count++;
    g_stage_tasks++;

    pthread_cond_signal(&g_cond);
}

int PopTask(SortTask* task)
{
    pthread_mutex_lock(&g_mutex);

    while (g_task_count == 0 && !g_finish)
    {
        pthread_cond_wait(&g_cond, &g_mutex);
    }

    if (g_finish)
    {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }

    task->left = g_tasks[g_task_head].left;
    task->mid = g_tasks[g_task_head].mid;
    task->right = g_tasks[g_task_head].right;
    task->type = g_tasks[g_task_head].type;

    g_task_head++;
    g_task_count--;

    pthread_mutex_unlock(&g_mutex);

    return 1;
}

void FinishTask()
{
    pthread_mutex_lock(&g_mutex);

    g_stage_tasks--;

    if (g_stage_tasks == 0)
    {
        pthread_cond_signal(&g_stage_cond);
    }

    pthread_mutex_unlock(&g_mutex);
}

void SimpleSort(int left, int right)
{
    for (int i = left + 1; i <= right; i++)
    {
        int value = g_array[i];
        int j = i - 1;

        while (j >= left && g_array[j] > value)
        {
            g_array[j + 1] = g_array[j];
            j--;
        }

        g_array[j + 1] = value;
    }
}

void Merge(int left, int mid, int right)
{
    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid && j <= right)
    {
        if (g_array[i] <= g_array[j])
        {
            g_temp[k] = g_array[i];
            i++;
        }
        else
        {
            g_temp[k] = g_array[j];
            j++;
        }

        k++;
    }

    while (i <= mid)
    {
        g_temp[k] = g_array[i];
        i++;
        k++;
    }

    while (j <= right)
    {
        g_temp[k] = g_array[j];
        j++;
        k++;
    }

    for (i = left; i <= right; i++)
    {
        g_array[i] = g_temp[i];
    }
}

void ProcessTask(SortTask task)
{
    if (task.type == 0)
        SimpleSort(task.left, task.right);
    else
        Merge(task.left, task.mid, task.right);
}

void* WorkerThread(void* param)
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

unsigned long ToMs(struct timespec* tm)
{
    return (unsigned long)tm->tv_sec * 1000 +
           (unsigned long)tm->tv_nsec / 1000000;
}

void WaitStage()
{
    pthread_mutex_lock(&g_mutex);

    while (g_stage_tasks > 0)
    {
        pthread_cond_wait(&g_stage_cond, &g_mutex);
    }

    g_task_head = 0;
    g_task_tail = 0;
    g_task_count = 0;

    pthread_mutex_unlock(&g_mutex);
}

int main()
{
    FILE* input = fopen("input.txt", "r");
    if (input == NULL)
    {
        return 1;
    }

    fscanf(input, "%d", &g_threads_count);
    fscanf(input, "%d", &g_n);

    if (g_threads_count <= 0 || g_threads_count > MAX_THREADS || g_n <= 0)
    {
        fclose(input);
        return 1;
    }

    g_array = new int[g_n];
    g_temp = new int[g_n];
    g_tasks = new SortTask[g_n];
    g_threads = new pthread_t[g_threads_count];

    for (int i = 0; i < g_n; i++)
    {
        fscanf(input, "%d", &g_array[i]);
    }

    fclose(input);

    pthread_mutex_init(&g_mutex, 0);
    pthread_cond_init(&g_cond, 0);
    pthread_cond_init(&g_stage_cond, 0);

    for (int i = 0; i < g_threads_count; i++)
    {
        pthread_create(&g_threads[i], 0, WorkerThread, 0);
    }

    struct timespec startTime;
    struct timespec finishTime;

    clock_gettime(CLOCK_REALTIME, &startTime);

    pthread_mutex_lock(&g_mutex);

    for (int left = 0; left < g_n; left += SMALL_SIZE)
    {
        int right = left + SMALL_SIZE - 1;

        if (right >= g_n)
            right = g_n - 1;

        PushTask(left, 0, right, 0);
    }

    pthread_mutex_unlock(&g_mutex);

    WaitStage();

    for (int width = SMALL_SIZE; width < g_n; width *= 2)
    {
        pthread_mutex_lock(&g_mutex);

        for (int left = 0; left < g_n; left += width * 2)
        {
            int mid = left + width - 1;
            int right = left + width * 2 - 1;

            if (mid >= g_n - 1)
                continue;

            if (right >= g_n)
                right = g_n - 1;

            PushTask(left, mid, right, 1);
        }

        pthread_mutex_unlock(&g_mutex);

        WaitStage();
    }

    clock_gettime(CLOCK_REALTIME, &finishTime);

    pthread_mutex_lock(&g_mutex);
    g_finish = 1;
    pthread_cond_broadcast(&g_cond);
    pthread_mutex_unlock(&g_mutex);

    for (int i = 0; i < g_threads_count; i++)
    {
        pthread_join(g_threads[i], 0);
    }

    unsigned long workTime = ToMs(&finishTime) - ToMs(&startTime);

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
        fprintf(timeFile, "%lu\n", workTime);
        fclose(timeFile);
    }

    pthread_cond_destroy(&g_stage_cond);
    pthread_cond_destroy(&g_cond);
    pthread_mutex_destroy(&g_mutex);

    delete[] g_threads;
    delete[] g_tasks;
    delete[] g_temp;
    delete[] g_array;

    return 0;
}
