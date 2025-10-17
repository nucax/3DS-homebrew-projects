#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include <time.h>

#define CELL_SIZE 8
#define GRID_WIDTH  40
#define GRID_HEIGHT 30
#define MAX_LENGTH  300

typedef struct {
    int x, y;
} Point;

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;

int main() {
    // Init systems
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

    // Colors
    u32 colBG     = C2D_Color32(25, 20, 30, 255);
    u32 colSnake  = C2D_Color32(80, 200, 120, 255);
    u32 colHead   = C2D_Color32(50, 255, 80, 255);
    u32 colFood   = C2D_Color32(255, 80, 80, 255);
    u32 colText   = C2D_Color32(255, 255, 255, 255);

    // Game state
    Point snake[MAX_LENGTH];
    int length = 3;
    Direction dir = DIR_RIGHT;
    bool gameOver = false;
    int foodX = rand() % GRID_WIDTH;
    int foodY = rand() % GRID_HEIGHT;
    int score = 0;
    srand(time(NULL));

    // Init snake in center
    for (int i = 0; i < length; i++) {
        snake[i].x = GRID_WIDTH/2 - i;
        snake[i].y = GRID_HEIGHT/2;
    }

    // Main loop
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) break;
        if (gameOver && (kDown & KEY_A)) {
            // Reset game
            length = 3;
            dir = DIR_RIGHT;
            foodX = rand() % GRID_WIDTH;
            foodY = rand() % GRID_HEIGHT;
            score = 0;
            gameOver = false;
            for (int i = 0; i < length; i++) {
                snake[i].x = GRID_WIDTH/2 - i;
                snake[i].y = GRID_HEIGHT/2;
            }
        }

        if (!gameOver) {
            // Input
            if ((kDown & KEY_DUP) && dir != DIR_DOWN) dir = DIR_UP;
            else if ((kDown & KEY_DDOWN) && dir != DIR_UP) dir = DIR_DOWN;
            else if ((kDown & KEY_DLEFT) && dir != DIR_RIGHT) dir = DIR_LEFT;
            else if ((kDown & KEY_DRIGHT) && dir != DIR_LEFT) dir = DIR_RIGHT;

            // Move body
            for (int i = length - 1; i > 0; i--)
                snake[i] = snake[i - 1];

            // Move head
            if (dir == DIR_UP) snake[0].y--;
            if (dir == DIR_DOWN) snake[0].y++;
            if (dir == DIR_LEFT) snake[0].x--;
            if (dir == DIR_RIGHT) snake[0].x++;

            // Check collisions
            if (snake[0].x < 0 || snake[0].x >= GRID_WIDTH ||
                snake[0].y < 0 || snake[0].y >= GRID_HEIGHT)
                gameOver = true;

            for (int i = 1; i < length; i++)
                if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
                    gameOver = true;

            // Eat food
            if (snake[0].x == foodX && snake[0].y == foodY) {
                if (length < MAX_LENGTH) length++;
                score++;
                foodX = rand() % GRID_WIDTH;
                foodY = rand() % GRID_HEIGHT;
            }
        }

        // --- DRAW ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, colBG);
        C2D_SceneBegin(top);

        // Draw food
        C2D_DrawRectSolid(foodX * CELL_SIZE, foodY * CELL_SIZE, 0,
                          CELL_SIZE, CELL_SIZE, colFood);

        // Draw snake
        for (int i = 0; i < length; i++) {
            u32 color = (i == 0) ? colHead : colSnake;
            C2D_DrawRectSolid(snake[i].x * CELL_SIZE,
                              snake[i].y * CELL_SIZE,
                              0, CELL_SIZE, CELL_SIZE, color);
        }

        // Text
        C2D_TextBuf buf = C2D_TextBufNew(64);
        C2D_Text text;
        char msg[64];
        if (!gameOver)
            snprintf(msg, sizeof(msg), "Score: %d", score);
        else
            snprintf(msg, sizeof(msg), "Game Over! Score: %d\nPress A to restart", score);
        C2D_TextParse(&text, buf, msg);
        C2D_TextOptimize(&text);
        C2D_DrawText(&text, C2D_WithColor, 2, 2, 0.5f, 0.6f, 0.6f, colText);
        C2D_TextBufDelete(buf);

        C3D_FrameEnd(0);

        svcSleepThread(100000000); // 0.1s per frame
    }

    // Clean up
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
