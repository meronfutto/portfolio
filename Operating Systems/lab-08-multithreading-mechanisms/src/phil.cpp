#include <pthread.h>  // для потоков Linux
#include <semaphore.h>  // для семафоров
#include <stdio.h>   // для atoi
#include <stdlib.h>  
#include <unistd.h> // для usleep (сон в микросекундах)
#include <time.h>   // для измерения времени

#define PHIL_COUNT 5       // Философов всегда 5
#define STATE_THINKING 'T' 
#define STATE_EATING 'E'

int g_total_time = 0; // TOTAL из командной строки
int g_phil_time = 0;  // PHIL из командной строки

struct timespec g_start_time; // время старта программы

pthread_t g_philosophers[PHIL_COUNT]; // 5 потоков философов
pthread_t g_manager; // поток управляющего

sem_t g_manager_sem; // на нём спит управляющий поток
sem_t g_permission_sem[PHIL_COUNT]; // отдельный семафор для каждого философа

pthread_mutex_t g_mutex;
/* Мьютекс для общих данных:
   очередь запросов,
   состояния философов
   кто ждёт
   кто ест
   флаги завершения
*/

char g_state[PHIL_COUNT]; // Текущее состояние каждого философа

int g_request_queue[PHIL_COUNT]; // Массив очереди
int g_queue_head = 0;  // откуда забираем запрос
int g_queue_tail = 0;  // куда кладём новый запрос
int g_queue_count = 0; // сколько запросов сейчас лежит в очереди

int g_waiting[PHIL_COUNT]; // массив флагов, ждёт ли филосов уже разрешения есть
// 1 - философ попросил есть и ждёт разрешения
// 0 - философ сейчас не ждёт разрешения
int g_eating[PHIL_COUNT]; // чтобы один и тот же философ не попал в очередь несколько раз

int g_finished = 0;
int g_active_philosophers = PHIL_COUNT;

long GetTimeMs() // Сколько мс прошло от старта программы
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    long sec = now.tv_sec - g_start_time.tv_sec;
    long nsec = now.tv_nsec - g_start_time.tv_nsec;

    return sec * 1000 + nsec / 1000000;
}

void SleepMs(int ms) // Философ думает минимум PHIL time
{
    usleep(ms * 1000);
}

void PrintState(int id, char oldState, char newState)
{
    printf("%ld:%d:%c->%c\n", GetTimeMs(), id + 1, oldState, newState);
    fflush(stdout);
}

int LeftNeighbor(int id) // Узнать левого соседа
{
    return (id + PHIL_COUNT - 1) % PHIL_COUNT;
}

int RightNeighbor(int id) // Узнать правого соседа
{
    return (id + 1) % PHIL_COUNT;
}

int CanEat(int id) // Может ли текущий философ есть
{
    int left = LeftNeighbor(id);
    int right = RightNeighbor(id);

    if (g_eating[left])
        return 0;

    if (g_eating[right])
        return 0;

    return 1;
}

void AddRequest(int id)
{
    if (g_waiting[id]) // Если философ уже ждёт, ничего не делаем
        return;

    g_request_queue[g_queue_tail] = id; // Иначе кладём его айди в очередь
    g_queue_tail = (g_queue_tail + 1) % PHIL_COUNT;
    g_queue_count++;

    g_waiting[id] = 1; // Философ ждёт
}

int PopRequest(int* id) // Функция, к-ая достаёт запрос из очереди
{
    if (g_queue_count == 0) // Если очередь пустая, возвращаем 0
        return 0;

    *id = g_request_queue[g_queue_head]; // Иначе берём первого философа из очереди
    g_queue_head = (g_queue_head + 1) % PHIL_COUNT; // Сдвигаем head
    g_queue_count--; // Уменьшаем count;

    return 1;
}

void TryGivePermissions() // Главная функция управляющего
{
    /*  Функция смотрит очередь запросов и разрешает есть
        тем философам, у кого соседи сейчас не едят 
    */
    int checked = g_queue_count; /*  Запоминаем
     сколько запросов было в очереди на момент начала проверки */
    /* checked нужен, чтобы за один проход проверить только текущую очередь 
    и не зациклиться на философах, которым сейчас нельзя есть. 
    Если им нельзя, они возвращаются в очередь и будут проверены позже, 
    когда изменится ситуация. */
    for (int i = 0; i < checked; i++)
    {
        int id; // Философ

        if (!PopRequest(&id)) // Берём 1-ого философа из очереди
            break;

        if (!g_finished && CanEat(id)) // Если программа ещё не заканчивается и философ может есть
        {
            g_waiting[id] = 0;
            g_eating[id] = 1;
            sem_post(&g_permission_sem[id]); // Разрешаем
            // sem_post будит именно этого философа
        }
        else // Если есть нельзя, кладём его обратно в конец очереди
        {
            g_request_queue[g_queue_tail] = id;
            g_queue_tail = (g_queue_tail + 1) % PHIL_COUNT;
            g_queue_count++;
        }
    }
}

// pthread_create требует, чтобы функция имела такой вид
void* PhilosopherThread(void* param) // Функция Философа
{
    int id = (int)((long)param); // Извлекаем id философа

    while (1)
    {
        SleepMs(g_phil_time); //Философ думает

        if (GetTimeMs() >= g_total_time) // Если время вышло, есть не просит
            break;

        
        /* Т.к дальше проверяется общий флаг завершения и происходит
           добавление в общую очередь запросов, и эту общую очередь запросов
           может использовать несколько потоков, следовательно, доступ
           к этим данным должен быть защищён
        */
        pthread_mutex_lock(&g_mutex);

        if (!g_finished)
        {
            AddRequest(id); // Хочет кушать, философ добавляет себя в очередь
            sem_post(&g_manager_sem); // Обращается к управляющеу потоку
        }

        pthread_mutex_unlock(&g_mutex);

        sem_wait(&g_permission_sem[id]); // Философ ждёт разрешения есть

        pthread_mutex_lock(&g_mutex);

        // Проверка на завершение программы
        if (g_finished && !g_eating[id])
        {
            pthread_mutex_unlock(&g_mutex);
            break;
        }

        // Философ начал есть
        PrintState(id, STATE_THINKING, STATE_EATING);

        pthread_mutex_unlock(&g_mutex);

        SleepMs(g_phil_time); // Ест PHIL time 

        pthread_mutex_lock(&g_mutex);

        PrintState(id, STATE_EATING, STATE_THINKING);

        g_eating[id] = 0;
        g_state[id] = STATE_THINKING;

        sem_post(&g_manager_sem); // увеличиваем семафор на 1

        pthread_mutex_unlock(&g_mutex);
    }

    pthread_mutex_lock(&g_mutex);

    g_active_philosophers--;
    sem_post(&g_manager_sem);

    pthread_mutex_unlock(&g_mutex);

    return 0;
}

void* ManagerThread(void* param)
{
    while (1)
    {
        /* В начале управляющий поток спит, т.к мы
           проинициализировали его семафор нулём. 
           Он проснётся тогда, когда философ увеличит
           значение семафора упраляющего через sem_post,
           т.е обратится к нему. До этого ему нечего делать
        */ 
        sem_wait(&g_manager_sem);

        /* Дальше управляющий поток работает с общими данными,
            т.к дальше он будет читать/менять общие данные, связанные
            с очередью, к-ые могут изменить потоки-философы. */
        pthread_mutex_lock(&g_mutex);

        TryGivePermissions(); // Пробуем выдать разрешения есть философами

        if (g_finished && g_active_philosophers == 0)
        {
            pthread_mutex_unlock(&g_mutex);
            break;
        }

        pthread_mutex_unlock(&g_mutex);
    }

    return 0;
}

int main(int argc, char* argv[]) // ./phil 5000 100
{
    if (argc != 3) // Проверяем, что аргументов ровно 3
    {
        return 1;
    }

    g_total_time = atoi(argv[1]); // Переводим строки в числа
    g_phil_time = atoi(argv[2]);

    if (g_total_time <= 0 || g_phil_time <= 0)
    {
        return 1;
    }

    // Запоминаем время старта программы
    clock_gettime(CLOCK_REALTIME, &g_start_time);

    pthread_mutex_init(&g_mutex, 0); // Инициализируем мьютекс

    sem_init(&g_manager_sem, 0, 0); // Семафор управляющего потока
    // Начальное значение – 0


    // Для каждого философа создаём личный семафор разрешения
    for (int i = 0; i < PHIL_COUNT; i++)
    {
        // Начальное значение 0 – философы будут ждать пока управляющий им не разрешит
        sem_init(&g_permission_sem[i], 0, 0); 

        g_state[i] = STATE_THINKING; // В начале все думают
        g_waiting[i] = 0; // никто ещё не ждёт разрешения
        g_eating[i] = 0;  // и никто не ест
    }

    // Создаём управляющий поток
    // Он будет ждать запросы философов и выдавать разрешения
    pthread_create(&g_manager, 0, ManagerThread, 0);

    // Создаём 5 потоков философов
    for (int i = 0; i < PHIL_COUNT; i++)
    {
        pthread_create(&g_philosophers[i], 0, PhilosopherThread, (void*)((long)i));
    }

    SleepMs(g_total_time);

    pthread_mutex_lock(&g_mutex);

    g_finished = 1;

    // Надо разбудить философов, к-ые ожидают разрешения поесть
    for (int i = 0; i < PHIL_COUNT; i++)
    {
        sem_post(&g_permission_sem[i]);
    }

    sem_post(&g_manager_sem); // Программа начинает завершаться
    // Надо разбудить управляющего

    pthread_mutex_unlock(&g_mutex);

    // Нужно подождать всех философов, к-ые могут есть
    for (int i = 0; i < PHIL_COUNT; i++)
    {
        pthread_join(g_philosophers[i], 0); // ждём когда указанный поток завершится
    }

    sem_post(&g_manager_sem);
    pthread_join(g_manager, 0);

    for (int i = 0; i < PHIL_COUNT; i++)
    {
        sem_destroy(&g_permission_sem[i]);
    }

    sem_destroy(&g_manager_sem);
    pthread_mutex_destroy(&g_mutex);

    return 0;
}