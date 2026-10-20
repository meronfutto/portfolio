#include <windows.h>
#include <stdio.h>

#define MAX_THREADS 1024

struct ChessTask // Одна задача для потока
{
    int start_pos; // С какой клетки начинаем перебор
    int left_to_place; // Сколько королей ещё надо поставить
    unsigned char* board; // копия текущей доски для этой задачи
};

int g_threads_count = 0; // Кол-во потоков
int g_n = 0; // Размер доски
int g_l = 0; // Сколько новых королей надо поставить
int g_k = 0; // Сколько королей уже стоит

unsigned char* g_initial_board = NULL; // Начальная доска

ChessTask* g_tasks = NULL; // Массив задач
int g_task_count = 0;      // Сколько задач всего создано
int g_next_task = 0;       // Какую задачу следующий поток должен взять

unsigned long long g_answer = 0; // Общее кол-во решений

HANDLE g_start_event = NULL;  // Событие старт, чтобы потоки не начали считать до таймера
HANDLE g_task_mutex = NULL;   // Мьютекс для выдачи задач
HANDLE g_answer_mutex = NULL; // Мьютекс для общего ответа

int GetIndex(int x, int y) // Переводим координаты клетки в индекс одномерного массива
{
    return x * g_n + y;
}

int IsInside(int x, int y) // Проверка, что координаты внутри доски
{
    return x >= 0 && x < g_n && y >= 0 && y < g_n;
}

unsigned char* CopyBoard(unsigned char* board) // создаём копию доски для каждой задачи
{
    int size = g_n * g_n;
    unsigned char* copy = new unsigned char[size];

    for (int i = 0; i < size; i++)
    {
        copy[i] = board[i];
    }

    return copy;
}

int CanPlaceKing(unsigned char* board, int pos)
{
    if (board[pos]) // Если клетка уже занята, ставить нельзя
        return 0;

    // Переводим pos в координаты
    int x = pos / g_n;
    int y = pos % g_n;

    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0) // Пропускаем саму клетку
                continue;

            int nx = x + dx;
            int ny = y + dy;

            if (IsInside(nx, ny)) // Если сосед внутри доски
            {
                int index = GetIndex(nx, ny);

                if (board[index]) // Проверяем есть ли король
                    return 0;
            }
        }
    }

    return 1; // Ставить можно
}

// Сколько способов поставить left_to_place королей, начиная с клетки start_pos.
unsigned long long CountWays(unsigned char* board, int start_pos, int left_to_place)
{
    // Если ставить больше ничего не надо, то найдено одно решение
    if (left_to_place == 0)
        return 1;

    int size = g_n * g_n;

    // Если клеток осталось меньше, чем королей, то решений нет
    if (size - start_pos < left_to_place)
        return 0;

    unsigned long long count = 0;

    // Пробуем поставить следующего короля в каждую клетку от start_pos до конца
    for (int pos = start_pos; pos < size; pos++)
    {
        if (CanPlaceKing(board, pos)) // Если клетка безопасная
        {
            board[pos] = 1; // ставим короля

            count += CountWays(board, pos + 1, left_to_place - 1);
            /* Рекурсия пробует поставить очередного короля в каждую безопасную клетку. 
            После постановки она вызывает себя, чтобы поставить оставшихся королей дальше по доске. 
            Когда ставить больше не надо, возвращает 1 найденное решение. 
            Потом король убирается, чтобы попробовать другие варианты. */
            board[pos] = 0;
        }
    }

    return count;
}


/* GetNextTask() под мьютексом выдаёт потоку следующую задачу из массива g_tasks.
 Если задачи закончились, возвращает 0. Мьютекс нужен, чтобы два потока не получили 
 одну и ту же задачу. */
int GetNextTask(ChessTask* task) // Взять следующую задачу и добавить результат
{
    int has_task = 0; // считаем, что задачи нет

    // Захватываем мьютекс очереди задач
    WaitForSingleObject(g_task_mutex, INFINITE);

    // Проверяем, остались ли задачи
    if (g_next_task < g_task_count)
    {
        task->start_pos = g_tasks[g_next_task].start_pos;
        task->left_to_place = g_tasks[g_next_task].left_to_place;
        task->board = g_tasks[g_next_task].board;

        g_next_task++;
        has_task = 1;
    }

    ReleaseMutex(g_task_mutex);

    return has_task;
}

// Прибавляем локальный результат потока к общему ответу
void AddAnswer(unsigned long long value)
{
    WaitForSingleObject(g_answer_mutex, INFINITE);

    g_answer += value;

    ReleaseMutex(g_answer_mutex);
}

DWORD WINAPI WorkerThread(void* param)
{
    /* Локальный счётчик нужен, чтобы поток считал 
    свои решения без постоянного обращения к общему 
    g_answer. В общий ответ он добавляет результат 
    один раз в конце, под мьютексом. */
    unsigned long long local_answer = 0;

    WaitForSingleObject(g_start_event, INFINITE);

    while (1)
    {
        ChessTask task;

        if (!GetNextTask(&task))
            break;

        local_answer += CountWays(task.board, task.start_pos, task.left_to_place);
    }

    AddAnswer(local_answer);

    return 0;
}

void CreateTasks()
{
    int size = g_n * g_n;

    if (g_l == 0) // Если новых королей ставить не надо, то уже есть одно решение: начальная доска
    {
        g_answer = 1;
        return;
    }

    g_tasks = new ChessTask[size]; // Создаём массив задач
    g_task_count = 0;
    g_next_task = 0;

    // Пробуем поставить первого нового короля в каждую клетку
    for (int pos = 0; pos < size; pos++)
    {
        // Если клетка безопасна
        if (CanPlaceKing(g_initial_board, pos))
        {
            // Создаём копию начальной доски
            unsigned char* board = CopyBoard(g_initial_board);
            board[pos] = 1; // ставим на эту клетку короля

            // Создаём задачу
            g_tasks[g_task_count].start_pos = pos + 1;
            g_tasks[g_task_count].left_to_place = g_l - 1;
            g_tasks[g_task_count].board = board;

            g_task_count++;
        }
    }
}

int main()
{
    FILE* input = fopen("input.txt", "r");
    if (input == NULL)
    {
        return 1;
    }

    fscanf(input, "%d", &g_threads_count);
    fscanf(input, "%d %d %d", &g_n, &g_l, &g_k);

    if (g_threads_count <= 0 || g_threads_count > MAX_THREADS || g_n <= 0 || g_l < 0 || g_k < 0)
    {
        fclose(input);
        return 1;
    }

    int size = g_n * g_n;

    g_initial_board = new unsigned char[size];

    for (int i = 0; i < size; i++)
    {
        g_initial_board[i] = 0;
    }

    for (int i = 0; i < g_k; i++)
    {
        int x, y;
        fscanf(input, "%d %d", &x, &y);

        if (x >= 0 && x < g_n && y >= 0 && y < g_n)
        {
            g_initial_board[GetIndex(x, y)] = 1;
        }
    }

    fclose(input);

    g_start_event = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_task_mutex = CreateMutex(NULL, FALSE, NULL);
    g_answer_mutex = CreateMutex(NULL, FALSE, NULL);

    if (g_start_event == NULL || g_task_mutex == NULL || g_answer_mutex == NULL)
    {
        return 1;
    }

    CreateTasks();

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

    SetEvent(g_start_event);

    for (int i = 0; i < g_threads_count; i++)
    {
        WaitForSingleObject(threads[i], INFINITE);
    }

    DWORD finishTime = GetTickCount();
    DWORD workTime = finishTime - startTime;

    FILE* output = fopen("output.txt", "w");
    if (output != NULL)
    {
        fprintf(output, "%I64u\n", g_answer);
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
    CloseHandle(g_answer_mutex);

    if (g_tasks != NULL)
    {
        for (int i = 0; i < g_task_count; i++)
        {
            delete[] g_tasks[i].board;
        }

        delete[] g_tasks;
    }

    delete[] threads;
    delete[] g_initial_board;

    return 0;
}