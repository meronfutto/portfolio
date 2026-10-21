#define _CRT_SECURE_NO_WARNINGS

// Для картинок
#define STB_IMAGE_IMPLEMENTATION     // Чтобы была реализована библиотека загрузки картинок (stb_image).
#include <stb_image.h>

// OpenGL
#define GLEW_STATIC                  // Чтобы правильно подключалась статическая версия библиотеки GLEW.(OpenGL)
#include <glew.h>                    // Библиотека, которая помогает работать с расширениями OpenGL
#include <glfw3.h>                   // Библиотека для создания окон, обработки ввода и управления контекстом OpenGL

// Прочее
#include <stdio.h>
#include <stb_easy_font.h>           // Библиотека для вывода текста в OpenGL.
#include <time.h>                    // Для замера времени игры, таймеров, и хранения даты/времени:
#include <stdbool.h>                 // Вообще необязательно, просто захотелось использовать булевую переменную
#include <Windows.h>
#include <profileapi.h>  // для QueryPerformanceCounter

// Макрос для замера времени в наносекундах
#define TIME_START_NS() long long _ns_start = get_time_ns()
#define TIME_END_NS(msg) do { \
    long long _ns_end = get_time_ns(); \
    long long _ns_diff = _ns_end - _ns_start; \
    printf("[TIMING-NS] %s: %lld ns (%.3f ms)\n", \
        msg, _ns_diff, _ns_diff / 1000000.0); \
} while(0)

// Получить время в наносекундах
static long long get_time_ns(void) {
    static LARGE_INTEGER freq;
    static int freq_initialized = 0;

    if (!freq_initialized) {
        QueryPerformanceFrequency(&freq);
        freq_initialized = 1;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    // Преобразуем в наносекунды (1 секунда = 1 000 000 000 нс)
    return (long long)((double)counter.QuadPart * 1.0e9 / (double)freq.QuadPart);
}

static double qpc_now_sec(void) {
    static LARGE_INTEGER freq;
    static int inited = 0;
    if (!inited) { QueryPerformanceFrequency(&freq); inited = 1; }

    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
}

int g_tab_held = 0;   // 1, если Tab зажат, 0 — если нет

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const float BASE_CELL_PX = 25.0f;   // базовый размер клетки в пикселях
float g_zoom = 1.0f;               // текущий масштаб
float g_cam_x = 0.0f;               // камера: левая видимая клетка
float g_cam_y = 0.0f;               // камера: верхняя видимая клетка

// Для перетаскивания средней кнопкой
int   g_middle_dragging = 0;
double g_drag_last_x = 0.0;
double g_drag_last_y = 0.0;




// Текущая ширина игрового окна
int window_width = 1680;    // Ширина
int window_height = 1020;   // Высота


GLFWwindow* g_window;       // глобально сохраняем указатель на окно
GLuint mineTexture = 0;     // идентификатор OpenGL-текстуры для картинки мины.
GLuint flagTexture = 0;     // идентификатор OpenGL-текстуры для картинки флажка.
GLuint howToTexture = 0;    // текстура для экрана How To Play

char inputWidth[8] = "";    // Ширина поля (Текстовый буфер для ввода ширины поля пользователем)
char inputHeight[8] = "";   // Высота поля (Текстовый буфер для ввода высоты поля пользователем)
int typingWidth = 1;        // если 1 — вводим ширину, если 0 — высоту



int current_mine_count = 0; //  фактическое количество мин для текущего поля:
int game_over = 0;          // Флаг состояния игры
double start_time = 0;      // время старта игры (в секундах)
double current_time = 0;    // текущее время в игре
double end_time = 0;        // глобальная переменная, изначально 0
int score = 0;              // Очки
int game_counter = 1;       // Счётчик игр
int remaining_mines = 0;
bool history_saved = false; // "Сохранили ли мы уже результат текущей партии в историю?"

int all_time_best_score;    // "лучший результат за всё время"

// Структура клетки поля (двусвязный линейный список)
typedef struct Cell {
    int x, y;                 // координаты клетки
    int is_mine;              // 1 если мина, иначе 0
    int is_revealed;          // открыта ли клетка
    int is_flagged;           // стоит ли флаг
    int is_triggered;         // Активирована ли мина (чтобы покрасить в красный цвет мину, на к-ую нажал пользователь)
    int adjacent_mines;       // число мин вокруг
    struct Cell* next;        // следующая клетка в списке
    struct Cell* prev;        // предыдущая клетка в списке
} Cell;

// ЗАЩИТА 3
typedef NTSTATUS(NTAPI* pNtQueryInformationProcess)(
    HANDLE ProcessHandle,
    ULONG ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
    );

// Структура к-ая описывает всё минное поле.
typedef struct {
    Cell* head;            // начало списка (первая клетка)
    Cell* tail;            // конец списка (последняя клетка)
    int width;             // ширина поля
    int height;            // высота поля
} CellList;
CellList* current_field = NULL;  // Пока поля нет

typedef struct GameHistory {
    int game_num;           // Номер игры (или время, или другой уникальный ключ)
    int score;              // Счёт за игру
    int duration_sec;       // Длительность игры в секундах
    int win;                // 1 — победа, 0 — поражение
    int width;              // ширина поля
    int height;             // высота поля
    time_t timestamp;       // время окончания игры (Unix-время)
} GameHistory; //Хранит одну запись истории сыгранной игры в «Сапёре».

typedef struct AVLNode {      // структура одного узла (node) самобалансирующегося бинарного дерева поиска AVL
    GameHistory data;         // информация об одной сыгранной игре
    struct AVLNode* left;     // Указатель на левый дочерний узел дерева.
    struct AVLNode* right;    // Указатель на правый дочерний узел дерева.
    int height;               // Высота этого узла в дереве (нужна для поддержания баланса AVL-дерева).
} AVLNode;
AVLNode* game_history_root = NULL; // Корень дерева истории игр

// Динамический массив для хранения всех записей истории (in?order)
GameHistory* history_array = NULL;
int          history_count = 0;   // текущее число записей в массиве
int          history_capacity = 0;   // ёмкость выделенного массива

// Настройки постраничного вывода
const int ENTRIES_PER_PAGE = 10;     // Сколько записей на одной странице
int       history_page = 0;          // Текущая страница (0-based)
int       history_max_page = 0;      // Максимальный номер страницы (0-based)

// Структура Кнопки
typedef struct {
    float x, y, w, h;
    const char* label;
} Button;

typedef enum {        //Различные экраны(главное меню,настройки)
    SCREEN_MAIN_MENU,
    SCREEN_FIELD_SELECTION,
    SCREEN_GAME,
    SCREEN_HISTORY,
    SCREEN_HOW_TO_PLAY
} ScreenState;
ScreenState currentScreen = SCREEN_MAIN_MENU; //Начальный экран


Button* hoveredButton = NULL; // Переменная, указывающая на “наведённую” кнопку (выделять кнопку визуально при наведении)
Button* clickedButton = NULL; // Переменная, указывающая на “кликнутую” кнопку

Button btnExit = { 100, 400, 150, 60, "Exit" };

Button btnPlay = { 100, 100, 170, 70, "Play" };

Button btnBack = { 30, 940, 190, 60, "Back" };

Button btnRetry = { 0, 0, 180, 50, "Retry" };

Button btnMainMenu = { 0, 0, 300, 50, "Main Menu" };

Button btnStartCustom = { 100, 250, 240, 65, "START" };

Button zoneWidth = { 650, 80, 400, 50, "" };   // x, y, w, h

Button zoneHeight = { 650, 150, 400, 50, "" };

Button btnHistory = { 100, 200, 560, 70, "History games" };

Button btnHowTo = { 100, 300, 480, 70, "How to play" };

// Кнопки для навигации внутри истории
Button btnPrevPage = { 500, 750, 120, 40, "Prev" };
Button btnNextPage = { 1000, 750, 120, 40, "Next" };
Button btnBackFromHistory = { 30, 900, 180, 80, "Back" };


/////////////////////////////////////////////////////(ТЕКСТ И ТЕКСТУРЫ)/////////////////////////////////////////////////////////////////////////////////////

GLuint load_texture(const char* filename, int* outWidth, int* outHeight); //загружает картинку из файла и создаёт из неё объект - текстуру для OpenGL, возвращая её номер(ID).
void draw_texture(GLuint texture, float x, float y, float width, float height); // Функция отрисовки текстуры (мины, флажок и т.д)
void draw_text_scaled(const char* text, float x, float y, float scale);         // Рисует текст на экране, к-ый может масштабироваться
void draw_centered_text(const char* text, float x, float y, float cell_size, float scale); //Нарисовать текст по центру внутри заданной области
void draw_field(CellList* field);       // Рисует всё игровое поле “Сапёра” на экране(Все клетки, их состояния, цифры, мины, флажки и т.д.)
void draw_background(GLuint texture, int imgW, int imgH); // рисует фоновую картинку на весь экран
void draw_button(Button* btn, float scale); // Рисует одну кнопку интерфейса
float get_text_width(const char* text);     // Вычисляет ширину строки текста

////////////////////////////////////////////////////////////////(ПОЛЕ И ИГРОВОЙ ПРОЦЕСС)//////////////////////////////////////////////////////////////////////

void reveal_all_cells(CellList* field);     // Раскрывает все клетки поля (победа или проигрыш)
void get_cell_layout(float* cell_size, float* offsetX, float* offsetY);    // Вычисляет, как правильно разместить всё игровое поле (сетку клеток) внутри окна любого размера.
void create_field(int width, int height, int mine_count, CellList* field); // Создаёт и полностью инициализирует игровое поле для новой игры
void reveal_cell_recursive(Cell* cell); // Автоматически открывать сразу много клеток вокруг выбранной, если в них нет мин.
int check_win(CellList* field);             // Проверка условий победы
int count_correct_flags(CellList* field);   // Кол-во правильных флагов
int count_wrong_flags(CellList* field);     // Кол-во неправильных флагов

////////////////////////////////////////////////////////////////(ОБРАБОТКА НАЖАТИЙ)//////////////////////////////////////////////////////////////////////
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods); //Обрабатывает ввод с клавиатуры в меню выбора размера поля
void char_callback(GLFWwindow* window, unsigned int codepoint); // нужен, чтобы ловить и обрабатывать вводимые пользователем символы (цифры, буквы)
void mouse_click_on_field(double mouseX, double mouseY); // Обрабатывает клик мышкой по игровому полю
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods); //Обрабатывает нажатия и отпускания кнопок мыши в окне твоей игры.
int is_inside(Button* btn, double x, double y); //проверяет, попала ли точка (обычно мышь) внутрь области кнопки

//РАЗНОЕ
void update_projection(); // Настраивает параметры OpenGL для отображения 2D-графики в окне любого размера
int my_max(int a, int b); // Просто функция, к-ая определяет максимум из 2-ух чисел
void set_window_icon(GLFWwindow* window); // Для иконки игры

////////////////////////////////////////////////////////////////(Всё что связано с деревом(историей игр))//////////////////////////////////////////////////////////////////////

int count_nodes(AVLNode* node);   // считает общее количество узлов (то есть записей об играх)
int get_best_score_of_all_time(); // ищет и возвращает максимальный (лучший) результат (score) среди всех сыгранных партий
int height(AVLNode* node);        // возвращает высоту переданного узла AVL-дерева
int get_balance(AVLNode* node);   // Вычисляет баланс-фактор узла AVL-дерева.
void fill_inorder(AVLNode* node, GameHistory* history_array, int* idx); // обходит всё AVL-дерево истории игр "слева-направо" (in-order) и записывает все записи в массив history_array по порядку.
void save_inorder(FILE* f, AVLNode* node); //сохраняет всё AVL-дерево истории игр в файл
void save_history_to_file(const char* filename, AVLNode* root); //сохраняет всю историю игр (AVL-дерево) в файл с именем history.txt
void free_avl_tree(AVLNode* node); //функцию для удаления всего дерева.
AVLNode* rotate_right(AVLNode* y); // правый (правосторонний) поворот
AVLNode* rotate_left(AVLNode* x);  // левый (левосторонний) поворот
AVLNode* avl_insert(AVLNode* node, GameHistory data);  //функция вставки нового элемента в AVL-дерево
AVLNode* load_history_from_file(const char* filename); // загружает все старые партии из файла истории и строит по ним дерево для работы внутри игры
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void clamp_camera();
void reset_camera_for_field(int w, int h);
float get_min_zoom();
void draw_text_shadowed(const char* text, float x, float y, float scale);
void draw_panel(float x, float y, float w, float h, float alpha);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void set_window_icon(GLFWwindow* window) {
    GLFWimage images[1];
    images[0].pixels = stbi_load("icon.png", &images[0].width, &images[0].height, 0, 4);
    if (images[0].pixels) {
        glfwSetWindowIcon(window, 1, images);
        stbi_image_free(images[0].pixels);
    }
}

int count_nodes(AVLNode* node) { //считает общее количество узлов (то есть записей об играх)
    if (!node) return 0;
    return 1 + count_nodes(node->left) + count_nodes(node->right);
    // 1 (за текущий узел) + число узлов в левом поддереве + число узлов в правом поддереве.
}

void fill_inorder(AVLNode* node, GameHistory* history_array, int* idx) {
    if (!node) return;
    fill_inorder(node->left, history_array, idx);
    history_array[(*idx)++] = node->data;
    fill_inorder(node->right, history_array, idx);
}

// Пересобрать history_array из game_history_root: выделить память, заполнить inorder
void rebuild_history_array(void) {
    // 1) Узнаём, сколько всего узлов в дереве
    int total = count_nodes(game_history_root);
    history_count = total;
    history_capacity = total;

    // 2) Очистим старый массив (если он был)
    if (history_array) {
        free(history_array);
        history_array = NULL;
    }

    // 3) Если записей нет — просто сбросим все счётчики
    if (total == 0) {
        history_array = NULL;
        history_count = 0;
        history_capacity = 0;
        history_page = 0;
        history_max_page = 0;
        return;
    }

    // 4) Выделим ровно столько памяти, сколько нужно
    history_array = (GameHistory*)malloc(sizeof(GameHistory) * total);
    if (!history_array) {
        fprintf(stderr, "Error: Failed to allocate memory for history_array\n");
        history_count = 0;
        history_capacity = 0;
        history_page = 0;
        history_max_page = 0;
        return;
    }

    // 5) Заполним массив inorder обходом
    int write_idx = 0;
    fill_inorder(game_history_root, history_array, &write_idx);

    // 6) Рассчитаем, сколько страниц у нас получится
    //    (0-based: если, скажем, 23 записи и по 10 на страницу ? страницы 0,1,2)
    history_max_page = (history_count - 1) / ENTRIES_PER_PAGE;
    if (history_page > history_max_page) {
        history_page = history_max_page;
    }
}

int get_best_score_of_all_time() {
    int best = 0;
    for (int i = 0; i < history_count; ++i) {
        if (history_array[i].score > best) {
            best = history_array[i].score;
        }
    }
    return best;
}

int my_max(int a, int b) { return a > b ? a : b; }


int height(AVLNode* node) {
    return node ? node->height : 0;
}

// Вычисляет баланс-фактор узла AVL-дерева.
int get_balance(AVLNode* node) {
    return node ? height(node->left) - height(node->right) : 0;
}


AVLNode* rotate_right(AVLNode* y) {
    AVLNode* x = y->left;     // x — левый потомок y
    AVLNode* T2 = x->right;   // T2 — правое поддерево x
    x->right = y;
    y->left = T2;
    y->height = my_max(height(y->left), height(y->right)) + 1;
    x->height = my_max(height(x->left), height(x->right)) + 1;
    return x;
}


AVLNode* rotate_left(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = my_max(height(x->left), height(x->right)) + 1;
    y->height = my_max(height(y->left), height(y->right)) + 1;
    return y;
}


AVLNode* avl_insert(AVLNode* node, GameHistory data) {
    if (!node) {
        AVLNode* new_node = malloc(sizeof(AVLNode));
        if (!new_node) {
            fprintf(stderr, "Error: Failed to allocate memory for new AVL node!\n");
            exit(1); // Можно обработать иначе, если нужно
        }
        new_node->data = data;
        new_node->left = new_node->right = NULL;
        new_node->height = 1;
        return new_node;
    }

    // по ключу — например, game_num или score
    if (data.game_num < node->data.game_num)
        node->left = avl_insert(node->left, data);
    else if (data.game_num > node->data.game_num)
        node->right = avl_insert(node->right, data);
    else
        return node; // дубликаты не добавляем

    node->height = 1 + max(height(node->left), height(node->right));
    int balance = get_balance(node);

    // 4 случая разбалансировки

    // Left Left
    if (balance > 1 && data.game_num < node->left->data.game_num)
        return rotate_right(node);

    // Right Right
    if (balance < -1 && data.game_num > node->right->data.game_num)
        return rotate_left(node);

    // Left Right
    if (balance > 1 && data.game_num > node->left->data.game_num) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    // Right Left
    if (balance < -1 && data.game_num < node->right->data.game_num) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}


AVLNode* load_history_from_file(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;

    AVLNode* root = NULL;
    GameHistory hist;
    long       ts;  // временная переменная для scanf

    // Пытаемся читать ровно 7 чисел: game_num, score, duration_sec,
    // win, width, height, timestamp
    while (true) {
        int n = fscanf(f,
            "%d %d %d %d %d %d %ld",
            &hist.game_num,
            &hist.score,
            &hist.duration_sec,
            &hist.win,
            &hist.width,
            &hist.height,
            &ts
        );
        if (n == EOF) {
            // конец файла
            break;
        }
        if (n != 7) {
            // Менее семи чисел — пропускаем некорректную строку целиком
            // (например, там устаревший формат из 6 чисел).
            // Перед тем как снова вызывать fscanf, нужно сбросить остаток этой строки:
            int c;
            do {
                c = fgetc(f);
            } while (c != '\n' && c != EOF);
            continue;
        }

        // Если мы сюда попали, значит fscanf вернул ровно 7, значит ts корректный
        hist.timestamp = (time_t)ts;
        root = avl_insert(root, hist);

        if (hist.game_num >= game_counter) {  // ← ИСПРАВЛЕНО: h вместо hist
            game_counter = hist.game_num + 1;
        }
    }

    fclose(f);
    return root;
}

void free_avl_tree(AVLNode* node) {
    if (!node) return;
    free_avl_tree(node->left);
    free_avl_tree(node->right);
    free(node);
}


void save_inorder(FILE* f, AVLNode* node) {  //сохраняет всё AVL-дерево истории игр в файл
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
}

void save_history_to_file(const char* filename, AVLNode* root) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    save_inorder(f, root); // Уже реализована выше
    fclose(f);
}


int count_correct_flags(CellList* field) {
    int cnt = 0;
    for (Cell* c = field->head; c; c = c->next) {
        if (c->is_flagged && c->is_mine) cnt++;
    }
    return cnt;
}


int count_wrong_flags(CellList* field) {
    int cnt = 0;
    for (Cell* c = field->head; c; c = c->next) {
        if (c->is_flagged && !c->is_mine) cnt++;
    }
    return cnt;
}

void reveal_all_cells(CellList* field) {
    Cell* cur = field->head;
    while (cur) {
        cur->is_revealed = 1;
        cur = cur->next;
    }
}

void reset_camera_for_field(int w, int h) {
    (void)w; (void)h; // можно вообще не использовать

    g_zoom = get_min_zoom() * 1.2f;      // чуть крупнее минимального,
    if (g_zoom > 5.0f) g_zoom = 5.0f;    // но не безумно

    g_cam_x = 0.0f;
    g_cam_y = 0.0f;

    clamp_camera();
}

void get_cell_layout(float* cell_size, float* offsetX, float* offsetY) {
    if (!current_field) return;

    float size = BASE_CELL_PX * g_zoom;
    *cell_size = size;

    // камера хранится в клетках, левый верхний угол экрана:
    *offsetX = -g_cam_x * size;
    *offsetY = -g_cam_y * size;
}




int check_win(CellList* field) {
    Cell* cur = field->head;
    while (cur) {
        if (!cur->is_mine && !cur->is_revealed)
            return 0; // Есть неоткрытая безопасная клетка
        cur = cur->next;
    }
    return 1; // Все безопасные клетки открыты — победа!
}


void create_field(int width, int height, int mine_count, CellList* field) {
    // Очистка старого поля
    Cell* cur = field->head;
    while (cur) {
        Cell* to_delete = cur;
        cur = cur->next;
        free(to_delete);
    }
    field->head = field->tail = NULL;
    field->width = width;
    field->height = height;

    // Добавление клеток
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Cell* new_cell = malloc(sizeof(Cell));
            if (!new_cell) {
                fprintf(stderr, "Error: Failed to allocate memory for cell\n");
                exit(1); // или другая обработка
            }
            new_cell->x = x;
            new_cell->y = y;
            new_cell->is_mine = 0;
            new_cell->is_revealed = 0;
            new_cell->is_flagged = 0;
            new_cell->is_triggered = 0;
            new_cell->adjacent_mines = 0;
            new_cell->next = NULL;
            new_cell->prev = field->tail;

            if (field->tail) field->tail->next = new_cell;
            else field->head = new_cell;

            field->tail = new_cell;
        }
    }

    // Расстановка мин
    int placed = 0;
    while (placed < mine_count) {
        int rx = rand() % width;
        int ry = rand() % height;

        Cell* cur = field->head;
        while (cur) {
            if (cur->x == rx && cur->y == ry && !cur->is_mine) {
                cur->is_mine = 1;
                placed++;
                break;
            }
            cur = cur->next;
        }
    }

    // Подсчёт соседей
    Cell* cell = field->head;
    while (cell) {
        if (!cell->is_mine) {
            int count = 0;
            Cell* check = field->head;
            while (check) {
                if (abs(check->x - cell->x) <= 1 &&
                    abs(check->y - cell->y) <= 1 &&
                    check->is_mine) {
                    count++;
                }
                check = check->next;
            }
            cell->adjacent_mines = count;
        }
        cell = cell->next;
    }
}


float get_text_width(const char* text) {
    return strlen(text) * 8.0f;
}

void draw_panel(float x, float y, float w, float h, float alpha)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();


    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void draw_text_shadowed(const char* text, float x, float y, float scale)
{
    // тень
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text_scaled(text, x + 2, y + 2, scale);

    // основной текст
    glColor3f(1.0f, 1.0f, 1.0f);
    draw_text_scaled(text, x, y, scale);
}

void draw_centered_text(const char* text, float x, float y, float cell_size, float scale)
{
    // 1) базовые размеры в пикселях у stb_easy_font
    float rawW = strlen(text) * 8.0f;  // ширина
    float rawH = 8.0f;                 // высота – ровно 8

    // 2) итоговые размеры с учётом масштаба
    float tw = rawW * scale;
    float th = rawH * scale;

    // 3) вычисляем центрирование внутри клетки
    float px = x + (cell_size - tw) * 0.5f;
    float py = y + (cell_size - th) * 0.5f;

    // 4) округляем до целого пикселя (убирает дробные артефакты)
    px = roundf(px);
    py = roundf(py);

    // 5) рисуем
    draw_text_scaled(text, px, py, scale);
}


void draw_field(CellList* field) {
    float cell_size, offsetX, offsetY;
    get_cell_layout(&cell_size, &offsetX, &offsetY);
    float scale = cell_size / 10.0f;

    for (Cell* c = field->head; c; c = c->next) {
        float x = c->x * cell_size + offsetX;
        float y = c->y * cell_size + offsetY;
        if (x + cell_size < 0 || x > window_width ||
            y + cell_size < 0 || y > window_height) {
            continue;
        }

        // --- 1) Закрытая клетка ---
        if (!c->is_revealed) {
            // фон
            glColor3f(0.9f, 0.9f, 0.9f);
            glBegin(GL_QUADS);
            glVertex2f(x, y);
            glVertex2f(x + cell_size, y);
            glVertex2f(x + cell_size, y + cell_size);
            glVertex2f(x, y + cell_size);
            glEnd();
            // контур
            glColor3f(0, 0, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(x, y);
            glVertex2f(x + cell_size, y);
            glVertex2f(x + cell_size, y + cell_size);
            glVertex2f(x, y + cell_size);
            glEnd();
            // флаг на **закрытой** клетке
            if (c->is_flagged) {
                glColor3f(1.0f, 1.0f, 1.0f);
                draw_texture(flagTexture,
                    x, y,
                    cell_size, cell_size);
            }
            continue;
        }

        // --- 2) Открытая клетка ---
        // фон (обычный или взорванная мина)
        if (c->is_mine && c->is_triggered) {
            glColor3f(1, 0, 0);
        }
        else {
            glColor3f(0.4f, 0.4f, 0.4f);
        }
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + cell_size, y);
        glVertex2f(x + cell_size, y + cell_size);
        glVertex2f(x, y + cell_size);
        glEnd();

        // если это мина — рисуем текстуру и переходим дальше
        if (c->is_mine) {
            glColor3f(1, 1, 1);
            draw_texture(mineTexture, x, y, cell_size, cell_size);

        }

        // контур
        glColor3f(0, 0, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + cell_size, y);
        glVertex2f(x + cell_size, y + cell_size);
        glVertex2f(x, y + cell_size);
        glEnd();

        // цифра
        if (c->adjacent_mines > 0) {
            switch (c->adjacent_mines) {
            case 1: glColor3f(0.0f, 0.0f, 1.0f); break; // синий
            case 2: glColor3f(0.0f, 0.6f, 0.0f); break; // зелёный
            case 3: glColor3f(1.0f, 0.0f, 0.0f); break; // красный
            case 4: glColor3f(0.0f, 0.0f, 0.4f); break; // тёмно-синий
            case 5: glColor3f(0.5f, 0.0f, 0.0f); break; // бордовый
            case 6: glColor3f(0.0f, 0.5f, 0.5f); break;
            case 7: glColor3f(0.0f, 0.0f, 0.0f); break;
            case 8: glColor3f(0.5f, 0.5f, 0.5f); break;
            default: glColor3f(0.0f, 0.0f, 0.0f); break;
            }
            char txt[2] = { '0' + c->adjacent_mines, '\0' };
            draw_centered_text(txt, x + 8, y, cell_size, scale);
        }
    }
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_TAB) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            g_tab_held = 1;
        }
        else if (action == GLFW_RELEASE) {
            g_tab_held = 0;
        }
    }

    if (currentScreen != SCREEN_FIELD_SELECTION) return;

    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        char* target = typingWidth ? inputWidth : inputHeight;
        

        if (key == GLFW_KEY_BACKSPACE) {
            size_t len = strlen(target);
            if (len > 0) {
                target[len - 1] = '\0';
            }
        }
        else if (key == GLFW_KEY_TAB) {
            typingWidth = !typingWidth;
        }
        else if (key == GLFW_KEY_ENTER) {
            int w = atoi(inputWidth);
            int h = atoi(inputHeight);
            if (w >= 20 && h >= 20 && w <= 4000 && h <= 4000) {
                current_field = malloc(sizeof(CellList));
                if (!current_field) {
                    fprintf(stderr, "Error: Failed to allocate memory for field!\n");
                    exit(1);
                }
                current_field->head = NULL;
                current_field->tail = NULL;
                current_mine_count = (w * h) / 8;  // <--- вот здесь сохраняем
                double t0 = qpc_now_sec();
                create_field(w, h, current_mine_count, current_field); // используем переменную
                double t1 = qpc_now_sec();
                printf("create_field %dx%d mines=%d took %.3f ms\n",
                    w, h, current_mine_count, (t1 - t0) * 1000.0);
                g_cam_x = 0.0f;
                g_cam_y = 0.0f;
                g_zoom = 1.0f;
                clamp_camera();
                remaining_mines = current_mine_count;
                start_time = glfwGetTime();
                score = 0;
                currentScreen = SCREEN_GAME;
                reset_camera_for_field(w, h);
            }
        }
    }
}


void char_callback(GLFWwindow* window, unsigned int codepoint) {
    if (currentScreen != SCREEN_FIELD_SELECTION) return;

    char ch = (char)codepoint;
    if (ch >= '0' && ch <= '9') {
        char* target = typingWidth ? inputWidth : inputHeight;
        if (strlen(target) < 7) {
            size_t len = strlen(target);
            target[len] = ch;
            target[len + 1] = '\0';
        }
    }
}


void reveal_cell_recursive(Cell* cell) {
    TIME_START_NS();
    if (!cell || cell->is_revealed || cell->is_flagged) return;

    cell->is_revealed = 1;

    if (!cell->is_mine) {
        score += 10;
    }

    if (cell->adjacent_mines > 0) return;

    Cell* neighbor = current_field->head;
    while (neighbor) {
        if (abs(neighbor->x - cell->x) <= 1 &&
            abs(neighbor->y - cell->y) <= 1 &&
            !(neighbor->x == cell->x && neighbor->y == cell->y)) {
            reveal_cell_recursive(neighbor);
        }
        neighbor = neighbor->next;
    }
    TIME_END_NS("reveal_area"); 
}

float get_min_zoom(void) {
    if (!current_field) return 0.1f;

    // чтобы поле полностью перекрывало окно
    float z_w = (float)window_width / (BASE_CELL_PX * current_field->width);
    float z_h = (float)window_height / (BASE_CELL_PX * current_field->height);

    float z = fmaxf(z_w, z_h);   // берём большее из двух

    if (z < 0.01f) z = 0.01f;
    return z;
}

void clamp_camera(void) {
    if (!current_field) return;

    float size = BASE_CELL_PX * g_zoom;

    // сколько клеток помещается на экране
    float visible_cells_x = window_width / size;
    float visible_cells_y = window_height / size;

    float max_cam_x = current_field->width - visible_cells_x;
    float max_cam_y = current_field->height - visible_cells_y;

    if (max_cam_x < 0) max_cam_x = 0;
    if (max_cam_y < 0) max_cam_y = 0;

    if (g_cam_x < 0)              g_cam_x = 0;
    else if (g_cam_x > max_cam_x) g_cam_x = max_cam_x;

    if (g_cam_y < 0)              g_cam_y = 0;
    else if (g_cam_y > max_cam_y) g_cam_y = max_cam_y;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (currentScreen != SCREEN_GAME || !current_field) return;

    float old_zoom = g_zoom;
    float size_before = BASE_CELL_PX * old_zoom;

    // где мышка в "мире" (в клетках) ДО изменения зума
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    float world_x = g_cam_x + (float)(mx / size_before);
    float world_y = g_cam_y + (float)(my / size_before);

    // меняем zoom
    if (yoffset > 0)       g_zoom *= 1.1f;   // приблизить
    else if (yoffset < 0)  g_zoom /= 1.1f;   // отдалить

    // ограничиваем
    float min_zoom = get_min_zoom();
    float max_zoom = 5.0f;

    if (g_zoom < min_zoom) g_zoom = min_zoom;
    if (g_zoom > max_zoom) g_zoom = max_zoom;

    float size_after = BASE_CELL_PX * g_zoom;

    // держим под мышью ту же клетку
    g_cam_x = world_x - (float)(mx / size_after);
    g_cam_y = world_y - (float)(my / size_after);

    clamp_camera();
}


void mouse_click_on_field(double mouseX, double mouseY) {
    if (currentScreen != SCREEN_GAME || !current_field) return;

    float cell_size = 25.0f;
    float offsetX = (window_width - current_field->width * cell_size) / 2.0f;
    float offsetY = (window_height - current_field->height * cell_size) / 2.0f;

    get_cell_layout(&cell_size, &offsetX, &offsetY);

    int cx = (int)((mouseX - offsetX) / cell_size);
    int cy = (int)((mouseY - offsetY) / cell_size);

    if (cx < 0 || cx >= current_field->width || cy < 0 || cy >= current_field->height) return;

    Cell* cell = current_field->head;
    while (cell) {
        if (cell->x == cx && cell->y == cy) {
            if (!cell->is_flagged && !cell->is_revealed) {
                if (cell->is_mine) {
                    game_over = 1;
                    cell->is_triggered = 1;
                    reveal_all_cells(current_field);  // Открыть всё поле
                    end_time = glfwGetTime();
                    if (!history_saved) {
                        double time_sec = end_time - start_time;
                        int correct_flags = count_correct_flags(current_field);
                        int wrong_flags = count_wrong_flags(current_field);
                        int raw_score = correct_flags * 100 - wrong_flags * 50;
                        double max_time = current_field->width * current_field->height * 0.8;

                        double multiplier = 1.0;
                        if (game_over == 2) { // победа
                            multiplier = max_time / time_sec;
                            if (multiplier < 0.2) multiplier = 0.2; // минимальный множитель
                        }
                        int final_score = (int)(raw_score * multiplier);
                        if (final_score < 0) final_score = 0;

                        GameHistory new_game;
                        new_game.game_num = game_counter++;
                        new_game.score = final_score;
                        new_game.duration_sec = (int)time_sec;
                        new_game.win = (game_over == 2) ? 1 : 0;
                        new_game.width = current_field->width;
                        new_game.height = current_field->height;
                        new_game.timestamp = time(NULL);  // <-- вот тут записываем текущее время

                        game_history_root = avl_insert(game_history_root, new_game);
                        save_history_to_file("history.txt", game_history_root);
                        history_saved = true;
                    }
                }
                else {
                    reveal_cell_recursive(cell);
                    if (check_win(current_field)) {
                        game_over = 2; // например, 2 — победа
                        reveal_all_cells(current_field);
                        end_time = glfwGetTime();
                        if (!history_saved) {
                            double time_sec = end_time - start_time;
                            int correct_flags = count_correct_flags(current_field);
                            int wrong_flags = count_wrong_flags(current_field);
                            int raw_score = correct_flags * 100 - wrong_flags * 50;
                            double max_time = current_field->width * current_field->height * 0.8;

                            double multiplier = 1.0;
                            if (game_over == 2) { // победа
                                multiplier = max_time / time_sec;
                                if (multiplier < 0.2) multiplier = 0.2; // минимальный множитель
                            }
                            int final_score = (int)(raw_score * multiplier);
                            if (final_score < 0) final_score = 0;

                            GameHistory new_game;
                            new_game.game_num = game_counter++;
                            new_game.score = final_score;
                            new_game.duration_sec = (int)time_sec;
                            new_game.win = (game_over == 2) ? 1 : 0;
                            new_game.width = current_field->width;
                            new_game.height = current_field->height;
                            new_game.timestamp = time(NULL);  // <-- вот тут записываем текущее время

                            game_history_root = avl_insert(game_history_root, new_game);
                            save_history_to_file("history.txt", game_history_root);
                            history_saved = true;
                        }
                    }
                }
            }
            break;
        }
        cell = cell->next;
    }
}


GLuint load_texture(const char* filename, int* outWidth, int* outHeight) {
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);  // <= RGBA!

    if (!image) {
        fprintf(stderr, "Error loading image: %s\n", filename);
        return 0;
    }

    if (outWidth) *outWidth = width;    //Сохраняем высоту и ширину
    if (outHeight) *outHeight = height;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Используем GL_RGBA вместо GL_RGB
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    stbi_image_free(image);
    return texture;
}


void draw_background(GLuint texture, int imgW, int imgH) {
    float scaleX = (float)window_width / imgW;
    float scaleY = (float)window_height / imgH;
    float scale = fmaxf(scaleX, scaleY); // можно fminf, если хочешь "обрезать" по размеру

    float drawW = imgW * scale;
    float drawH = imgH * scale;

    float offsetX = (window_width - drawW) / 2;
    float offsetY = (window_height - drawH) / 2;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(offsetX, offsetY);
    glTexCoord2f(1, 0); glVertex2f(offsetX + drawW, offsetY);
    glTexCoord2f(1, 1); glVertex2f(offsetX + drawW, offsetY + drawH);
    glTexCoord2f(0, 1); glVertex2f(offsetX, offsetY + drawH);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}


void draw_texture(GLuint texture, float x, float y, float width, float height) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x, y);
    glTexCoord2f(1, 0); glVertex2f(x + width, y);
    glTexCoord2f(1, 1); glVertex2f(x + width, y + height);
    glTexCoord2f(0, 1); glVertex2f(x, y + height);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}




void draw_button(Button* btn, float scale) {
    float padding = 10.0f;

    float x = btn->x - padding;
    float y = btn->y - padding;
    float w = btn->w + padding * 2.0f;
    float h = btn->h + padding * 2.0f;

    int hovered = (hoveredButton == btn);
    int pressed = (clickedButton == btn);

    // базовая прозрачность панели
    float alpha = 0.85f;
    if (hovered) {
        alpha = pressed ? 1.0f : 0.95f;
    }

    // фон и рамка как у HUD/финальной статистики
    draw_panel(x, y, w, h, alpha);

    // текст кнопки
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text_scaled(btn->label, btn->x, btn->y, scale);
}

/*void draw_button(Button* btn, float scale) {
    int isHovered = (hoveredButton == btn);
    int isClicked = (clickedButton == btn);

    if (isHovered || isClicked) {
        glColor3f(isClicked ? 0.3f : 0.7f, 0.7f, 0.7f);
        glBegin(GL_QUADS);
        glVertex2f(btn->x - 10, btn->y - 10);
        glVertex2f(btn->x + btn->w + 10, btn->y - 10);
        glVertex2f(btn->x + btn->w + 10, btn->y + btn->h);
        glVertex2f(btn->x - 10, btn->y + btn->h);
        glEnd();
    }
    glColor3f(0.0f, 0.0f, 0.0f); // чёрный цвет текста
    draw_text_scaled(btn->label, btn->x, btn->y, scale);
}*/


int is_inside(Button* btn, double x, double y) {
    return (x >= btn->x && x <= btn->x + btn->w &&
        y >= btn->y && y <= btn->y + btn->h);
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Перемещение камеры средней кнопкой мыши
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            g_middle_dragging = 1;
            g_drag_last_x = xpos;
            g_drag_last_y = ypos;
        }
        else if (action == GLFW_RELEASE) {
            g_middle_dragging = 0;
        }
        return; // средняя кнопка только двигает камеру
    }

    if (currentScreen == SCREEN_FIELD_SELECTION && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (is_inside(&zoneWidth, xpos, ypos)) {
            typingWidth = 1;
        }
        else if (is_inside(&zoneHeight, xpos, ypos)) {
            typingWidth = 0;
        }
    }

    if (currentScreen == SCREEN_GAME && action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT) {
        float cell_size, offsetX, offsetY;
        get_cell_layout(&cell_size, &offsetX, &offsetY);

        int cx = (int)((xpos - offsetX) / cell_size);
        int cy = (int)((ypos - offsetY) / cell_size);

        if (cx < 0 || cy < 0 || cx >= current_field->width || cy >= current_field->height)
            return;

        Cell* cell = current_field->head;
        while (cell) {
            if (cell->x == cx && cell->y == cy) {
                if (!cell->is_revealed) {
                    if (!cell->is_flagged && remaining_mines > 0) {
                        // Ставим флаг
                        cell->is_flagged = 1;
                        remaining_mines--;
                    }
                    else if (cell->is_flagged) {
                        // Снимаем флаг
                        cell->is_flagged = 0;
                        remaining_mines++;
                    }
                    // Если remaining_mines == 0 и клетка не флаг — ничего не делаем (флаг не ставим)
                }
                break;
            }
            cell = cell->next;
        }

        return; // Не обрабатывать дальше
    }

    if (currentScreen == SCREEN_GAME && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

        mouse_click_on_field(xpos, ypos);
        // Проверка выхода в меню
        if (is_inside(&btnMainMenu, xpos, ypos)) {
            clickedButton = &btnMainMenu;
            return;
        }
    }

    // --- НАЖАТИЕ ---
    if (action == GLFW_PRESS) {
        if (currentScreen == SCREEN_MAIN_MENU) {
            if (is_inside(&btnExit, xpos, ypos)) clickedButton = &btnExit;
            else if (is_inside(&btnHowTo, xpos, ypos)) clickedButton = &btnHowTo;
            else if (is_inside(&btnHistory, xpos, ypos)) {
                rebuild_history_array();
                currentScreen = SCREEN_HISTORY;
                return;
            }
            else if (is_inside(&btnPlay, xpos, ypos)) clickedButton = &btnPlay;
        }

        else if (currentScreen == SCREEN_FIELD_SELECTION) {
            if (is_inside(&btnStartCustom, xpos, ypos)) clickedButton = &btnStartCustom;
            if (is_inside(&btnBack, xpos, ypos)) clickedButton = &btnBack; // --- обязательно
        }
        else if (currentScreen == SCREEN_HOW_TO_PLAY) {
            if (is_inside(&btnBack, xpos, ypos)) clickedButton = &btnBack;
        }
        else if (currentScreen == SCREEN_GAME && game_over) {
            if (is_inside(&btnRetry, xpos, ypos)) clickedButton = &btnRetry;
            else if (is_inside(&btnMainMenu, xpos, ypos)) clickedButton = &btnMainMenu;
        }
        if (currentScreen == SCREEN_HISTORY && action == GLFW_PRESS) {
            // Кнопка  Prev
            if (is_inside(&btnPrevPage, xpos, ypos) && history_page > 0) {
                history_page--;
                return;
            }
            // Кнопка Next 
            if (is_inside(&btnNextPage, xpos, ypos) && history_page < history_max_page) {
                history_page++;
                return;
            }
            // Кнопка Back из экрана истории (возвращаемся в главное меню)
            if (is_inside(&btnBackFromHistory, xpos, ypos)) {
                currentScreen = SCREEN_MAIN_MENU;
                return;
            }
            // Если клик был внутри экрана истории, но не попал по кнопкам, — просто игнорируем.
        }
        if (clickedButton == &btnRetry) {
            history_saved = false;
            create_field(current_field->width, current_field->height, current_mine_count, current_field);
            remaining_mines = current_mine_count;
            start_time = glfwGetTime();
            score = 0;
            end_time = 0;
            game_over = 0;
            reset_camera_for_field(current_field->width, current_field->height);
        }
        else if (clickedButton == &btnMainMenu) {
            history_saved = false;
            game_over = 0;
            currentScreen = SCREEN_MAIN_MENU;
        }
    }

    // --- ОТПУСКАНИЕ ---
    else if (action == GLFW_RELEASE) {
        if (clickedButton != NULL && is_inside(clickedButton, xpos, ypos)) {
            // --- Действие при клике ---
            if (clickedButton == &btnExit) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            else if (clickedButton == &btnPlay) {
                inputWidth[0] = '\0';
                inputHeight[0] = '\0';
                typingWidth = 1;
                currentScreen = SCREEN_FIELD_SELECTION;
            }
            else if (clickedButton == &btnMainMenu) {
                // Переход в меню без записи игры в историю
                history_saved = false;
                game_over = 0;
                currentScreen = SCREEN_MAIN_MENU;
            }
            else if (clickedButton == &btnHistory) {
                // просто переключаемся на экран истории
                currentScreen = SCREEN_HISTORY;
            }
            else if (clickedButton == &btnHowTo) {
                // переключаемся на экран “How To Play”
                currentScreen = SCREEN_HOW_TO_PLAY;
            }
            else if (clickedButton == &btnStartCustom) {
                int w = atoi(inputWidth);
                int h = atoi(inputHeight);
                if (w >= 20 && h >= 20 && w <= 4000 && h <= 4000) {
                    current_field = malloc(sizeof(CellList));
                    current_field->head = NULL;
                    current_field->tail = NULL;
                    current_mine_count = (w * h) / 8;
                    double t0 = qpc_now_sec();
                    create_field(w, h, (w * h) / 8, current_field);
                    double t1 = qpc_now_sec();
                    printf("create_field %dx%d mines=%d took %.3f ms\n",
                        w, h, current_mine_count, (t1 - t0) * 1000.0);
                    remaining_mines = current_mine_count;
                    start_time = glfwGetTime();
                    score = 0;
                    end_time = 0;
                    currentScreen = SCREEN_GAME;
                    reset_camera_for_field(w, h);
                }
            }

            else if (clickedButton == &btnBack) {
                currentScreen = SCREEN_MAIN_MENU;
            }
        }

        clickedButton = NULL;
    }
}




void draw_text_scaled(const char* text, float x, float y, float scale) {
    char buffer[9999];
    int quads = stb_easy_font_print(0, 0, (char*)text, NULL, buffer, sizeof(buffer)); //переводит текст в массив вершин, в буффер сохраняет коорды каждой вершины,и возвращает их кол-во

    glPushMatrix(); // Сохраняем текущую матричную трансформацию
    glTranslatef(x, y, 0);     // Перемещаем в нужную позицию
    glScalef(scale, scale, 1); // Масштабируем

    glEnableClientState(GL_VERTEX_ARRAY);      //Включаем использование массива вершин 
    glVertexPointer(2, GL_FLOAT, 16, buffer);  //Говорим, где и как лежат вершины( 2 - 2D коорды )
    glDrawArrays(GL_QUADS, 0, quads * 4);      //Рисуем мини-квадратики(пиксели) составляющие текст
    glDisableClientState(GL_VERTEX_ARRAY);     //Выключаем массив вершин

    glPopMatrix();  //Возвращаем матрицу в исходное состояние
}



int main(void) {

    
    // ЗАЩИТА 1
    if (IsDebuggerPresent()) {
        MessageBoxA(NULL, "Отладчик обнаружен!", "Ошибка", MB_ICONERROR);
        ExitProcess(EXIT_FAILURE); // Программа завершает работу при наличии отладчика
    }

    // ЗАЩИТА 2
    BOOL isDebuggerPresent = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebuggerPresent)) {
        if (isDebuggerPresent) {
            MessageBoxA(NULL, "Отладчик обнаружен!", "Ошибка", MB_ICONERROR);
            ExitProcess(EXIT_FAILURE);
        }
    }
    

    // ЗАЩИТА 3
    HMODULE hNtdll = LoadLibraryA("ntdll.dll");
    pNtQueryInformationProcess NtQueryInfoProcess = (pNtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");
    DWORD_PTR debugPort = 0;
    NTSTATUS status = NtQueryInfoProcess(GetCurrentProcess(), 7, &debugPort, sizeof(debugPort), NULL);
    if (status >= 0) {
        if (debugPort != 0) {
            MessageBoxA(NULL, "Отладчик обнаружен!", "Ошибка", MB_ICONERROR);
            ExitProcess(EXIT_FAILURE);
        }

    }

     // ЗАЩИТА 4
    /*
    AddVectoredExceptionHandler(1, VectoredExceptionHandler);
    __debugbreak();
    */

    // ЗАЩИТА 5
    void* peb;
#if defined(_WIN64)
    peb = (void*)__readgsqword(0x60);
#else
    peb = (void*)__readfsdword(0x30);
#endif
    unsigned long* forceFlags = (unsigned long*)((char*)peb + 0x70);
    if (*forceFlags != 0 && *forceFlags != 0x02) {
        MessageBoxA(NULL, "Отладчик обнаружен!", "Ошибка", MB_ICONERROR);
        ExitProcess(EXIT_FAILURE);
    }


    srand((unsigned int)time(NULL)); //инициализация случайности, чтобы каждое новое поле было разным.

    if (!glfwInit()) {
        fprintf(stderr, "Failed to implement GLFW\n");
        return -1;
    }

    game_history_root = load_history_from_file("history.txt");
    rebuild_history_array(); // чтобы заполнить history_array из дерева
    all_time_best_score = get_best_score_of_all_time();

    // Создаём окно
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int screen_width = 1680;
    int screen_height = 1020;
    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Minesweeper", NULL, NULL);
    /*glfwSetWindowPos(window,
        (mode->width - screen_width) / 2,
        (mode->height - screen_height) / 2
    );*/

    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);

    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwSetMouseButtonCallback(window, mouse_button_callback); //Регистрация обработчика событий мыши (Если пользователь кликает в каком-то месте, то потом проверяем, куда он кликнул)
    glfwSetCharCallback(window, char_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    window_width = screen_width;
    window_height = screen_height;
    g_window = window; //Сохраняем указатель на окно глобально, чтобы использовать его в других функциях

    set_window_icon(g_window);

    glfwMakeContextCurrent(window); //Т.е означает, что мы рисуем в окне, к-ое открыли

    int iconW, iconH;
    GLuint icon_texture = load_texture("mina.png", &iconW, &iconH);
    if (!icon_texture) {
        fprintf(stderr, "Failed to load icon texture\n");
        return -1;
    }

    int bgW, bgH;
    GLuint background_texture = load_texture("background.jpg", &bgW, &bgH);
    if (!background_texture) {
        fprintf(stderr, "Error background.jpg\n");
        return -1;
    }

    mineTexture = load_texture("mine.png", NULL, NULL);
    if (!mineTexture) {
        fprintf(stderr, "Failed to load mine.png\n");
        return -1;
    }

    flagTexture = load_texture("flag.png", NULL, NULL);
    if (!flagTexture) {
        fprintf(stderr, "Failed to load flag.png\n");
        return -1;
    }

    howToTexture = load_texture("howto.png", NULL, NULL);
    if (!howToTexture) {
        fprintf(stderr, "Error loading howto.png\n");
        return -1;
    }

    // Инициализируем GLEW
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    glEnable(GL_BLEND);  //включает поддержку прозрачности( Грубо говоря, чтобы если какая-то картинка без заднего фона может быть, она была в игре тоже без заднего фона)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //Это нужно для того, чтобы можно было делать прозрачный фон)

    glMatrixMode(GL_PROJECTION); //Переключаем матрицу в проекционный режим, чтобы перевести координаты в пиксели на экране
    glLoadIdentity();  //Устанавливаем начальный режим, чтобы потом задать свои координаты
    glOrtho(0, window_width, window_height, 0, -1, 1); // экранная система координат
    glMatrixMode(GL_MODELVIEW);  // Переключаем на модельно видовую матрицу( перемещение,масштаб,поворот)
    glLoadIdentity(); // Сброс всех трансформаций объектов (позиция, масштаб, поворот) — перед рисованием.

    // Основной цикл
    while (!glfwWindowShouldClose(window)) { //цикл продолжается пока пользователь не закроет окно

        double mouseX, mouseY;
        glfwGetCursorPos(g_window, &mouseX, &mouseY);
        hoveredButton = NULL;

        // Перетаскивание камеры средней кнопкой
        if (currentScreen == SCREEN_GAME && g_middle_dragging) {
            double dx = mouseX - g_drag_last_x;
            double dy = mouseY - g_drag_last_y;

            float size = BASE_CELL_PX * g_zoom;

            g_cam_x -= (float)(dx / size);
            g_cam_y -= (float)(dy / size);

            g_drag_last_x = mouseX;
            g_drag_last_y = mouseY;

            clamp_camera();
        }

        if (currentScreen == SCREEN_MAIN_MENU) {
            if (is_inside(&btnExit, mouseX, mouseY)) hoveredButton = &btnExit;
            else if (is_inside(&btnHowTo, mouseX, mouseY)) hoveredButton = &btnHowTo;
            else if (is_inside(&btnHistory, mouseX, mouseY)) hoveredButton = &btnHistory;
            else if (is_inside(&btnPlay, mouseX, mouseY)) hoveredButton = &btnPlay;
        }

        else if (currentScreen == SCREEN_HOW_TO_PLAY) {
            if (is_inside(&btnBack, mouseX, mouseY)) hoveredButton = &btnBack;
        }


        else if (currentScreen == SCREEN_FIELD_SELECTION) {
            if (is_inside(&btnStartCustom, mouseX, mouseY)) hoveredButton = &btnStartCustom;
            else if (is_inside(&zoneWidth, mouseX, mouseY)) hoveredButton = &zoneWidth;
            else if (is_inside(&zoneHeight, mouseX, mouseY)) hoveredButton = &zoneHeight;
            else if (is_inside(&btnBack, mouseX, mouseY)) hoveredButton = &btnBack;
        }

        else if (currentScreen == SCREEN_HISTORY) {
            if (is_inside(&btnPrevPage, mouseX, mouseY)) hoveredButton = &btnPrevPage;
            else if (is_inside(&btnNextPage, mouseX, mouseY)) hoveredButton = &btnNextPage;
            else if (is_inside(&btnBackFromHistory, mouseX, mouseY)) hoveredButton = &btnBackFromHistory;
        }

        else if (currentScreen == SCREEN_GAME) {
            // Кнопка Main Menu всегда доступна
            if (is_inside(&btnMainMenu, mouseX, mouseY)) hoveredButton = &btnMainMenu;
            // Если игра завершена — добавить ещё и Retry, как раньше
            if ((game_over == 1 || game_over == 2) && is_inside(&btnRetry, mouseX, mouseY)) hoveredButton = &btnRetry;
        }

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); //Цвет фона(после того как очистим) R,G,B,A A-прозрачность(пока белый)
        glClear(GL_COLOR_BUFFER_BIT); //Очищаем фон, он становится полностью белым из-за glClearColor
        draw_background(background_texture, bgW, bgH);

        if (currentScreen == SCREEN_MAIN_MENU) {
            glColor3f(0.0f, 0.0f, 0.0f); // Чёрный цвет текста
            draw_button(&btnPlay, 8.0f);
            draw_button(&btnHistory, 8.0f);
            draw_button(&btnHowTo, 8.0f);
            draw_button(&btnExit, 8.0f);
            glColor3f(1.0f, 1.0f, 1.0f);
            draw_texture(icon_texture, 1006.0f, 392.0f, 501.0f, 501.0f);
        }

        else if (currentScreen == SCREEN_FIELD_SELECTION) {
            glColor3f(0.0f, 0.0f, 0.0f); // Чёрный цвет текста

            // --- КОНСТАНТЫ ---
            const float lx = 100.0f;      // Левый край для всех лейблов
            const float lyW = 80.0f;      // Y для "Enter Width"
            const float lyH = 150.0f;     // Y для "Enter Height"
            const float labelScale = 8.0f; // Масштаб шрифта (на глаз)
            const float hint_offset = 800.0f; // Смещение вправо для подсказки "(0-40)"
            const float input_offset = 600.0f; // Смещение вправо для поля ввода
            const float arrow_offset = 570.0f; // Смещение для ">" (курсор)
            const float input_y_shift = 8.0f;  // Чтобы текст не прилипал к верху поля

            // --- Рисуем лейблы ---
            draw_text_scaled("Enter Width:", lx, lyW, labelScale);
            draw_text_scaled("Enter Height:", lx, lyH, labelScale);

            // --- Подсказки (hint) ---
            draw_text_scaled("(20-4000)", lx + hint_offset, lyW, labelScale);
            draw_text_scaled("(20-4000)", lx + hint_offset, lyH, labelScale);

            // --- Поля для ввода ---
            draw_text_scaled(inputWidth, lx + input_offset, lyW + input_y_shift, labelScale);
            draw_text_scaled(inputHeight, lx + input_offset, lyH + input_y_shift, labelScale);

            // --- Курсор ">" ---
            if (typingWidth) {
                draw_text_scaled(">", lx + arrow_offset, lyW + input_y_shift, labelScale);
            }
            else {
                draw_text_scaled(">", lx + arrow_offset, lyH + input_y_shift, labelScale);
            }

            // --- Кнопки ---
            draw_button(&btnStartCustom, 8.0f);
            draw_button(&btnBack, 8.0f);

            glColor3f(1.0f, 1.0f, 1.0f); // Белый цвет для текстуры
            draw_texture(icon_texture, 1006.0f, 392.0f, 501.0f, 501.0f);
        }

        else if (currentScreen == SCREEN_HOW_TO_PLAY) {

            glEnable(GL_TEXTURE_2D);                    //Включаем поддержку текстур
            glBindTexture(GL_TEXTURE_2D, howToTexture); //Выбираем нужную текстуру

            //Рисуем прямоугольник (QUAD) с текстурой.
            //Рисуем четырёхугольник размером 1920*1110 пикселей (на весь экран).
            //В результате текстура howToTexture будет "натянута" на этот прямоугольник, полностью покрывая его.
            glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(0.0f, 0.0f);
            glTexCoord2f(1, 0); glVertex2f(1680.0f, 0.0f);
            glTexCoord2f(1, 1); glVertex2f(1680.0f, 1020.0f);
            glTexCoord2f(0, 1); glVertex2f(0.0f, 1020.0f);
            glEnd();

            glDisable(GL_TEXTURE_2D); //Выключаем поддержку текстур

            // Рисуем кнопку BACK, чтобы можно было вернуться в главное меню
            draw_button(&btnBack, 8.0f);
        }

        if (currentScreen == SCREEN_HISTORY) {
            // 1) Заголовок экрана
            glColor3f(0, 0, 0);
            draw_text_scaled("History Games", 650, 50, 8.0f);

            if (history_count == 0) {
                draw_text_scaled("No games played yet.", 700, 200, 6.0f);
            }
            else {
                int start_idx = history_page * ENTRIES_PER_PAGE;
                int end_idx = start_idx + ENTRIES_PER_PAGE;
                if (end_idx > history_count) end_idx = history_count;

                // Базовые координаты для «первой» (самой левой) колонки
                float baseY = 170.0f;           // Чуть выше, чем раньше
                float lineH = 55.0f;            // Высота строки чуть меньше

                // Жёстко задаём X-координаты «столбиков»
                const float colNumX = 10.0f;
                const float colDimX = 150.0f;
                const float colDateX = 320.0f;
                const float colScoreX = 940.0f;
                const float colTimeX = 1170.0f;
                const float colResX = 1370.0f;

                for (int i = start_idx; i < end_idx; i++) {
                    GameHistory* h = &history_array[i];
                    float y = baseY + (i - start_idx) * lineH;
                    char tmp[64];

                    // 1) Номер игры (“#  1”)
                    snprintf(tmp, sizeof(tmp), "#%2d", h->game_num);
                    draw_text_scaled(tmp, colNumX, y, 5.0f);

                    // 2) Размер поля (“10*10”)
                    snprintf(tmp, sizeof(tmp), "%2dx%-2d         ", h->width, h->height);
                    draw_text_scaled(tmp, colDimX, y, 5.0f);

                    // 3) Дата + время (“DD.MM.YYYY HH:MM:SS”)
                    {
                        char datebuf[32];
                        struct tm* tinfo = localtime(&h->timestamp);
                        if (tinfo) {
                            strftime(datebuf, sizeof(datebuf), "%d.%m.%Y %H:%M:%S", tinfo);
                        }
                        else {
                            strncpy(datebuf, "??.??.???? ??:??:??", sizeof(datebuf) - 1);
                            datebuf[sizeof(datebuf) - 1] = '\0'; // Гарантируем нуль!
                        }
                        draw_text_scaled(datebuf, colDateX, y, 5.0f);
                    }

                    // 4) Score (“Score:  351”)
                    snprintf(tmp, sizeof(tmp), "Score:%4d", h->score);
                    draw_text_scaled(tmp, colScoreX - 100, y, 5.0f);

                    // 5) Time (“Time:  9s”)
                    snprintf(tmp, sizeof(tmp), "Time:%3ds", h->duration_sec);
                    draw_text_scaled(tmp, colTimeX - 20, y, 5.0f);

                    // 6) Результат (“WON” или “LOST”)
                    draw_text_scaled(h->win ? "WON" : "LOST", colResX + 100, y, 5.0f);
                }

                // 7) Кнопки Prev/Next, если они нужны
                if (history_page > 0) draw_button(&btnPrevPage, 5.0f);
                if (history_page < history_max_page) draw_button(&btnNextPage, 5.0f);
            }

            // Кнопка «Back» всегда снизу-слева
            draw_button(&btnBackFromHistory, 8.0f);

            // «Отдаём» кадр
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        else if (currentScreen == SCREEN_GAME) {

            // 1. Таймер и счёт
            if (game_over == 1 || game_over == 2)
                current_time = end_time - start_time;
            else
                current_time = glfwGetTime() - start_time;


            // 2. Поле
            draw_field(current_field);
            // --- Кнопка Main Menu внизу справа только пока идёт игра и зажат TAB ---
            if (!game_over &&
                glfwGetKey(g_window, GLFW_KEY_TAB) == GLFW_PRESS)
            {
                btnMainMenu.x = 1350;
                btnMainMenu.y = 930;
                draw_button(&btnMainMenu, 6.0f);
            }

            // собираем строки, как и раньше
            char timer_text[32];
            snprintf(timer_text, sizeof(timer_text), "TIMER: %02d:%02d",
                (int)current_time / 60, (int)current_time % 60);

            char bombs_text[32];
            snprintf(bombs_text, sizeof(bombs_text), "BOMBS: %d", remaining_mines);

            char score_text[32];
            snprintf(score_text, sizeof(score_text), "SCORE: %d", score);

            // показывать HUD:
            // - пока TAB зажат
            // - или всегда, если игра уже закончилась
            int showHUD =
                glfwGetKey(g_window, GLFW_KEY_TAB) == GLFW_PRESS || (game_over != 0);

            if (showHUD) {
                float hudX = 20.0f;
                float hudY = 40.0f;
                float hudW = 360.0f;
                float hudH = 180.0f;

                // фон панели
                draw_panel(hudX, hudY, hudW, hudH, 0.85f);

                glColor3f(0.0f, 0.0f, 0.0f);
                draw_text_scaled(timer_text, hudX + 10, hudY + 25, 5.0f);
                draw_text_scaled(bombs_text, hudX + 10, hudY + 75, 5.0f);
                draw_text_scaled(score_text, hudX + 10, hudY + 125, 5.0f);
            }

            if (game_over == 1 || game_over == 2)
            {
                // Получаем лучший результат до этой игры
                int prev_best_score = get_best_score_of_all_time();
                // Сообщение по центру поля
                const char* msg;
                float r, g, b;

                if (game_over == 1) {
                    msg = "YOU LOST";
                    r = 1.0f; g = 0.0f; b = 0.0f;      // красный
                }
                else {
                    msg = "YOU WON!";
                    r = 0.0f; g = 0.7f; b = 0.0f;      // приятный зелёный
                }

               

                float panelX = 700.0f;
                float panelY = 10.0f;
                float panelW = 310.0f;
                float panelH = 70.0f;

                // Рисуем белую панель
                draw_panel(panelX, panelY, panelW, panelH, 0.85f);

                // Текст по центру
                float textX = 710.0f;
                float textY = 25.0f;

                glColor3f(r, g, b);
                draw_text_scaled(msg, textX, textY, 6.0f);

                // Информация о бонусах и финальном результате (правый блок)
                char buf[128];
                double time_sec = end_time - start_time;
                int correct_flags = count_correct_flags(current_field);
                int wrong_flags = count_wrong_flags(current_field);
                int raw_score = correct_flags * 100 - wrong_flags * 50;
                double max_time = current_field->width * current_field->height * 0.8;

                double multiplier = 1.0;
                if (game_over == 2) { // победа
                    multiplier = max_time / time_sec;
                    if (multiplier < 0.2) multiplier = 0.2; // минимальный множитель
                }

                int final_score = (int)(raw_score * multiplier);
                if (final_score < 0) final_score = 0;

                int is_new_record = 0;
                if (final_score > prev_best_score) {
                    is_new_record = 1;
                }

                glColor3f(0, 0, 0);
                float infoX = 1200.0f, infoY = 180.0f, infoScale = 4.0f;
                // --- Прозрачный белый фон под блоком результатов ---
                float bgX = infoX - 40;
                float bgY = infoY + 20;
                float bgW = 500;   // ширина фона
                float bgH = 350;   // высота (или больше, если хочешь захватить все строки текста)

                draw_panel(bgX, bgY, bgW, bgH, 0.7f);
                glColor3f(0, 0, 0);

                snprintf(buf, sizeof(buf), "Correct flags: %d", correct_flags);
                draw_text_scaled(buf, infoX, infoY + 40, infoScale);
                snprintf(buf, sizeof(buf), "Wrong flags: %d", wrong_flags);
                draw_text_scaled(buf, infoX, infoY + 80, infoScale);

                if (game_over == 2) {
                    snprintf(buf, sizeof(buf), "Time: %.1fs", time_sec);
                    draw_text_scaled(buf, infoX, infoY + 120, infoScale);
                    snprintf(buf, sizeof(buf), "Multiplier: %.2f", multiplier);
                    draw_text_scaled(buf, infoX, infoY + 160, infoScale);
                }

                snprintf(buf, sizeof(buf), "Final score: %d", final_score);
                draw_text_scaled(buf, infoX, infoY + 200, infoScale * 1.1f);

                if (is_new_record) {
                    glColor3f(1, 0, 0);
                    draw_text_scaled("New record!", infoX, infoY + 280, infoScale * 1.2f);
                    glColor3f(0, 0, 0);
                }

                snprintf(buf, sizeof(buf), "Best score: %d", my_max(final_score, prev_best_score));
                draw_text_scaled(buf, infoX, infoY + 320, infoScale * 1.1f);
                btnRetry.x = 50;
                btnRetry.y = 930;
                draw_button(&btnRetry, 6.0f);

                btnMainMenu.x = 1350;
                btnMainMenu.y = 930;
                draw_button(&btnMainMenu, 6.0f);
            }
        }
        glfwSwapBuffers(window);  //Для вывода итоговой картинки( если короче и грубо )
        glfwPollEvents();//Обрабатывает события ввода(мышь, клавиатура, закрытие окна и т.д.)
    }

    // 1. Освободить клетки и поле
    if (current_field) {
        Cell* cur = current_field->head;
        while (cur) {
            Cell* next = cur->next;
            free(cur);
            cur = next;
        }
        free(current_field);
        current_field = NULL;
    }

    // 2. Освободить history_array
    if (history_array) {
        free(history_array);
        history_array = NULL;
    }

    // 3. Освободить дерево истории
    free_avl_tree(game_history_root);
    game_history_root = NULL;

    // 4. Удалить текстуры
    glDeleteTextures(1, &icon_texture);
    glDeleteTextures(1, &background_texture);
    glDeleteTextures(1, &mineTexture);
    glDeleteTextures(1, &flagTexture);
    glDeleteTextures(1, &howToTexture);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}