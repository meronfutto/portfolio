#pragma once
// render.h — всё, что связано с рисованием (OpenGL)
// Здесь только объявления (прототипы) и типы, нужные для рисования.
#define GLEW_STATIC                  // Чтобы правильно подключалась статическая версия библиотеки GLEW.(OpenGL)

#include <stdint.h>
#include <glew.h>
#include <glfw3.h>
#include "field.h"        // Field, Cell

// -------------------- UI --------------------

typedef struct Button {
    float x, y, w, h;
    const char* label;
} Button;

void set_window_icon(GLFWwindow* window);

// Проверка: точка внутри прямоугольника кнопки
int is_inside(const Button* btn, double x, double y);

// Рисование кнопки 
void draw_button(const Button* btn, const Button* hovered, const Button* clicked, float scale);

// -------------------- Text / Panels --------------------

float get_text_width(const char* text);

void draw_text_scaled(const char* text, float x, float y, float scale);
void draw_text_shadowed(const char* text, float x, float y, float scale);
void draw_centered_text(const char* text, float x, float y, float cell_size, float scale);

void draw_panel(float x, float y, float w, float h, float alpha);

// -------------------- Textures / Background --------------------

GLuint load_texture(const char* filename, int* outWidth, int* outHeight);
void   draw_texture(GLuint texture, float x, float y, float width, float height);
void   draw_background(GLuint texture, int imgW, int imgH);

// -------------------- Field render --------------------
// Рисует игровое поле (клетки, цифры, мины, флаги).
void draw_field(Field* field);

void get_cell_layout(float* cell_size, float* offsetX, float* offsetY);
void update_projection();