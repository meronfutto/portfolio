#include "app.h"

// -------------------- TAB-hud --------------------
int g_tab_held = 0;

// -------------------- Camera / zoom --------------------
const float BASE_CELL_PX = 25.0f;
float g_zoom = 1.0f;
float g_cam_x = 0.0f;
float g_cam_y = 0.0f;

// -------------------- Middle mouse dragging --------------------
int    g_middle_dragging = 0;
double g_drag_last_x = 0.0;
double g_drag_last_y = 0.0;

// -------------------- End stats cache (old fields you still use) --------------------
int    g_end_correct_flags = 0;
int    g_end_wrong_flags = 0;
int    g_end_final_score = 0;
double g_end_time_sec = 0.0;
double g_end_multiplier = 1.0;
int    g_end_ready = 0;

EndStats g_end = { 0 };

// -------------------- Window size --------------------
int window_width = 1680;
int window_height = 1020;

// -------------------- OpenGL window + textures --------------------
GLFWwindow* g_window = NULL;
GLuint mineTexture = 0;
GLuint flagTexture = 0;
GLuint howToTexture = 0;

// -------------------- Field selection input --------------------
char inputWidth[8] = "";
char inputHeight[8] = "";
int  typingWidth = 1;

// -------------------- Game state --------------------
Field* current_field = NULL;

int     current_mine_count = 0;
int     game_over = 0;
double  start_time = 0.0;
double  current_time = 0.0;
double  end_time = 0.0;
int     score = 0;
int     game_counter = 1;
int     remaining_mines = 0;
bool    history_saved = false;

int     all_time_best_score = 0;

// -------------------- Screen state --------------------
ScreenState currentScreen = SCREEN_MAIN_MENU;

// -------------------- Hover/click state --------------------
Button* hoveredButton = NULL;
Button* clickedButton = NULL;

// -------------------- Buttons --------------------
Button btnExit = { 100, 400, 150, 60, "Exit" };
Button btnPlay = { 100, 100, 170, 70, "Play" };
Button btnBack = { 30, 940, 190, 60, "Back" };

Button btnRetry = { 0, 0, 180, 50, "Retry" };
Button btnMainMenu = { 0, 0, 300, 50, "Main Menu" };

Button btnStartCustom = { 100, 250, 240, 65, "START" };

Button zoneWidth = { 650, 80, 400, 50, "" };
Button zoneHeight = { 650, 150, 400, 50, "" };

Button btnHistory = { 100, 200, 560, 70, "History games" };
Button btnHowTo = { 100, 300, 480, 70, "How to play" };

// History navigation buttons
Button btnPrevPage = { 1200, 940, 120, 40, "Prev" };
Button btnNextPage = { 1400, 940, 120, 40, "Next" };
Button btnBackFromHistory = { 30, 900, 180, 80, "Back" };