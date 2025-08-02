#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define clamp(x, lower, upper) min(max((x), (lower)), (upper))

#define WIDTH 960
#define HEIGHT 720
#define SHADERS_DIR "./shaders"

#define DEFAULT_TEXT_HEIGHT 24.f
#define PROGRAM_LIST_TEXT_HEIGHT 48.f

#define DEFAULT_PADDING 2.f
#define ROW_HEIGHT 50.f
#define ROW_PADDING 12.f
#define REC_PADDING 40.f
#define SCROLLBAR_AREA_WIDTH 20.f
#define SCROLLBAR_WIDTH 12.f
#define POPUP_WIDTH 150.f
#define POPUP_HEIGHT 100.f
#define POPUP_PADDING 10.f

typedef struct Program {
    Shader shader;
    const char *name;
    const char *relative_path;
    long last_mod;
} Program;
static Program *current_program = NULL;

#define PROGRAM_CAP 70
static struct Program_List {
    Program data[PROGRAM_CAP];
    int count;
} programs = { 0 };

typedef struct Popup {
    float life;
} Popup;

#define POPUP_CAP 5
static struct Popup_List {
    Popup data[POPUP_CAP];
    int count;
} popups;

const Rectangle scrollbar_area = {
    WIDTH - REC_PADDING - SCROLLBAR_AREA_WIDTH * 2,
    0 + REC_PADDING + ROW_PADDING,
    SCROLLBAR_AREA_WIDTH,
    HEIGHT - REC_PADDING * 2 - ROW_PADDING * 2,
};

const Rectangle scrollbar_max = {
    scrollbar_area.x + (SCROLLBAR_AREA_WIDTH - SCROLLBAR_WIDTH) / 2,
    scrollbar_area.y + (SCROLLBAR_AREA_WIDTH - SCROLLBAR_WIDTH) / 2,
    SCROLLBAR_WIDTH,
    scrollbar_area.height - (SCROLLBAR_AREA_WIDTH - SCROLLBAR_WIDTH),
};

Color get_color_alpha(int hex, unsigned char alpha) {
    return GetColor((hex << 8) | (alpha & 0xFF));
}

int main(void) {
                                     //NOTE: for development reasons
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_TOPMOST);
    InitWindow(WIDTH, HEIGHT, "shader");
    SetTargetFPS(60);

    Font font_big = LoadFontEx("resources/fonts/Alegreya-Regular.ttf", PROGRAM_LIST_TEXT_HEIGHT, NULL, 0);
    Font font_small = LoadFontEx("resources/fonts/Alegreya-Regular.ttf", DEFAULT_TEXT_HEIGHT, NULL, 0);

    Texture2D tex = LoadTextureFromImage(GenImageColor(HEIGHT, HEIGHT, WHITE));
    FilePathList files = LoadDirectoryFilesEx(SHADERS_DIR, ".fs", true);
    for (unsigned int i = 0; i < files.count; i++) {
        Program *p = &programs.data[programs.count++];
        const char *path = strdup(files.paths[i]); //TODO: memory leak
        *p = (Program){
            LoadShader(NULL, files.paths[i]),
            path + sizeof SHADERS_DIR,
            path,
            GetFileModTime(path),
        };
    }
    UnloadDirectoryFiles(files);

    float scroll = 0;
    float saved_position = -1;
    bool select_menu = true;
    Rectangle scrollbar = scrollbar_max;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouse_position = GetMousePosition();
        if (IsKeyPressed(KEY_TAB)) {
            select_menu = !select_menu;
        }

        if (IsKeyPressed(KEY_R)) {
            for (int i = 0; i < programs.count; i++) {
                Program *program = &programs.data[i];
                //TODO: handle new and deleted files
                long new_mod = GetFileModTime(program->relative_path);
                if (new_mod != program->last_mod) {
                    Shader old_shader = program->shader;
                    program->last_mod = new_mod;
                    program->shader = LoadShader(NULL, program->relative_path);
                    if (!IsShaderValid(program->shader)) {
                        //TODO: add popup system
                        printf("error in shader `%s`\n", program->relative_path);
                        program->shader = old_shader;
                    } else {
                        UnloadShader(old_shader);
                        //TODO: add popup system
                        printf("update in shader `%s`\n", program->relative_path);
                    }
                }
            }
        }

        if (select_menu) {
            scroll += GetMouseWheelMove() * dt * 800.f;
            if (IsKeyPressed(KEY_UP)) {
                scroll += 20;
            } else if (IsKeyPressed(KEY_DOWN)) {
                scroll -= 20;
            }
            int limit = max(0, (ROW_HEIGHT + ROW_PADDING) * programs.count - (HEIGHT - REC_PADDING * 2 - ROW_PADDING * 2) - ROW_PADDING);
            scroll = clamp(scroll, -limit, 0);
        }

        BeginDrawing();
        ClearBackground(GetColor(0x111111FF));
        BeginBlendMode(BLEND_ALPHA);
        if (current_program && IsShaderValid(current_program->shader)) BeginShaderMode(current_program->shader);
        DrawTexture(tex, (WIDTH - HEIGHT) / 2, 0, WHITE);
        if (current_program) EndShaderMode();

        if (select_menu) {
            int alpha = 0xBB;
            Rectangle rec = {
                0 + REC_PADDING,
                0 + REC_PADDING,
                WIDTH - REC_PADDING * 2,
                HEIGHT - REC_PADDING * 2,
            };
            DrawRectangleRounded(rec, 0.1f, 20, get_color_alpha(0x181818, alpha));
            rec.height = ROW_HEIGHT;
            rec.width -= ROW_PADDING + SCROLLBAR_AREA_WIDTH * 3;
            rec.x += ROW_PADDING;
            rec.y -= ROW_HEIGHT - scroll;

            Rectangle valid_area = {
                rec.x,
                REC_PADDING + ROW_PADDING,
                WIDTH - REC_PADDING * 2 - ROW_PADDING * 2,
                HEIGHT - REC_PADDING * 2 - ROW_PADDING * 2,
            };

            DrawRectangleRounded(scrollbar_area, 0.3f, 20, get_color_alpha(0x0B0B0B, alpha));

            {
                float total_height = max(0, (ROW_HEIGHT + ROW_PADDING) * programs.count - ROW_PADDING);
                scrollbar.height = total_height / valid_area.height > 1 ? (valid_area.height / total_height) * scrollbar_max.height : scrollbar_max.height;
                Color color = get_color_alpha(0x202020, alpha);
                if (CheckCollisionPointRec(mouse_position, scrollbar) || saved_position != -1) {
                    color = ColorBrightness(color, 0.03f);
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        saved_position = mouse_position.y - scrollbar.y;
                    }
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                        color = ColorBrightness(color, 0.03f);
                        scrollbar.y = mouse_position.y - saved_position;
                        scrollbar.y = scrollbar_max.y + clamp(scrollbar.y - scrollbar_max.y, 0, scrollbar_max.height - scrollbar.height);
                        scroll = ((scrollbar_max.y - scrollbar.y) / scrollbar_max.height) * total_height;
                    }
                    if (IsMouseButtonUp(MOUSE_LEFT_BUTTON)) {
                        saved_position = -1;
                    }
                }
                scrollbar.y = scrollbar_max.y - (scroll / total_height) * scrollbar_max.height;
                DrawRectangleRounded(scrollbar, 0.3f, 20, color);
            }

            //NOTE: pay attention when debugging
            BeginScissorMode(valid_area.x, valid_area.y, valid_area.width, valid_area.height);

            for (int i = 0; i < programs.count; i++) {
                rec.y += ROW_HEIGHT + ROW_PADDING;
                Color color = get_color_alpha(0x381818, alpha);
                if (CheckCollisionPointRec(mouse_position, valid_area) && CheckCollisionPointRec(mouse_position, rec)) {
                    color = ColorBrightness(color, 0.1f);
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                        color = ColorBrightness(color, 0.3f);
                    }
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        current_program = &programs.data[i];
                        printf("path: %s\n", current_program->relative_path);
                    }
                }
                DrawRectangleRounded(rec, 0.3f, 20, color);
                Vector2 text_position = {
                    rec.x + ROW_PADDING,
                    rec.y + (ROW_HEIGHT / 2) - (PROGRAM_LIST_TEXT_HEIGHT / 2),
                };
                DrawTextEx(font_big, TextFormat("%s", programs.data[i].name), text_position, PROGRAM_LIST_TEXT_HEIGHT, 0, get_color_alpha(0, alpha));
            }

            EndScissorMode();
        }

        for (int i = 0; i < popups.count; i++) {
            int x = WIDTH - (POPUP_PADDING + POPUP_WIDTH);
            int y = HEIGHT - (i + 1) * (POPUP_PADDING + POPUP_HEIGHT);

            DrawRectangle(x, y, POPUP_WIDTH, POPUP_HEIGHT, WHITE);
        }

        const char *program_name = current_program ? current_program->name : "null";
        const char *final_text = TextFormat("program: %s", program_name);
        Vector2 size = MeasureTextEx(font_small, final_text, DEFAULT_TEXT_HEIGHT, 0);
        Rectangle rec = {
            DEFAULT_PADDING,
            HEIGHT - (size.y + DEFAULT_PADDING * 3),
            size.x + DEFAULT_PADDING * 2,
            size.y + DEFAULT_PADDING * 2,
        };

        Color color = get_color_alpha(0xD1D0E4, 0x53);
        DrawRectangleRounded(rec, 0.3f, 20, color);
        DrawTextEx(font_small, final_text, (Vector2){DEFAULT_PADDING * 2, HEIGHT - (size.y + DEFAULT_PADDING * 2)}, DEFAULT_TEXT_HEIGHT, 0, BLACK);
        EndBlendMode();
        EndDrawing();
    }

    CloseWindow();
}
