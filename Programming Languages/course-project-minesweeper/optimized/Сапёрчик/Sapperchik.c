#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <time.h>        // время для rng_seed(), history timestamp, таймер игры
#include <stdbool.h>
#include <Windows.h>
#include <math.h>

#include <glew.h>
#include <glfw3.h>

#include "app.h"
#include "rng.h"
#include "field.h"
#include "render.h"
#include "input.h"
#include "history.h"

// -----------------------------------------------------
// WinMain: точка входа Windows-приложения
// -----------------------------------------------------
int main(void)
{
    // =================================================
    // 1) RNG / история
    // =================================================

    // Инициализация rng (смешиваем текущее время и адрес переменной)
    rng_seed((uint32_t)time(NULL) ^ (uint32_t)(uintptr_t)&g_window);

    // =================================================
    // 2) GLFW init + окно
    // =================================================

    if (!glfwInit()) {
        fprintf(stderr, "Failed to init GLFW\n");
        return -1;
    }

    // Загружаем историю игр (AVL-дерево из файла)
    game_history_root = load_history_from_file("history.txt");
    rebuild_history_array();               // дерево -> массив для отображения страниц
    all_time_best_score = get_best_score_of_all_time();

    // Создаём окно (фиксированный размер)
    const int screen_width = 1680;
    const int screen_height = 1020;

    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Minesweeper", NULL, NULL);
   
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // Делаем окно нересайзабельным 
    glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);

    // Сохраняем размеры окна в глобалы (их использует рендер и камера)
    window_width = screen_width;
    window_height = screen_height;

    // Сохраняем указатель на окно глобально, чтобы пользоваться в других местах
    g_window = window;

    // Ставим иконку окна 
    set_window_icon(g_window);

    // Делаем это окно активным контекстом OpenGL
    glfwMakeContextCurrent(window);

    // Регистрируем callbacks ввода
    input_init(window);

    // =================================================
    // 3) Загрузка текстур (иконки/фон/мины/флаги)
    // =================================================

    int iconW, iconH;
    GLuint icon_texture = load_texture("assets/mina.png", &iconW, &iconH);

    int bgW, bgH;
    GLuint background_texture = load_texture("assets/background.jpg", &bgW, &bgH);

    mineTexture = load_texture("assets/mine.png", NULL, NULL);
    flagTexture = load_texture("assets/flag.png", NULL, NULL);
    howToTexture = load_texture("assets/howto.png", NULL, NULL);

    // =================================================
    // 4) GLEW init + базовая настройка OpenGL
    // =================================================

    // GLEW должен инициализироваться уже после glfwMakeContextCurrent
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Прозрачность для PNG/панелей
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Настройка 2D ортографической проекции:
    // (0,0) сверху слева, Y вниз, удобно для UI.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_width, window_height, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // =================================================
    // 5) Главный игровой цикл
    // =================================================

    while (!glfwWindowShouldClose(window))
    {
        // -------------------------------------------------
        // 5.1) Текущее положение курсора и hover логика
        // -------------------------------------------------
        double mouseX, mouseY;
        glfwGetCursorPos(g_window, &mouseX, &mouseY);

        hoveredButton = NULL;

        // -------------------------------------------------
        // 5.2) Перетаскивание камеры средней кнопкой мыши
        // -------------------------------------------------
        if (currentScreen == SCREEN_GAME && g_middle_dragging) {

            // Сдвиг мыши в пикселях
            double dx = mouseX - g_drag_last_x;
            double dy = mouseY - g_drag_last_y;

            // Перевод пикселей -> клетки
            float cell_px = BASE_CELL_PX * g_zoom;

            // Камера хранится в "клетках", поэтому делим на cell_px
            g_cam_x -= (float)(dx / cell_px);
            g_cam_y -= (float)(dy / cell_px);

            // Обновляем прошлую позицию
            g_drag_last_x = mouseX;
            g_drag_last_y = mouseY;

            // Не даём камере улететь за пределы поля
            clamp_camera();
        }

        // -------------------------------------------------
        // 5.3) Определяем hoveredButton (кнопка под мышью)
        // -------------------------------------------------
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
            if (is_inside(&btnMainMenu, mouseX, mouseY)) hoveredButton = &btnMainMenu;
            if ((game_over == 1 || game_over == 2) && is_inside(&btnRetry, mouseX, mouseY)) hoveredButton = &btnRetry;
        }

        // -------------------------------------------------
        // 5.4) Начинаем кадр: очистка + фон
        // -------------------------------------------------
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Фоновая картинка (растягивается "cover")
        draw_background(background_texture, bgW, bgH);

        // =================================================
        // 5.5) Рендер разных экранов
        // =================================================

        // -------------------- Главное меню --------------------
        if (currentScreen == SCREEN_MAIN_MENU) {

            glColor3f(0.0f, 0.0f, 0.0f);
            draw_button(&btnPlay, hoveredButton, clickedButton, 8.0f);
            draw_button(&btnHistory, hoveredButton, clickedButton, 8.0f);
            draw_button(&btnHowTo, hoveredButton, clickedButton, 8.0f);
            draw_button(&btnExit, hoveredButton, clickedButton, 8.0f);

            // Иконка игры справа
            glColor3f(1.0f, 1.0f, 1.0f);
            draw_texture(icon_texture, 1006.0f, 392.0f, 501.0f, 501.0f);
        }

        // -------------------- Выбор размеров поля --------------------
        else if (currentScreen == SCREEN_FIELD_SELECTION) {

            glColor3f(0.0f, 0.0f, 0.0f);

            // Координаты/разметка для текста
            const float lx = 100.0f;
            const float lyW = 80.0f;
            const float lyH = 150.0f;

            const float labelScale = 8.0f;
            const float hint_offset = 800.0f;
            const float input_offset = 600.0f;
            const float arrow_offset = 570.0f;
            const float input_y_shift = 8.0f;

            // Лейблы
            draw_text_scaled("Enter Width:", lx, lyW, labelScale);
            draw_text_scaled("Enter Height:", lx, lyH, labelScale);

            // Подсказки
            draw_text_scaled("(20-4000)", lx + hint_offset, lyW, labelScale);
            draw_text_scaled("(20-4000)", lx + hint_offset, lyH, labelScale);

            // Текущий ввод
            draw_text_scaled(inputWidth, lx + input_offset, lyW + input_y_shift, labelScale);
            draw_text_scaled(inputHeight, lx + input_offset, lyH + input_y_shift, labelScale);

            // Курсор ">"
            draw_text_scaled(">", lx + arrow_offset,
                (typingWidth ? lyW : lyH) + input_y_shift,
                labelScale);

            // Кнопки
            draw_button(&btnStartCustom, hoveredButton, clickedButton, 8.0f);
            draw_button(&btnBack, hoveredButton, clickedButton, 8.0f);

            // Иконка
            glColor3f(1.0f, 1.0f, 1.0f);
            draw_texture(icon_texture, 1006.0f, 392.0f, 501.0f, 501.0f);
        }

        // -------------------- How To Play --------------------
        else if (currentScreen == SCREEN_HOW_TO_PLAY) {

            // Рисуем текстуру how-to на весь экран
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, howToTexture);

            glBegin(GL_QUADS);
            glTexCoord2f(0, 0); glVertex2f(0.0f, 0.0f);
            glTexCoord2f(1, 0); glVertex2f(1680.0f, 0.0f);
            glTexCoord2f(1, 1); glVertex2f(1680.0f, 1020.0f);
            glTexCoord2f(0, 1); glVertex2f(0.0f, 1020.0f);
            glEnd();

            glDisable(GL_TEXTURE_2D);

            // Кнопка назад
            draw_button(&btnBack, hoveredButton, clickedButton, 8.0f);
        }

        // -------------------- История --------------------
        
        if (currentScreen == SCREEN_HISTORY) {
            
            
            glColor3f(0, 0, 0);
            draw_text_scaled("History Games", 650, 50, 8.0f);

            if (history_count == 0) {
                draw_text_scaled("No games played yet.", 700, 200, 6.0f);
            }
            else {
                int start_idx = history_page * ENTRIES_PER_PAGE; // первая запись, к-ый отрисовываем
                int end_idx = start_idx + ENTRIES_PER_PAGE;      // последняя запись, к-ую отрисовываем
                if (end_idx > history_count) end_idx = history_count;

                float baseY = 170.0f; // Начальная вертикальная позиция с к-ой начинается отрисовка
                float lineH = 70.0f;  // 

                // X координаты колонок
                const float colNumX = 5.0f;
                const float colDimX = 145.0f;
                const float colDateX = 400.0f;
                const float colScoreX = 1050.0f;
                const float colTimeX = 1250.0f;
                const float colResX = 1450.0f;

                float currentY = baseY;
                
                
                for (int i = start_idx; i < end_idx; i++) {
                    GameHistory* h = &history_array[i];
                    // float y = baseY + (i - start_idx) * lineH;  // УБРАНО

                    char tmp[64];

                    // #N
                    snprintf(tmp, sizeof(tmp), "#%1d", h->game_num);
                    draw_text_scaled(tmp, colNumX, currentY, 5.0f);

                    // WxH
                    snprintf(tmp, sizeof(tmp), "%2dx%-2d         ", h->width, h->height);
                    draw_text_scaled(tmp, colDimX, currentY, 5.0f);

                    
                    char datebuf[32];
                    struct tm* tinfo = localtime(&h->timestamp);
                    if (tinfo) {
                        strftime(datebuf, sizeof(datebuf), "%d.%m.%Y %H:%M:%S", tinfo);
                    }
                    else {
                        strncpy(datebuf, "??.??.???? ??:??:??", sizeof(datebuf) - 1);
                        datebuf[sizeof(datebuf) - 1] = '\0';
                    }
                    draw_text_scaled(datebuf, colDateX, currentY, 5.0f);

                    // score
                    snprintf(tmp, sizeof(tmp), "Score:%2d", h->score);
                    draw_text_scaled(tmp, colScoreX - 100, currentY, 5.0f);

                    // time
                    snprintf(tmp, sizeof(tmp), "Time:%2ds", h->duration_sec);
                    draw_text_scaled(tmp, colTimeX - 20, currentY, 5.0f);

                    // won/lost
                    draw_text_scaled(h->win ? "WON" : "LOST", colResX + 100, currentY, 5.0f);

                    currentY += lineH;  // ПРОСТОЕ СЛОЖЕНИЕ ВМЕСТО УМНОЖЕНИЯ
                }
                

                // кнопки страниц
                if (history_page > 0) draw_button(&btnPrevPage, hoveredButton, clickedButton, 5.0f);
                if (history_page < history_max_page) draw_button(&btnNextPage, hoveredButton, clickedButton, 5.0f);
            }

            // Back
            draw_button(&btnBackFromHistory, hoveredButton, clickedButton, 8.0f);
            

            // Этот экран сам завершает кадр
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        // -------------------- Игра --------------------
        else if (currentScreen == SCREEN_GAME) {

            // 1) Таймер
            if (game_over == 1 || game_over == 2)
                current_time = end_time - start_time;
            else
                current_time = glfwGetTime() - start_time;

            // 2) Поле
            draw_field(current_field);

            // 3) Кнопка Main Menu (только пока игра идёт и зажат TAB)
            if (!game_over && glfwGetKey(g_window, GLFW_KEY_TAB) == GLFW_PRESS) {
                btnMainMenu.x = 1350;
                btnMainMenu.y = 930;
                draw_button(&btnMainMenu, hoveredButton, clickedButton, 6.0f);
            }

            // 4) HUD (таймер/мины/очки)
            char timer_text[32];
            snprintf(timer_text, sizeof(timer_text), "TIMER: %02d:%02d",
                (int)current_time / 60, (int)current_time % 60);

            char bombs_text[32];
            snprintf(bombs_text, sizeof(bombs_text), "BOMBS: %d", remaining_mines);

            char score_text[32];
            snprintf(score_text, sizeof(score_text), "SCORE: %d", score);

            // Показываем HUD пока TAB зажат или игра закончена
            int showHUD = (glfwGetKey(g_window, GLFW_KEY_TAB) == GLFW_PRESS) || (game_over != 0);

            if (showHUD) {
                float hudX = 20.0f, hudY = 40.0f, hudW = 460.0f, hudH = 180.0f;

                draw_panel(hudX, hudY, hudW, hudH, 0.85f);

                glColor3f(0, 0, 0);
                draw_text_scaled(timer_text, hudX + 10, hudY + 25, 5.0f);
                draw_text_scaled(bombs_text, hudX + 10, hudY + 75, 5.0f);
                draw_text_scaled(score_text, hudX + 10, hudY + 125, 5.0f);
            }

            // 5) Экран победы/поражения (поверх игры)
            if (game_over == 1 || game_over == 2)
            {
                // Лучший результат до этой игры (для "New record!")
                int prev_best_score = get_best_score_of_all_time();

                const char* msg = (game_over == 1) ? "YOU LOST" : "YOU WON!";
                float r = (game_over == 1) ? 1.0f : 0.0f;
                float g = (game_over == 1) ? 0.0f : 0.7f;
                float b = 0.0f;

                // Панель с большим сообщением
                float panelX = 700.0f, panelY = 10.0f, panelW = 310.0f, panelH = 70.0f;
                draw_panel(panelX, panelY, panelW, panelH, 0.85f);

                glColor3f(r, g, b);
                draw_text_scaled(msg, 710.0f, 25.0f, 6.0f);

                // Блок подробной статистики справа
                double time_sec = g_end_time_sec;
                int correct_flags = g_end_correct_flags;
                int wrong_flags = g_end_wrong_flags;
                int raw_score = correct_flags * 100 - wrong_flags * 50;

                double max_time = current_field->width * current_field->height * 0.8;
                double multiplier = g_end_multiplier;

                
                if (game_over == 2) {
                    multiplier = max_time / time_sec;
                    if (multiplier < 0.2) multiplier = 0.2;
                }

                int final_score = g_end_final_score;
                if (final_score < 0) final_score = 0;

                int is_new_record = (final_score > prev_best_score);

                float infoX = 1200.0f, infoY = 180.0f, infoScale = 4.0f;

                // Фон под статистикой
                draw_panel(infoX - 40, infoY + 20, 500, 350, 0.7f);

                glColor3f(0, 0, 0);
                char buf[128];

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

                int best = (final_score > prev_best_score) ? final_score : prev_best_score;
                snprintf(buf, sizeof(buf), "Best score: %d", best);
                draw_text_scaled(buf, infoX, infoY + 320, infoScale * 1.1f);

                // Кнопки Retry/MainMenu
                btnRetry.x = 50;
                btnRetry.y = 930;
                draw_button(&btnRetry, hoveredButton, clickedButton, 6.0f);

                btnMainMenu.x = 1350;
                btnMainMenu.y = 930;
                draw_button(&btnMainMenu, hoveredButton, clickedButton, 6.0f);
            }
        }

        // -------------------------------------------------
        // 5.6) Завершаем кадр
        // -------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // =================================================
    // 6) Cleanup: освобождаем ресурсы
    // =================================================

    // Освобождаем поле
    if (current_field) {
        free(current_field->cells);
        current_field->cells = NULL;
        free(current_field);
        current_field = NULL;
    }

    // Освобождаем массив истории
    if (history_array) {
        free(history_array);
        history_array = NULL;
    }

    // Освобождаем дерево истории
    free_avl_tree(game_history_root);
    game_history_root = NULL;

    // Удаляем текстуры (если они созданы)
    glDeleteTextures(1, &icon_texture);
    glDeleteTextures(1, &background_texture);
    glDeleteTextures(1, &mineTexture);
    glDeleteTextures(1, &flagTexture);
    glDeleteTextures(1, &howToTexture);

    // Закрываем окно и завершаем GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}