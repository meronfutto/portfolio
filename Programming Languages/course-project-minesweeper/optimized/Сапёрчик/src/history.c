#define _CRT_SECURE_NO_WARNINGS
#include "history.h"
#include "app.h"

#include <stdio.h>
#include <stdlib.h>

// Корень AVL-дерева истории игр
AVLNode* game_history_root = NULL;

// Массив истории (in-order)
GameHistory* history_array = NULL;
int history_count = 0;      // текущее количество записей
int history_capacity = 0;   // выделенная ёмкость массива

// Настройки страниц
const int ENTRIES_PER_PAGE = 10;
int history_page = 0;
int history_max_page = 0;

// Возвращает высоту узла AVL-дерева
static int height(AVLNode* node) 
{
    return node ? node->height : 0;
}

// Возвращает максимум из двух чисел
static int my_max(int a, int b) 
{
    return a > b ? a : b;
}

// Вычисляет баланс-фактор узла (разность высот поддеревьев)
static int get_balance(AVLNode* node) 
{
    return node ? height(node->left) - height(node->right) : 0;
}

// Правый поворот AVL-дерева (используется при дисбалансе)
static AVLNode* rotate_right(AVLNode* y) 
{
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = my_max(height(y->left), height(y->right)) + 1;
    x->height = my_max(height(x->left), height(x->right)) + 1;

    return x;
}

// Левый поворот AVL-дерева (используется при дисбалансе)
static AVLNode* rotate_left(AVLNode* x) 
{
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = my_max(height(x->left), height(x->right)) + 1;
    y->height = my_max(height(y->left), height(y->right)) + 1;

    return y;
}

/* Вставляет новую запись истории в AVL - дерево по ключу game_num.
   Возвращает новый корень поддерева. */
AVLNode* avl_insert(AVLNode* node, GameHistory data) 
{
    if (!node) 
    {
        AVLNode* n = malloc(sizeof(AVLNode));
        if (!n) return NULL;

        n->data = data;
        n->left = n->right = NULL;
        n->height = 1;
        return n;
    }

    if (data.game_num < node->data.game_num)
        node->left = avl_insert(node->left, data);
    else if (data.game_num > node->data.game_num)
        node->right = avl_insert(node->right, data);
    else
        return node; // дубликаты не добавляем

    node->height = 1 + my_max(height(node->left), height(node->right));
    int balance = get_balance(node);

    // LL
    if (balance > 1 && data.game_num < node->left->data.game_num)
        return rotate_right(node);

    // RR
    if (balance < -1 && data.game_num > node->right->data.game_num)
        return rotate_left(node);

    // LR
    if (balance > 1 && data.game_num > node->left->data.game_num) 
    {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    // RL
    if (balance < -1 && data.game_num < node->right->data.game_num) 
    {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

// Подсчитывает количество узлов в AVL-дереве
int count_nodes(AVLNode* node) 
{
    if (!node) return 0;
    return 1 + count_nodes(node->left) + count_nodes(node->right);
}

// In-order обход AVL-дерева с сохранением данных в массив
void fill_inorder(AVLNode* node, GameHistory* arr, int* idx) 
{
    if (!node) return;
    fill_inorder(node->left, arr, idx);
    arr[(*idx)++] = node->data;
    fill_inorder(node->right, arr, idx);
}

// Пересобирает массив истории из AVL-дерева и пересчитывает страницы
void rebuild_history_array(void) 
{
    int total = count_nodes(game_history_root);

    free(history_array);
    history_array = NULL;

    history_count = total;
    history_capacity = total;

    if (total == 0) 
    {
        history_page = history_max_page = 0;
        return;
    }

    history_array = malloc(sizeof(GameHistory) * total);
    if (!history_array) 
    {
        history_count = history_capacity = 0;
        return;
    }

    int idx = 0;
    fill_inorder(game_history_root, history_array, &idx);

    history_max_page = (history_count - 1) / ENTRIES_PER_PAGE;
    if (history_page > history_max_page)
        history_page = history_max_page;
}

// Возвращает лучший (максимальный) счёт за всё время
int get_best_score_of_all_time(void) 
{
    int best = 0;
    for (int i = 0; i < history_count; ++i) 
    {
        if (history_array[i].score > best)
            best = history_array[i].score;
    }
    return best;
}

// Загружает историю игр из файла и строит AVL-дерево
AVLNode* load_history_from_file(const char* filename)
{
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;

    AVLNode* root = NULL;
    GameHistory h;
    long ts;
    int win_flag;

    // ОЧИСТКА существующего массива
    history_count = 0;

    while (fscanf(f, "%d %d %d %d %d %d %ld",
        &h.game_num, &h.score, &h.duration_sec,
        &win_flag, &h.width, &h.height, &ts) == 7)
    {
        h.timestamp = (time_t)ts;
        h.win = (win_flag != 0);

        // КЭШИРУЕМ ДАТУ СРАЗУ ПРИ ЗАГРУЗКЕ
        struct tm tinfo_storage;
        struct tm* tinfo;

        // Безопасная версия для Windows
        errno_t err = localtime_s(&tinfo_storage, &h.timestamp);
        if (err == 0) {
            tinfo = &tinfo_storage;
            strftime(h.cached_date, sizeof(h.cached_date),
                "%d.%m.%Y %H:%M:%S", tinfo);
        }
        else {
            // Резервный вариант на случай ошибки
            strncpy(h.cached_date, "??.??.???? ??:??:??",
                sizeof(h.cached_date) - 1);
            h.cached_date[sizeof(h.cached_date) - 1] = '\0';
        }

        root = avl_insert(root, h);

        if (h.game_num >= game_counter) {
            game_counter = h.game_num + 1;
        }
    }

    fclose(f);
    rebuild_history_array();
    return root;
}


// Новая функция для сохранения с подсчётом
static int save_inorder(FILE* f, AVLNode* node) {
    if (!node) return 0;

    int count = 0;
    count += save_inorder(f, node->left);

    fprintf(f, "%d %d %d %d %d %d %lld\n",
        node->data.game_num,
        node->data.score,
        node->data.duration_sec,
        node->data.win,
        node->data.width,
        node->data.height,
        (long long)node->data.timestamp);

    count++;

    count += save_inorder(f, node->right);
    return count;
}

/*static void save_inorder(FILE* f, AVLNode* node) {
    if (!node) return;
    save_inorder(f, node->left);

    // Теперь добавляем timestamp:
    fprintf(f, "%d %d %d %d %d %d %ld\n",
        node->data.game_num,
        node->data.score,
        node->data.duration_sec,
        node->data.win,
        node->data.width,
        node->data.height,
        (long)node->data.timestamp
    );

    save_inorder(f, node->right);
}*/

void save_history_to_file(const char* filename, AVLNode* root)
{
    

    if (!root) {
        printf("[SAVE] WARNING: root is NULL! Nothing to save.\n");
        return;
    }

    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("[SAVE] ERROR: Cannot open file %s for writing!\n", filename);
        perror("fopen");
        return;
    }

    

    int count = 0;
    // Добавьте подсчёт записей
    count = save_inorder(f, root);

    fclose(f);

    
}

/*void save_history_to_file(const char* filename, AVLNode* root)
{
    FILE* f = fopen(filename, "w");
    if (!f) return;

    save_inorder(f, root);

    fclose(f);
}*/

// Освобождает память всего AVL-дерева истории
void free_avl_tree(AVLNode* node) {
    if (!node) return;
    free_avl_tree(node->left);
    free_avl_tree(node->right);
    free(node);
}


