#include "field.h"

#include <stdio.h>
#include <stdlib.h>

#include "rng.h"   // create_field() использует rng_bounded() для рандомной тасовки
#include "app.h"   // тут лежат start_time/end_time и "кэш" конца игры (g_end_...)

#include <windows.h>  // Уже есть у вас

// Макросы для наносекунд на Windows
#define TIME_START_NS() \
    LARGE_INTEGER _freq_ns, _start_ns; \
    QueryPerformanceFrequency(&_freq_ns); \
    QueryPerformanceCounter(&_start_ns)

#define TIME_END_NS(msg) do { \
    LARGE_INTEGER _end_ns; \
    QueryPerformanceCounter(&_end_ns); \
    double _elapsed_ns = ((double)(_end_ns.QuadPart - _start_ns.QuadPart) * 1e9) / _freq_ns.QuadPart; \
    printf("[TIMING] %s: %.0f ns (%.3f ms)\n", msg, _elapsed_ns, _elapsed_ns / 1e6); \
} while(0)

// =====================================================
//                 Работа с клетками
// =====================================================

// Безопасно возвращает указатель на клетку по координатам.
// Если поле NULL или координаты вне границ — вернёт NULL.
Cell* get_cell(Field* field, int x, int y)
{
    if (!field) return NULL;
    if (x < 0 || x >= field->width)  return NULL;
    if (y < 0 || y >= field->height) return NULL;

    // Клетки лежат в одномерном массиве:
    // индекс = y * width + x
    return &field->cells[y * field->width + x];
}

// =====================================================
//              Создание / пересоздание поля
// =====================================================

void create_field(int width, int height, int mine_count, Field* field)
{
    // Если поле уже было создано ранее — освобождаем старый массив клеток,
    // чтобы не было утечек памяти.
    if (field->cells)
    {
        free(field->cells);
        field->cells = NULL;
    }

    // Сохраняем новые размеры
    field->width = width;
    field->height = height;

    int total = width * height;

    // Выделяем память под все клетки поля
    field->cells = (Cell*)malloc(sizeof(Cell) * total);

    // Если malloc не смог выделить память — это критическая ошибка
    if (!field->cells)
    {
        fprintf(stderr, "Error: failed to allocate cells\n");
        exit(1);
    }

    Cell* cells = field->cells;
    // -------------------------------------------------
    // Инициализация всех клеток "по умолчанию"
    // -------------------------------------------------
    
    TIME_START_NS();
    for (int i = 0; i < total; i++) {
        Cell* c = &field->cells[i];
        c->x = i % width;
        c->y = i / width;
        c->is_mine = 0;
        c->is_revealed = 0;
        c->is_flagged = 0;
        c->is_triggered = 0;
        c->adjacent_mines = 0;
    }
    TIME_END_NS("inithialisathion");
    

    // Если мин попросили больше чем клеток — ограничиваем
    if (mine_count > total) mine_count = total;

    // -------------------------------------------------
    // Быстрая расстановка мин (без повторов)
    // Идея:
    //   1) делаем массив индексов [0..total-1]
    //   2) тасуем Fisher–Yates
    //   3) первые mine_count индексов считаем минами
    // -------------------------------------------------
    
    int* idx = (int*)malloc(sizeof(int) * total);
    for (int i = 0; i < total; ++i) idx[i] = i;

    // Fisher–Yates shuffle:
    // пробегаем с конца, выбираем случайный j [0..i] и меняем местами
    for (int i = total - 1; i > 0; --i)
    {
        uint32_t j = rng_bounded((uint32_t)(i + 1));
        int tmp = idx[i];
        idx[i] = idx[j];
        idx[j] = tmp;
    }

    // Первые mine_count индексов из перемешанного списка — это мины
    for (int i = 0; i < mine_count; ++i)
    {
        int k = idx[i];
        cells[k].is_mine = 1;  
    }
    free(idx);

    // -------------------------------------------------
    // Подсчёт количества мин вокруг для каждой НЕ-мины
    // (8 соседей вокруг клетки)
    // -------------------------------------------------
    
    // Предвычисленные смещения 8 соседей
    static const int offsets[8][2] = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0},          {1,  0},
        {-1,  1}, {0,  1}, {1,  1}
    };

    for (int i = 0; i < total; i++) {
        // Пропускаем мины
        if (cells[i].is_mine) continue;

        int x = i % width;
        int y = i / width;
        int count = 0;

        // Проверяем всех 8 соседей
        for (int n = 0; n < 8; n++) {
            int nx = x + offsets[n][0];
            int ny = y + offsets[n][1];

            // Проверка границ вручную (быстрее вызова функции!)
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                int neighbor_idx = ny * width + nx;
                if (cells[neighbor_idx].is_mine) {
                    count++;
                }
            }
        }

        cells[i].adjacent_mines = count;
    }

    
}
    

// =====================================================
//        Flood fill (раскрытие пустых областей)
// =====================================================

/*
    Раскрывает область пустых клеток (adjacent_mines == 0) и их соседей.

    Важно:
    - сделано через стек, а не рекурсию
      (рекурсия на большом поле легко может убить стек вызовов)
    - возвращает количество открытых клеток
*/
int reveal_area(Field* field, int start_x, int start_y)
{
    
    int revealed = 0; // сколько клеток реально открыли

    // Берём стартовую клетку
    Cell* start = get_cell(field, start_x, start_y);
    if (!start || start->is_revealed || start->is_flagged) return 0;

    // Если стартовая клетка — мина, flood fill не запускаем
    if (start->is_mine) return 0;

    // Максимальный размер стека — все клетки поля (худший случай)
    int max_cells = field->width * field->height;

    // Стек хранит указатели на клетки, которые надо обработать
    Cell** stack = (Cell**)malloc(sizeof(Cell*) * max_cells);
    if (!stack) return 0;

    int sp = 0;              // stack pointer (индекс вершины стека)
    stack[sp++] = start;     // кладём стартовую клетку

    // Пока стек не пустой — достаём клетку и обрабатываем
    while (sp > 0)
    {
        // pop
        Cell* c = stack[--sp];

        // Пропускаем уже открытые или помеченные флагом
        if (!c || c->is_revealed || c->is_flagged) continue;

        // Открываем клетку
        c->is_revealed = 1;
        revealed++;

        // Если у клетки есть рядом мины (число > 0),
        // то по правилам сапёра дальше область не расширяется.
        if (c->adjacent_mines > 0) continue;

        // Иначе (adjacent_mines == 0) — добавляем соседей в стек
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0) continue;

                int nx = c->x + dx;
                int ny = c->y + dy;

                Cell* n = get_cell(field, nx, ny);
                if (!n) continue;

                // Не добавляем клетки, которые уже открыты/помечены/являются минами
                if (n->is_revealed || n->is_flagged || n->is_mine) continue;

                // Защита: не выходим за выделенный размер стека
                if (sp < max_cells)
                {
                    stack[sp++] = n;
                }
            }
        }
    }

    free(stack);
    
    return revealed;
}

// =====================================================
//                 Вспомогательные функции
// =====================================================

// Открывает все клетки поля сразу.
// Используется при проигрыше (показать все мины)
// или при победе (показать итоговое поле).
void reveal_all_cells(Field* field)
{
    

    Cell* cells = field->cells;
    int total = field->width * field->height;

    for (int i = 0; i < total; i++)
    {
        cells[i].is_revealed = 1;
    }

    
}

// Проверяет условие победы:
// победа, если все НЕ-мины открыты.
int check_win(Field* field)
{
    
    for (int y = 0; y < field->height; ++y)
    {
        for (int x = 0; x < field->width; ++x)
        {
            Cell* c = get_cell(field, x, y);

            // Если нашли хотя бы одну не-мину, которая ещё закрыта — победы нет
            if (!c->is_mine && !c->is_revealed) {
                

                return 0;
            }
        }
    }
    
    return 1;
}

// Считает правильно поставленные флаги:
// флаг стоит И под ним реально мина.
int count_correct_flags(Field* field)
{
    

    int cnt = 0;

    // 1. Берём прямой указатель на массив клеток
    Cell* cells = field->cells;

    // 2. Вычисляем общее количество клеток
    int total_cells = field->width * field->height;

    // 3. Один линейный цикл вместо двух вложенных
    for (int i = 0; i < total_cells; i++)
    {
        // 4. Прямой доступ без вызова функций
        if (cells[i].is_flagged && cells[i].is_mine)
        {
            cnt++;
        }
    }

    
    return cnt;
}

// Считает неправильно поставленные флаги:
// флаг стоит, а мины там нет.

int count_wrong_flags(Field* field) {
    
    int cnt = 0;
    Cell* cells = field->cells;
    int total = field->width * field->height;

    for (int i = 0; i < total; i++) {
        if (cells[i].is_flagged && !cells[i].is_mine)
            cnt++;
    }
    
    return cnt;
}



// =====================================================
//          Подсчёт "итога" игры (очки, множитель)
// =====================================================

void compute_end_stats(Field* field, int game_over_state)
{
    // Время игры: конец - старт
    g_end_time_sec = end_time - start_time;

    // Статистика по флагам
    g_end_correct_flags = count_correct_flags(field);
    g_end_wrong_flags = count_wrong_flags(field);

    // "Сырые" очки:
    // +100 за правильный флаг
    // -50 за неправильный
    int raw_score = g_end_correct_flags * 100 - g_end_wrong_flags * 50;

    // Базовое "нормальное" время: зависит от размера поля
    // (чем больше поле, тем больше допускаем времени)
    double max_time = field->width * field->height * 0.8;

    // Множитель по времени (используется только при победе)
    g_end_multiplier = 1.0;

    if (game_over_state == 2) // 2 = победа
    {
        // Чем быстрее победил — тем больше множитель
        g_end_multiplier = max_time / g_end_time_sec;

        // Ограничение снизу, чтобы не уходило слишком мало
        if (g_end_multiplier < 0.2) g_end_multiplier = 0.2;
    }

    // Итоговые очки = raw_score * multiplier
    g_end_final_score = (int)(raw_score * g_end_multiplier);

    // На всякий случай не даём уходить в минус
    if (g_end_final_score < 0) g_end_final_score = 0;

    // Флаг: результаты готовы (их можно показывать на UI)
    g_end_ready = 1;
}