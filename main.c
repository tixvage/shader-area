#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define clamp(x, lower, upper) min(max((x), (lower)), (upper))

#define WIDTH 800
#define HEIGHT 600
#define SHADERS_DIR "./shaders"

#define DEFAULT_TEXT_HEIGHT 20.f
#define PROGRAM_LIST_TEXT_HEIGHT 30.f

#define ROW_HEIGHT 50.f
#define ROW_PADDING 10.f
#define REC_PADDING 40.f
#define SCROLLBAR_AREA_WIDTH 20.f
#define SCROLLBAR_WIDTH 12.f

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
    float saved_position;
    Rectangle scrollbar = scrollbar_max;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mouse_position = GetMousePosition();
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
                scroll += GetMouseWheelMove() * dt * 800.f;
                if (IsKeyPressed(KEY_UP)) {
                    scroll += 20;
                } else if (IsKeyPressed(KEY_DOWN)) {
                    scroll -= 20;
                }
                int limit = max(0, (ROW_HEIGHT + ROW_PADDING) * programs.count - (HEIGHT - REC_PADDING * 2 - ROW_PADDING * 2) - ROW_PADDING);
                scroll = clamp(scroll, -limit, 0);
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
                    //TODO: one of the most confusing code I ever wrote, just don't touch for now it's really working
                    float total_height = max(0, (ROW_HEIGHT + ROW_PADDING) * programs.count - ROW_PADDING);
                    scrollbar.height = total_height - valid_area.height > 0 ? scrollbar_max.height - ((total_height - valid_area.height) / valid_area.height) * scrollbar_max.height : scrollbar_max.height;
                    Color color = get_color_alpha(0x202020, alpha);
                    if (CheckCollisionPointRec(mouse_position, scrollbar)) {
                        color = ColorBrightness(color, 0.03f);
                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            saved_position = mouse_position.y - scrollbar.y;
                        }
                        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                            color = ColorBrightness(color, 0.03f);
                            scrollbar.y = mouse_position.y - saved_position;
                            scrollbar.y = scrollbar_max.y + clamp(scrollbar.y - scrollbar_max.y, 0, scrollbar_max.height - scrollbar.height);
                            scroll = ((scrollbar_max.y - scrollbar.y) / scrollbar_max.height) * valid_area.height;
                        }
                    }
                    scrollbar.y = scrollbar_max.y - (scroll / valid_area.height) * scrollbar_max.height;
                    DrawRectangleRounded(scrollbar, 0.3f, 20, color);
                }

                //NOTE: pay attention when debugging
                BeginScissorMode(valid_area.x, valid_area.y, valid_area.width, valid_area.height);

                for (int i = 0; i < programs.count; i++) {
                    rec.y += ROW_HEIGHT + ROW_PADDING;
                    Color color = get_color_alpha(0x331818, alpha);
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
                    DrawText(TextFormat("%s", programs.data[i].name), rec.x + ROW_PADDING, rec.y + (ROW_HEIGHT / 2) - (PROGRAM_LIST_TEXT_HEIGHT / 2), PROGRAM_LIST_TEXT_HEIGHT, get_color_alpha(0, alpha));
                }

                EndScissorMode();
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
