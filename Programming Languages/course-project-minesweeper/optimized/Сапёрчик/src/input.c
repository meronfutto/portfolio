#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "history.h"
#include "app.h"
#include "input.h"
#include "field.h"
#include "render.h"

static double qpc_now_sec(void) {
    static LARGE_INTEGER freq;
    static int inited = 0;
    if (!inited) { QueryPerformanceFrequency(&freq); inited = 1; }

    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)freq.QuadPart;
}

// -----------------------------------------------------
// Внутренняя (локальная) функция этого файла.
// static = видна только внутри input.c.
// Нужна, чтобы выставить камеру/зум так, чтобы поле выглядело нормально.
static void reset_camera_for_field(int w, int h);

// =====================================================
//                Инициализация ввода
// =====================================================

void input_init(GLFWwindow* window)
{
    // Регистрируем callback-функции GLFW.
    // После этого GLFW сам будет вызывать их при событиях ввода.
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCharCallback(window, char_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
}

// =====================================================
//                    Zoom helpers
// =====================================================

// Минимальный допустимый зум.
// Смысл: чтобы при "отдалении" поле всё ещё перекрывало окно (не становилось меньше окна).
float get_min_zoom(void)
{
    if (!current_field) return 0.1f;

    // Минимальный комфортный размер клетки 
    const float MIN_CELL_PX = 20.0f;
    float normal_zoom = MIN_CELL_PX / BASE_CELL_PX;

    // COVER: поле должно перекрывать окно (без серых полей вокруг)
    float z_w = (float)window_width / (BASE_CELL_PX * current_field->width);
    float z_h = (float)window_height / (BASE_CELL_PX * current_field->height);
    float cover_zoom = fmaxf(z_w, z_h);

    float z = fmaxf(cover_zoom, normal_zoom);

    if (z < 0.01f) z = 0.01f;
    return z;
}

// =====================================================
//                   Keyboard callback
// =====================================================

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    // ---- TAB-hud ----
    // TAB удерживается -> показываем доп. UI (например кнопку Main Menu и т.п.)
    if (key == GLFW_KEY_TAB) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            g_tab_held = 1;
        }
        else if (action == GLFW_RELEASE) {
            g_tab_held = 0;
        }
    }

    // Ниже логика ввода размеров поля нужна только на экране выбора размеров
    if (currentScreen != SCREEN_FIELD_SELECTION) return;

    // Обрабатываем удержание и обычное нажатие
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {

        // target = куда сейчас печатаем:
        // typingWidth==1 -> inputWidth, иначе inputHeight
        char* target = typingWidth ? inputWidth : inputHeight;

        // ---- Backspace: удалить последний символ ----
        if (key == GLFW_KEY_BACKSPACE) {
            size_t len = strlen(target);
            if (len > 0) {
                target[len - 1] = '\0';
            }
        }
        // ---- TAB: переключиться между вводом ширины/высоты ----
        else if (key == GLFW_KEY_TAB) {
            typingWidth = !typingWidth;
        }
        // ---- Enter: подтвердить ввод и создать поле ----
        else if (key == GLFW_KEY_ENTER) {

            int w = atoi(inputWidth);
            int h = atoi(inputHeight);

            // Ограничения (защита от недопустимых размеров)
            if (w >= 20 && h >= 20 && w <= 4000 && h <= 4000) {

                // Если старое поле есть — освобождаем память
                if (current_field) {
                    if (current_field->cells)
                        free(current_field->cells);
                    free(current_field);
                }

                // Создаём новое поле
                current_field = malloc(sizeof(Field));
                if (!current_field) {
                    fprintf(stderr, "Error: Failed to allocate memory for field!\n");
                    exit(1);
                }

                // Инициализация структуры
                current_field->width = w;
                current_field->height = h;
                current_field->cells = NULL;

                // Кол-во мин: примерно 1/8 клеток
                current_mine_count = (w * h) / 8;
                
                create_field(w, h, current_mine_count, current_field);
                
                

                // Сброс камеры
                g_cam_x = 0.0f;
                g_cam_y = 0.0f;
                g_zoom = 1.0f;
                clamp_camera();

                // Сброс состояния игры
                remaining_mines = current_mine_count;
                start_time = glfwGetTime();
                score = 0;
                end_time = 0;
                game_over = 0;

                // Переходим в игру
                currentScreen = SCREEN_GAME;

                // Подгоняем камеру/зум под размеры поля (центрируем и ставим min zoom)
                reset_camera_for_field(w, h);
            }
        }
    }
}

// =====================================================
//                  Text input callback
// =====================================================

// Callback для "ввода символов".
// Он приходит, когда пользователь печатает именно символ,
// а не просто нажимает клавиши (это важно для нормального ввода текста).
void char_callback(GLFWwindow* window, unsigned int codepoint) {
    if (currentScreen != SCREEN_FIELD_SELECTION) return;

    // Превращаем codepoint (Unicode) в char.
    // Здесь мы сознательно принимаем только цифры '0'..'9'.
    char ch = (char)codepoint;

    if (ch >= '0' && ch <= '9') {
        char* target = typingWidth ? inputWidth : inputHeight;

        // Ограничение: максимум 7 символов (потому что массив [8] включая '\0')
        if (strlen(target) < 7) {
            size_t len = strlen(target);
            target[len] = ch;
            target[len + 1] = '\0';
        }
    }
}

// =====================================================
//                 Camera clamp (ограничение)
// =====================================================

// Не даём камере уехать за границы поля.
// Камера хранится в "клетках" (не в пикселях):
// g_cam_x/g_cam_y = какая клетка находится в левом верхнем углу экрана.
void clamp_camera(void) {
    if (!current_field) return;

    // Текущий размер клетки в пикселях с учётом zoom
    float size = BASE_CELL_PX * g_zoom;

    // Сколько клеток помещается на экране по x/y
    float visible_cells_x = window_width / size;
    float visible_cells_y = window_height / size;

    // Максимальное смещение камеры: чтобы правый/нижний край не вылезали
    float max_cam_x = current_field->width - visible_cells_x;
    float max_cam_y = current_field->height - visible_cells_y;

    // Если поле меньше окна, max_cam_* будет < 0 — тогда фиксируем в 0
    if (max_cam_x < 0) max_cam_x = 0;
    if (max_cam_y < 0) max_cam_y = 0;

    // Зажимаем координаты в диапазон [0 .. max_cam]
    if (g_cam_x < 0)              g_cam_x = 0;
    else if (g_cam_x > max_cam_x) g_cam_x = max_cam_x;

    if (g_cam_y < 0)              g_cam_y = 0;
    else if (g_cam_y > max_cam_y) g_cam_y = max_cam_y;
}

// =====================================================
//                Mouse wheel (zoom)
// =====================================================

// При прокрутке колеса меняем zoom.
// Фишка: "удерживаем" под мышью ту же клетку, чтобы зум был приятный.
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {

    // Зум нужен только в игре и только если поле есть
    if (currentScreen != SCREEN_GAME || !current_field) return;

    float old_zoom = g_zoom;
    float size_before = BASE_CELL_PX * old_zoom;

    // Получаем позицию курсора в пикселях относительно окна
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    // Координаты "мира" (в клетках), на которые указывает курсор ДО изменения zoom.
    // world = cam + (mouse_px / cell_px)
    float world_x = g_cam_x + (float)(mx / size_before);
    float world_y = g_cam_y + (float)(my / size_before);

    // Меняем zoom (плавно)
    if (yoffset > 0)       g_zoom *= 1.1f;   // приблизить
    else if (yoffset < 0)  g_zoom /= 1.1f;   // отдалить

    // Ограничиваем zoom
    float min_zoom = get_min_zoom();
    float max_zoom = 3.0f;

    if (g_zoom < min_zoom) g_zoom = min_zoom;
    if (g_zoom > max_zoom) g_zoom = max_zoom;

    float size_after = BASE_CELL_PX * g_zoom;

    // Поддерживаем "ту же клетку под мышью":
    // cam = world - mouse/cell
    g_cam_x = world_x - (float)(mx / size_after);
    g_cam_y = world_y - (float)(my / size_after);

    // И после любых изменений камеры обязательно clamp
    clamp_camera();
}

// =====================================================
//            Клик по полю (левая кнопка)
// =====================================================

// Обрабатывает именно "игровую часть": открыть клетку.
// mouseX/mouseY — координаты клика в пикселях (относительно окна).
void mouse_click_on_field(double mouseX, double mouseY) {
    if (currentScreen != SCREEN_GAME || !current_field) return;

    // Узнаём, как сейчас нарисовано поле:
    // cell_size — размер клетки
    // offsetX/offsetY — сдвиг поля (например если центрируешь)
    float cell_size, offsetX, offsetY;
    get_cell_layout(&cell_size, &offsetX, &offsetY);

    // Переводим пиксели в координаты клетки
    int cx = (int)((mouseX - offsetX) / cell_size);
    int cy = (int)((mouseY - offsetY) / cell_size);

    // Если клик вне поля — ничего не делаем
    if (cx < 0 || cx >= current_field->width ||
        cy < 0 || cy >= current_field->height) {
        return;
    }

    // Берём клетку
    Cell* cell = get_cell(current_field, cx, cy);
    if (!cell) return;

    // Открывать можно только закрытую и не помеченную флагом клетку
    if (!cell->is_flagged && !cell->is_revealed) {

        // ---- Клик по мине -> проигрыш ----
        if (cell->is_mine) {
            game_over = 1;
            cell->is_triggered = 1;           // помечаем именно эту мину как "взорванную"
            reveal_all_cells(current_field);  // показываем поле целиком
            end_time = glfwGetTime();

            // Считаем статистику конца игры
            g_end_ready = 0;
            compute_end_stats(current_field, game_over);

            // Запись в историю делаем один раз
            if (!history_saved) {

                GameHistory new_game;
                new_game.game_num = game_counter++;
                new_game.score = g_end_final_score;
                new_game.duration_sec = (int)g_end_time_sec;
                new_game.win = (game_over == 2) ? 1 : 0;
                new_game.width = current_field->width;
                new_game.height = current_field->height;
                new_game.timestamp = time(NULL);

                game_history_root = avl_insert(game_history_root, new_game);
                save_history_to_file("history.txt", game_history_root);
                history_saved = true;
            }
        }
        // ---- Клик по обычной клетке ----
        else {
            // Раскрываем область (flood fill). Возвращает сколько клеток открылось.
            int opened = reveal_area(current_field, cx, cy);

            // Очки: +10 за каждую открывшуюся клетку
            score += opened * 10;

            // Если после открытия выполнено условие победы
            if (check_win(current_field)) {
                game_over = 2; // победа
                reveal_all_cells(current_field);
                end_time = glfwGetTime();

                g_end_ready = 0;
                compute_end_stats(current_field, game_over);

                if (!history_saved) {

                    GameHistory new_game;
                    new_game.game_num = game_counter++;
                    new_game.score = g_end_final_score;
                    new_game.duration_sec = (int)g_end_time_sec;
                    new_game.win = 1;
                    new_game.width = current_field->width;
                    new_game.height = current_field->height;
                    new_game.timestamp = time(NULL);

                    game_history_root = avl_insert(game_history_root, new_game);
                    save_history_to_file("history.txt", game_history_root);
                    history_saved = true;
                }
            }
        }
    }
}

// =====================================================
//                 Mouse buttons callback
// =====================================================

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    // Получаем позицию курсора
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // -------------------------------------------------
    // Средняя кнопка мыши: перетаскивание камеры
    // -------------------------------------------------
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            g_middle_dragging = 1;
            g_drag_last_x = xpos;
            g_drag_last_y = ypos;
        }
        else if (action == GLFW_RELEASE) {
            g_middle_dragging = 0;
        }
        return; // средняя кнопка только для камеры, остальное не обрабатываем
    }

    // -------------------------------------------------
    // Экран ввода размеров: клик по зонам ввода ширины/высоты
    // -------------------------------------------------
    if (currentScreen == SCREEN_FIELD_SELECTION &&
        button == GLFW_MOUSE_BUTTON_LEFT &&
        action == GLFW_PRESS)
    {
        if (is_inside(&zoneWidth, xpos, ypos)) {
            typingWidth = 1;
        }
        else if (is_inside(&zoneHeight, xpos, ypos)) {
            typingWidth = 0;
        }
    }

    // -------------------------------------------------
    // Правая кнопка мыши в игре: поставить/снять флаг
    // -------------------------------------------------
    if (currentScreen == SCREEN_GAME && action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT) {

        float cell_size, offsetX, offsetY;
        get_cell_layout(&cell_size, &offsetX, &offsetY);

        int cx = (int)((xpos - offsetX) / cell_size);
        int cy = (int)((ypos - offsetY) / cell_size);

        // Если вне поля — выходим
        if (cx < 0 || cy < 0 || cx >= current_field->width || cy >= current_field->height)
            return;

        Cell* cell = get_cell(current_field, cx, cy);
        if (!cell) return;

        // Флаг ставим только на закрытые клетки
        if (!cell->is_revealed) {

            // Ставим флаг только если остались мины "в запасе"
            if (!cell->is_flagged && remaining_mines > 0) {
                cell->is_flagged = 1;
                remaining_mines--;
            }
            else if (cell->is_flagged) {
                cell->is_flagged = 0;
                remaining_mines++;
            }
        }

        return; // Не обрабатывать дальше (это был чисто флаг)
    }

    // -------------------------------------------------
    // Левая кнопка в игре: открыть клетку
    // -------------------------------------------------

    if (currentScreen == SCREEN_GAME &&
        button == GLFW_MOUSE_BUTTON_LEFT &&
        action == GLFW_PRESS)
    {
        // Сначала пытаемся открыть клетку (если клик в поле)
        mouse_click_on_field(xpos, ypos);

        // Проверка кнопки MainMenu ТОЛЬКО если она должна быть видима
        // (игра не завершена И зажат TAB)
        if (!game_over && glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
            // Убедимся, что координаты кнопки установлены
            btnMainMenu.x = 1350;
            btnMainMenu.y = 930;

            if (is_inside(&btnMainMenu, xpos, ypos)) {
                clickedButton = &btnMainMenu;
                return;
            }
        }

        // Проверка кнопки Retry (только если игра завершена)
        if ((game_over == 1 || game_over == 2) &&
            is_inside(&btnRetry, xpos, ypos)) {
            clickedButton = &btnRetry;
            return;
        }
    }

    // =================================================
    //          Общая логика кнопок UI (PRESS)
    // =================================================

    if (action == GLFW_PRESS) {

        // Главное меню
        if (currentScreen == SCREEN_MAIN_MENU) {
            if (is_inside(&btnExit, xpos, ypos)) clickedButton = &btnExit;
            else if (is_inside(&btnHowTo, xpos, ypos)) clickedButton = &btnHowTo;
            else if (is_inside(&btnHistory, xpos, ypos)) {
                // Перед входом в историю пересобираем массив для отображения страниц
                rebuild_history_array();
                currentScreen = SCREEN_HISTORY;
                return;
            }
            else if (is_inside(&btnPlay, xpos, ypos)) clickedButton = &btnPlay;
        }

        // Экран выбора поля
        else if (currentScreen == SCREEN_FIELD_SELECTION) {
            if (is_inside(&btnStartCustom, xpos, ypos)) clickedButton = &btnStartCustom;
            if (is_inside(&btnBack, xpos, ypos)) clickedButton = &btnBack; // back обязательно
        }

        // Экран how-to
        else if (currentScreen == SCREEN_HOW_TO_PLAY) {
            if (is_inside(&btnBack, xpos, ypos)) clickedButton = &btnBack;
        }

        // Экран окончания игры (внутри SCREEN_GAME, но game_over != 0)
        else if (currentScreen == SCREEN_GAME && game_over) {
            if (is_inside(&btnRetry, xpos, ypos)) clickedButton = &btnRetry;
            else if (is_inside(&btnMainMenu, xpos, ypos)) clickedButton = &btnMainMenu;
        }

        // Экран истории
        if (currentScreen == SCREEN_HISTORY && action == GLFW_PRESS) {

            if (is_inside(&btnPrevPage, xpos, ypos) && history_page > 0) {
                history_page--;
                return;
            }
            if (is_inside(&btnNextPage, xpos, ypos) && history_page < history_max_page) {
                history_page++;
                return;
            }
            if (is_inside(&btnBackFromHistory, xpos, ypos)) {
                currentScreen = SCREEN_MAIN_MENU;
                return;
            }
        }

        // -------------------------------------------------
        // Мгновенные действия при нажатии (а не отпускании)
        // -------------------------------------------------
        /*if (clickedButton == &btnRetry) {
            // Перезапуск игры с тем же полем
            history_saved = false;
            create_field(current_field->width, current_field->height, current_mine_count, current_field);
            remaining_mines = current_mine_count;
            start_time = glfwGetTime();
            score = 0;
            end_time = 0;
            game_over = 0;
            reset_camera_for_field(current_field->width, current_field->height);
        }*/
        else if (clickedButton == &btnMainMenu) {
            // Возврат в меню
            history_saved = false;
            game_over = 0;
            currentScreen = SCREEN_MAIN_MENU;
        }
    }

    // =================================================
    //          Общая логика кнопок UI (RELEASE)
    // =================================================
    else if (action == GLFW_RELEASE) {

        // Действие срабатывает только если отпустили мышь внутри той же кнопки
        if (clickedButton != NULL && is_inside(clickedButton, xpos, ypos)) {

            if (clickedButton == &btnExit) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            else if (clickedButton == &btnRetry) {
                history_saved = false;
                create_field(current_field->width, current_field->height,
                    current_mine_count, current_field);
                remaining_mines = current_mine_count;
                start_time = glfwGetTime();
                score = 0;
                end_time = 0;
                game_over = 0;
                reset_camera_for_field(current_field->width, current_field->height);
                clickedButton = NULL;  // важно!
                return;
            }

            else if (clickedButton == &btnPlay) {
                // Подготовка ввода размеров
                inputWidth[0] = '\0';
                inputHeight[0] = '\0';
                typingWidth = 1;
                currentScreen = SCREEN_FIELD_SELECTION;
            }

            else if (clickedButton == &btnMainMenu) {
                history_saved = false;
                game_over = 0;
                currentScreen = SCREEN_MAIN_MENU;
            }

            else if (clickedButton == &btnHistory) {
                currentScreen = SCREEN_HISTORY;
            }

            else if (clickedButton == &btnHowTo) {
                currentScreen = SCREEN_HOW_TO_PLAY;
            }

            else if (clickedButton == &btnStartCustom) {
                // Создание кастомного поля по введённым размерам (то же самое что Enter)
                int w = atoi(inputWidth);
                int h = atoi(inputHeight);

                if (w >= 20 && h >= 20 && w <= 4000 && h <= 4000) {

                    if (current_field) {
                        if (current_field->cells)
                            free(current_field->cells);
                        free(current_field);
                    }

                    current_field = malloc(sizeof(Field));
                    current_field->width = w;
                    current_field->height = h;
                    current_field->cells = NULL;

                    current_mine_count = (w * h) / 8;
                    
                    create_field(w, h, current_mine_count, current_field);
                    

                    remaining_mines = current_mine_count;
                    start_time = glfwGetTime();
                    score = 0;
                    end_time = 0;
                    game_over = 0;

                    currentScreen = SCREEN_GAME;
                    reset_camera_for_field(w, h);
                }
            }

            else if (clickedButton == &btnBack) {
                currentScreen = SCREEN_MAIN_MENU;
            }
        }

        // Сбрасываем состояние "нажатой кнопки"
        clickedButton = NULL;
    }
}

// =====================================================
//        Подгон камеры под поле (центровка)
// =====================================================

static void reset_camera_for_field(int w, int h) {
    g_zoom = get_min_zoom();
    g_cam_x = 0.0f;
    g_cam_y = 0.0f;
}
