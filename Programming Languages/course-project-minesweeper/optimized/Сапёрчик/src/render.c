#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION

#include "render.h"

#include <stb_image.h>
#include <stb_easy_font.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "app.h"
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
// =====================================================
//                 Window icon
// =====================================================

// Загружает icon.png и ставит его как иконку окна.
// Если icon.png не найден, просто ничего не делает.
void set_window_icon(GLFWwindow* window) {
    GLFWimage images[1];

    // stbi_load вернёт пиксели в формате RGBA (4 канала),
    // а также заполнит width/height.
    images[0].pixels = stbi_load("assets/icon.png",
        &images[0].width,
        &images[0].height,
        0,
        4);

    if (images[0].pixels) {
        glfwSetWindowIcon(window, 1, images);
        stbi_image_free(images[0].pixels); // освобождаем память после установки
    }
}

// =====================================================
//                 UI helpers
// =====================================================

// Проверяет, находится ли точка (x,y) внутри кнопки (btn).
// Используется для hover/click по кнопкам.
int is_inside(Button* btn, double x, double y) {
    return (x >= btn->x && x <= btn->x + btn->w &&
        y >= btn->y && y <= btn->y + btn->h);
}

// Рисует кнопку: панель + текст.
// hovered/clicked используются чтобы понять состояние кнопки.
void draw_button(const Button* btn, const Button* hovered, const Button* clicked, float scale) {

    // "подложка" чуть больше текста/рамки, чтобы выглядело как кнопка
    float padding = 10.0f;

    float x = btn->x - padding;
    float y = btn->y - padding;
    float w = btn->w + padding * 2.0f;
    float h = btn->h + padding * 2.0f;

    // сравниваем указатели: hovered == btn значит курсор над этой кнопкой
    int is_hovered = (hovered == btn);
    int is_pressed = (clicked == btn);

    // прозрачность панели: при наведении/нажатии становится "живее"
    float alpha = 0.85f;
    if (is_hovered) alpha = is_pressed ? 1.0f : 0.95f;

    // рисуем прямоугольную панель
    draw_panel(x, y, w, h, alpha);

    // рисуем текст кнопки
    glColor3f(0, 0, 0);
    draw_text_scaled(btn->label, btn->x, btn->y, scale);
}

// Очень грубая оценка ширины текста для stb_easy_font:
// 1 символ ~ 8 пикселей в "сыром" масштабе.
float get_text_width(const char* text) {
    return strlen(text) * 8.0f;
}

// =====================================================
//                 Text rendering
// =====================================================

// Рисует текст с масштабом.
// stb_easy_font_print превращает строку в набор квадов (GL_QUADS).
void draw_text_scaled(const char* text, float x, float y, float scale) {

    // stb_easy_font пишет вершины в буфер: каждая вершина содержит x,y и ещё служебные байты.
    char buffer[9999];

    // Возвращает количество "квадов" (прямоугольников), из которых состоит текст.
    // Каждый quad = 4 вершины.
    int quads = stb_easy_font_print(
        0, 0,              // базовая позиция (мы потом сами сдвигаем через матрицу)
        (char*)text,
        NULL,
        buffer,
        sizeof(buffer)
    );

    // Дальше мы трансформируем текст матрицей:
    // 1) переносим в (x,y)
    // 2) масштабируем
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1);

    // Говорим OpenGL: "используй массив вершин"
    glEnableClientState(GL_VERTEX_ARRAY);

    // Вершины: 2 float (x,y), stride=16 байт (формат stb_easy_font)
    glVertexPointer(2, GL_FLOAT, 16, buffer);

    // Рисуем quads * 4 вершин
    glDrawArrays(GL_QUADS, 0, quads * 4);

    glDisableClientState(GL_VERTEX_ARRAY);

    // Возвращаем матрицу обратно
    glPopMatrix();
}

// Рисует текст с тенью: сначала чёрный текст чуть сдвинутый,
// потом белый поверх.
void draw_text_shadowed(const char* text, float x, float y, float scale)
{
    glColor3f(0.0f, 0.0f, 0.0f);
    draw_text_scaled(text, x + 2, y + 2, scale);

    glColor3f(1.0f, 1.0f, 1.0f);
    draw_text_scaled(text, x, y, scale);
}

// Рисует текст по центру внутри клетки/прямоугольника.
// cell_size — размер клетки в пикселях.
// scale — масштаб текста.
void draw_centered_text(const char* text, float x, float y, float cell_size, float scale)
{
    // stb_easy_font: высота символа 8px, ширина ~ 8px на символ
    float rawW = strlen(text) * 8.0f;
    float rawH = 8.0f;

    // Итоговые размеры с учётом scale
    float tw = rawW * scale;
    float th = rawH * scale;

    // Центрируем внутри клетки
    float px = x + (cell_size - tw) * 0.5f;
    float py = y + (cell_size - th) * 0.5f;

    // Округление убирает "мыльные" артефакты из-за дробных пикселей
    px = roundf(px);
    py = roundf(py);

    draw_text_scaled(text, px, py, scale);
}

// =====================================================
//                 Panel rendering
// =====================================================

// Рисует прямоугольную панель (заливка + рамка).
// alpha контролирует прозрачность заливки.
void draw_panel(float x, float y, float w, float h, float alpha)
{
    // Включаем blending для прозрачности
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Заливка (белая с alpha)
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    // Контур (чёрная рамка)
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// =====================================================
//                 Texture loading / drawing
// =====================================================

// Загружает картинку как OpenGL текстуру.
// Возвращает GLuint id текстуры (0 если ошибка).
GLuint load_texture(const char* filename, int* outWidth, int* outHeight) {

    int width, height, channels;

    // STBI_rgb_alpha заставляет stb вернуть RGBA (4 канала),
    // удобно для прозрачности PNG.
    unsigned char* image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

    if (!image) {
        fprintf(stderr, "Error loading image: %s\n", filename);
        return 0;
    }

    // Если попросили, возвращаем размеры картинки наружу
    if (outWidth)  *outWidth = width;
    if (outHeight) *outHeight = height;

    // Создаём текстуру в OpenGL
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Фильтрация: линейная (чуть "мягче" при масштабировании)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Загружаем данные текстуры в GPU
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA,               // внутренний формат
        width, height,
        0,
        GL_RGBA,               // формат входных данных
        GL_UNSIGNED_BYTE,
        image);

    stbi_image_free(image); // освобождаем RAM

    return texture;
}

// Рисует текстуру в прямоугольник на экране
void draw_texture(GLuint texture, float x, float y, float width, float height) {

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Координаты текстуры (0..1) по углам прямоугольника
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x, y);
    glTexCoord2f(1, 0); glVertex2f(x + width, y);
    glTexCoord2f(1, 1); glVertex2f(x + width, y + height);
    glTexCoord2f(0, 1); glVertex2f(x, y + height);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

// Рисует фоновую картинку на весь экран, сохраняя пропорции.
// Масштаб "cover": картинка полностью перекрывает окно, возможно обрезаясь.
void draw_background(GLuint texture, int imgW, int imgH) {

    // Берём текущий viewport, чтобы не зависеть от window_width/window_height
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    float winW = (float)vp[2];
    float winH = (float)vp[3];

    // Масштаб по осям
    float scaleX = winW / (float)imgW;
    float scaleY = winH / (float)imgH;

    // Cover: берём больший масштаб, чтобы перекрыть экран
    float scale = fmaxf(scaleX, scaleY);

    float drawW = (float)imgW * scale;
    float drawH = (float)imgH * scale;

    // Центрирование
    float offX = (winW - drawW) * 0.5f;
    float offY = (winH - drawH) * 0.5f;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glColor3f(1, 1, 1); // белый = "без окраски"

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(offX, offY);
    glTexCoord2f(1, 0); glVertex2f(offX + drawW, offY);
    glTexCoord2f(1, 1); glVertex2f(offX + drawW, offY + drawH);
    glTexCoord2f(0, 1); glVertex2f(offX, offY + drawH);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

// =====================================================
//                 Field rendering
// =====================================================

// Рисует поле целиком, но оптимизировано: рисуем только видимый кусок
// (по камере g_cam_x/g_cam_y и текущему zoom).
void draw_field(Field* field) {
    

    Cell* cells = field->cells; 

    // Получаем текущее отображение клеток:
    // cell_size = BASE_CELL_PX * zoom
    // offset = -cam * cell_size
    float cell_size, offsetX, offsetY;
    get_cell_layout(&cell_size, &offsetX, &offsetY);

    // Масштаб текста относительно клетки
    float scale = cell_size / 10.0f;

    // -------------------------------------------------
    // 1) вычисляем видимый прямоугольник в координатах клеток
    // -------------------------------------------------
    int x0 = (int)floorf(g_cam_x);
    int y0 = (int)floorf(g_cam_y);

    // Правая/нижняя граница: камера + сколько клеток видно на экране
    int x1 = (int)ceilf(g_cam_x + (float)window_width / cell_size) + 1;
    int y1 = (int)ceilf(g_cam_y + (float)window_height / cell_size) + 1;

    // Clamp в границы поля
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > field->width)  x1 = field->width;
    if (y1 > field->height) y1 = field->height;

    // -------------------------------------------------
    // 2) рисуем только видимые клетки
    // -------------------------------------------------
    
    for (int cy = y0; cy < y1; ++cy) {
        for (int cx = x0; cx < x1; ++cx) {

            int idx = cy * field->width + cx;
            Cell* c = &cells[idx];  //  ПРЯМОЙ ДОСТУП!
            float px = cx * cell_size + offsetX;  
            float py = cy * cell_size + offsetY;
            // ====== 1) Закрытая клетка ======
            if (!c->is_revealed) {

                // заливка
                glColor3f(0.9f, 0.9f, 0.9f);
                glBegin(GL_QUADS);
                glVertex2f(px, py);
                glVertex2f(px + cell_size, py);
                glVertex2f(px + cell_size, py + cell_size);
                glVertex2f(px, py + cell_size);
                glEnd();

                // рамка
                glColor3f(0, 0, 0);
                glBegin(GL_LINE_LOOP);
                glVertex2f(px, py);
                glVertex2f(px + cell_size, py);
                glVertex2f(px + cell_size, py + cell_size);
                glVertex2f(px, py + cell_size);
                glEnd();

                // флаг поверх
                if (c->is_flagged) {
                    glColor3f(1.0f, 1.0f, 1.0f);
                    draw_texture(flagTexture, px, py, cell_size, cell_size);
                }

                // Закрытую клетку отрисовали, дальше не идём
                continue;
            }

            // ====== 2) Открытая клетка ======
            // Триггернутую мину рисуем красным фоном, обычные открытые клетки серые
            if (c->is_mine && c->is_triggered) glColor3f(1, 0, 0);
            else                               glColor3f(0.4f, 0.4f, 0.4f);

            glBegin(GL_QUADS);
            glVertex2f(px, py);
            glVertex2f(px + cell_size, py);
            glVertex2f(px + cell_size, py + cell_size);
            glVertex2f(px, py + cell_size);
            glEnd();

            // Мина поверх
            if (c->is_mine) {
                glColor3f(1, 1, 1);
                draw_texture(mineTexture, px, py, cell_size, cell_size);
            }

            // Контур
            glColor3f(0, 0, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(px, py);
            glVertex2f(px + cell_size, py);
            glVertex2f(px + cell_size, py + cell_size);
            glVertex2f(px, py + cell_size);
            glEnd();

            // Цифра (кол-во мин рядом)
            if (c->adjacent_mines > 0) {

                // Цвет цифры как в классическом сапёре
                switch (c->adjacent_mines) {
                case 1: glColor3f(0.0f, 0.0f, 1.0f); break;
                case 2: glColor3f(0.0f, 0.6f, 0.0f); break;
                case 3: glColor3f(1.0f, 0.0f, 0.0f); break;
                case 4: glColor3f(0.0f, 0.0f, 0.4f); break;
                case 5: glColor3f(0.5f, 0.0f, 0.0f); break;
                case 6: glColor3f(0.0f, 0.5f, 0.5f); break;
                case 7: glColor3f(0.0f, 0.0f, 0.0f); break;
                case 8: glColor3f(0.5f, 0.5f, 0.5f); break;
                default: glColor3f(0.0f, 0.0f, 0.0f); break;
                }

                // Превращаем число 1..8 в строку "1".."8"
                char txt[2] = { (char)('0' + c->adjacent_mines), '\0' };

                // Рисуем по центру
                draw_centered_text(txt, px + 8, py, cell_size, scale);
            }
        }
    }
    
}


// Возвращает параметры отрисовки поля:
// cell_size — размер клетки в пикселях,
// offsetX/Y — сдвиг в пикселях, чтобы камера работала.
void get_cell_layout(float* cell_size, float* offsetX, float* offsetY) {
    if (!current_field) return;

    float size = BASE_CELL_PX * g_zoom;
    *cell_size = size;

    float field_px_w = current_field->width * size;
    float field_px_h = current_field->height * size;

    // если поле меньше экрана — центрируем
    float baseX = 0.0f;
    float baseY = 0.0f;

    if (field_px_w < window_width)
        baseX = (window_width - field_px_w) * 0.5f;

    if (field_px_h < window_height)
        baseY = (window_height - field_px_h) * 0.5f;

    *offsetX = baseX - g_cam_x * size;
    *offsetY = baseY - g_cam_y * size;
}

