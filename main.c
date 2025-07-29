#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"

#define WIDTH 800
#define HEIGHT 600
#define SHADERS_DIR "./shaders"

#define DEFAULT_TEXT_HEIGHT 20.f
#define PROGRAM_LIST_TEXT_HEIGHT 30.f

typedef struct Program {
    Shader shader;
    const char *name;
    const char *relative_path;
    long last_mod;
} Program;
static Program *current_program = NULL;

#define PROGRAM_CAP 50
static struct Program_List {
    Program data[PROGRAM_CAP];
    int count;
} programs = { 0 };

typedef enum Mode {
    Mode_RENDER = 0,
    Mode_MENU,

    Mode_Count,
} Mode;
static Mode mode = Mode_MENU;

Color get_color_alpha(int hex, unsigned char alpha) {
    return GetColor((hex << 8) | (alpha & 0xFF));
}

int main(void) {
                                     //NOTE: for development reasons
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_TOPMOST);
    InitWindow(WIDTH, HEIGHT, "shader");
    SetTargetFPS(60);

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

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (IsKeyPressed(KEY_TAB)) {
            mode++;
            if (mode >= Mode_Count) mode = 0;
            switch (mode) {
                case Mode_RENDER: {} break;
                case Mode_MENU: {
                    scroll = 0;
                } break;
                default: break;
            }
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

        switch (mode) {
            case Mode_RENDER: {} break;
            case Mode_MENU: {
                scroll += GetMouseWheelMove() * dt * 200.f;
                if (IsKeyPressed(KEY_UP)) {
                    scroll += 5;
                } else if (IsKeyPressed(KEY_DOWN)) {
                    scroll -= 5;
                }
            } break;
            default: break;
        }
        BeginDrawing();
        ClearBackground(GetColor(0x111111FF));
        if (current_program && IsShaderValid(current_program->shader)) BeginShaderMode(current_program->shader);
        DrawTexture(tex, (WIDTH - HEIGHT) / 2, 0, WHITE);
        if (current_program) EndShaderMode();
        switch (mode) {
            case Mode_RENDER: {} break;
            case Mode_MENU: {
                BeginBlendMode(BLEND_ALPHA);
                int padding = 40;
                int alpha = 0xBB;
                Rectangle rec = {
                    0 + padding,
                    0 + padding,
                    WIDTH - padding * 2,
                    HEIGHT - padding * 2,
                };
                DrawRectangleRounded(rec, 0.1f, 20, get_color_alpha(0x181818, alpha));
                int row_height = 50;
                padding = 10;
                rec.height = row_height;
                rec.width -= padding * 2;
                rec.x += padding;
                rec.y -= row_height - scroll;

                for (int i = 0; i < programs.count; i++) {
                    rec.y += row_height + padding;
                    Color color = get_color_alpha(0x331818, alpha);
                    if (CheckCollisionPointRec(GetMousePosition(), rec)) {
                        color = ColorBrightness(color, 0.1f);
                        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                            color = ColorBrightness(color, 0.3f);
                        }
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            current_program = &programs.data[i];
                            printf("path: %s\n", current_program->relative_path);
                        }
                    }
                    DrawRectangleRec(rec, color);
                    DrawText(TextFormat("%s", programs.data[i].name), rec.x + padding, rec.y + (row_height / 2) - (PROGRAM_LIST_TEXT_HEIGHT / 2), PROGRAM_LIST_TEXT_HEIGHT, get_color_alpha(0, alpha));
                }
                EndBlendMode();
            } break;
            default: break;
        }
        const char *program_name = current_program ? current_program->name : "null";
        DrawText(TextFormat("program: %s", program_name), 5, HEIGHT - (DEFAULT_TEXT_HEIGHT + 5), DEFAULT_TEXT_HEIGHT, BLACK);
        DrawFPS(5, 5);
        EndDrawing();
    }

    CloseWindow();
}
