#pragma once
#include <time.h>

/*
    Модуль истории игр (AVL-дерево + массив для постраничного вывода).

    Что здесь хранится:
    - GameHistory: одна сыгранная партия (счёт, время, победа/поражение, размер поля, дата).
    - AVLNode: узел AVL-дерева (самобалансирующееся BST), используется для быстрого добавления и хранения истории.

    Снаружи модуля доступны:
    - корень дерева (game_history_root)
    - массив history_array (in-order обход дерева) для удобного вывода страниц
    - функции загрузки/сохранения/перестройки массива

*/

// Одна запись истории: результат одной сыгранной игры.
typedef struct GameHistory {
    int game_num;           // Номер игры (или время, или другой уникальный ключ)
    int score;              // Счёт за игру
    int duration_sec;       // Длительность игры в секундах
    int win;                // 1 — победа, 0 — поражение
    int width;              // ширина поля
    int height;             // высота поля
    time_t timestamp;       // время окончания игры (Unix-время)
    char cached_date[32];
} GameHistory; //Хранит одну запись истории сыгранной игры в «Сапёре».

// Узел AVL-дерева: хранит одну запись истории и ссылки на поддеревья.
typedef struct AVLNode {      // структура одного узла (node) самобалансирующегося бинарного дерева поиска AVL
    GameHistory data;         // информация об одной сыгранной игре
    struct AVLNode* left;     // Указатель на левый дочерний узел дерева.
    struct AVLNode* right;    // Указатель на правый дочерний узел дерева.
    int height;               // Высота этого узла в дереве (нужна для поддержания баланса AVL-дерева).
} AVLNode;

/* -------------------- Глобальные данные модуля -------------------- */

// Корень AVL-дерева истории игр (реальное определение находится в history.c)
extern AVLNode* game_history_root;

// Массив всех записей истории в порядке in-order (нужен для удобного вывода на экран)
extern GameHistory* history_array;
extern int history_count;                // сколько записей сейчас в массиве
extern int history_capacity;             // сколько памяти выделено под массив

// Настройки постраничного вывода истории
extern const int ENTRIES_PER_PAGE;       // сколько записей на одной странице
extern int history_page;                 // текущая страница (0..history_max_page)
extern int history_max_page;             // максимальная страница (0-based)


/* -------------------- Внутренние помощники (по дереву) -------------------- */

// Считает количество узлов (записей) в AVL-дереве
int count_nodes(AVLNode* node);

// In-order обход дерева: записывает все элементы в массив по порядку
void fill_inorder(AVLNode* node, GameHistory* history_array, int* idx);

/* -------------------- Основные функции модуля -------------------- */

// Пересобирает history_array из game_history_root и пересчитывает страницы
void rebuild_history_array(void);

// Возвращает лучший (максимальный) score среди всех партий (по history_array)
int get_best_score_of_all_time(void);

// Вставляет новую запись в AVL-дерево (возвращает новый корень поддерева)
AVLNode* avl_insert(AVLNode* node, GameHistory data);

// Загружает историю из файла и строит AVL-дерево
AVLNode* load_history_from_file(const char* filename);

// Сохраняет историю (AVL-дерево) в файл
void save_history_to_file(const char* filename, AVLNode* root);

// Освобождает память всего AVL-дерева
void free_avl_tree(AVLNode* node);

