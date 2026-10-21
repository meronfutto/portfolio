#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <Windows.h>

#include <glew.h>
#include <glfw3.h>

#include "field.h"     // логика игрового поля (мины, ячейки)
#include "history.h"   // история игр (результаты, рекорды)
#include "render.h"    // отрисовка (поле, кнопки, текст)

// -------------------- UI / Screens --------------------

typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_FIELD_SELECTION,
    SCREEN_GAME,
    SCREEN_HISTORY,
    SCREEN_HOW_TO_PLAY
} ScreenState;

// -------------------- End stats cache --------------------

typedef struct {
    int correct_flags;   // правильно поставленные флаги
    int wrong_flags;     // неправильно поставленные флаги
    int final_score;     // итоговый счёт за игру
    int best_score;      // лучший счёт за всё время
    int is_new_record;   // 1 если установлен новый рекорд
    double time_sec;     // время игры в секундах
    double multiplier;  // множитель очков
    int ready;           // данные готовы к отображению
} EndStats;

// -------------------- Globals (defined in app.c) --------------------

// TAB-hud
// Флаг: зажат ли TAB (показываем доп. интерфейс)
extern int g_tab_held;

// ---------- Camera / zoom ----------

// Базовый размер клетки в пикселях
extern const float BASE_CELL_PX;

// Текущий зум камеры
extern float g_zoom;

// Смещение камеры (для перетаскивания поля)
extern float g_cam_x;
extern float g_cam_y;

// ---------- Middle mouse dragging ----------

// Зажата ли средняя кнопка мыши
extern int g_middle_dragging;

// Последняя позиция мыши при перетаскивании
extern double g_drag_last_x;
extern double g_drag_last_y;

// ---------- End stats (старые поля, ещё используются) ----------

// Эти переменные ты пока оставил для совместимости
extern int    g_end_correct_flags;
extern int    g_end_wrong_flags;
extern int    g_end_final_score;
extern double g_end_time_sec;
extern double g_end_multiplier;
extern int    g_end_ready;

// Новая структура с результатами игры
extern EndStats g_end;

// ---------- Window size ----------

// Размеры окна (обновляются при ресайзе)
extern int window_width;
extern int window_height;

// ---------- OpenGL window + textures ----------

// Главное окно GLFW
extern GLFWwindow* g_window;

// Текстуры
extern GLuint mineTexture;   // мина
extern GLuint flagTexture;   // флаг
extern GLuint howToTexture;  // экран "how to play"

// ---------- Field selection input ----------

// Вводимые пользователем размеры поля (строки)
extern char inputWidth[8];
extern char inputHeight[8];

// Какое поле сейчас вводим: ширину или высоту
extern int typingWidth;

// ---------- Game state ----------

// Текущее игровое поле
extern Field* current_field;

// Количество мин на поле
extern int current_mine_count;

// Состояние игры:
// 0 — идёт игра
// 1 — проигрыш
// 2 — победа
extern int game_over;

// Тайминги
extern double start_time;
extern double current_time;
extern double end_time;

// Очки и счётчики
extern int score;
extern int game_counter;
extern int remaining_mines;

// История уже сохранена или нет
extern bool history_saved;

// Лучший счёт за всё время
extern int all_time_best_score;

// Текущий экран
extern ScreenState currentScreen;

// ---------- Hover / click state ----------

// Кнопка, на которую наведена мышь
extern Button* hoveredButton;

// Кнопка, по которой кликнули
extern Button* clickedButton;

// ---------- Buttons ----------

// Основные кнопки меню
extern Button btnExit;
extern Button btnPlay;
extern Button btnBack;
extern Button btnRetry;
extern Button btnMainMenu;
extern Button btnStartCustom;
extern Button btnHistory;
extern Button btnHowTo;

// Поля ввода размеров
extern Button zoneWidth;
extern Button zoneHeight;

// Кнопки истории
extern Button btnPrevPage;
extern Button btnNextPage;
extern Button btnBackFromHistory;