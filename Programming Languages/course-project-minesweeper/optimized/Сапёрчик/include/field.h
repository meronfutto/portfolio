#pragma once
#include <stdint.h>


// Структура клетки поля 
typedef struct Cell {
    int x, y;                 // Координаты клетки
    int is_mine;              // 1 - мина, 0 - нет мины
    int is_revealed;          // Открыта ли клетка
    int is_flagged;           // Стоит ли флаг
    int is_triggered;         // Активирована ли мина (чтобы покрасить в красный цвет мину, на к-ую нажал пользователь)
    int adjacent_mines;       // Число мин вокруг
} Cell;

// Структура к-ая описывает всё минное поле.
typedef struct {
    int width;             // Ширина поля
    int height;            // Высота поля
    Cell* cells;           // Массив клеток из width * height
} Field;

Cell* get_cell(Field* field, int x, int y); // Получить структуру клетки

void create_field(int width, int height, int mine_count, Field* field); // Создаёт и полностью инициализирует игровое поле для новой игры

int reveal_area(Field* field, int start_x, int start_y); // Автоматически открывать сразу много клеток вокруг выбранной, если в них нет мин.
void reveal_all_cells(Field* field);     // Раскрывает все клетки поля (победа или проигрыш)

int check_win(Field* field);             // Проверка условий победы

int count_correct_flags(Field* field);   // Кол-во правильных флагов
int count_wrong_flags(Field* field);     // Кол-во неправильных флагов

void compute_end_stats(Field* field, int game_over_state); // Финальная статистика