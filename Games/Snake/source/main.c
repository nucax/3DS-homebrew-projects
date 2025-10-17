#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH  40
#define SCREEN_HEIGHT 30
#define MAX_LENGTH    300

typedef struct {
    int x, y;
} Point;

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    srand(time(NULL));

    Point snake[MAX_LENGTH];
    int length = 3;
    int dirX = 1, dirY = 0;
    int foodX = rand() % SCREEN_WIDTH;
    int foodY = rand() % SCREEN_HEIGHT;
    int score = 0;
    bool gameOver = false;

    // Initialize snake in the middle
    for (int i = 0; i < length; i++) {
        snake[i].x = SCREEN_WIDTH / 2 - i;
        snake[i].y = SCREEN_HEIGHT / 2;
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) break;
        if (gameOver && (kDown & KEY_A)) {
            // Restart
            length = 3;
            dirX = 1; dirY = 0;
            foodX = rand() % SCREEN_WIDTH;
            foodY = rand() % SCREEN_HEIGHT;
            score = 0;
            gameOver = false;
            for (int i = 0; i < length; i++) {
                snake[i].x = SCREEN_WIDTH / 2 - i;
                snake[i].y = SCREEN_HEIGHT / 2;
            }
        }

        // Movement input
        if (!gameOver) {
            if (kDown & KEY_DRIGHT && dirX != -1) { dirX = 1; dirY = 0; }
            if (kDown & KEY_DLEFT && dirX != 1)  { dirX = -1; dirY = 0; }
            if (kDown & KEY_DUP && dirY != 1)    { dirX = 0; dirY = -1; }
            if (kDown & KEY_DDOWN && dirY != -1) { dirX = 0; dirY = 1; }

            // Move body
            for (int i = length - 1; i > 0; i--) {
                snake[i] = snake[i - 1];
            }

            // Move head
            snake[0].x += dirX;
            snake[0].y += dirY;

            // Collision with walls
            if (snake[0].x < 0 || snake[0].x >= SCREEN_WIDTH ||
                snake[0].y < 0 || snake[0].y >= SCREEN_HEIGHT)
                gameOver = true;

            // Collision with self
            for (int i = 1; i < length; i++) {
                if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
                    gameOver = true;
                    break;
                }
            }

            // Eat food
            if (snake[0].x == foodX && snake[0].y == foodY) {
                if (length < MAX_LENGTH) length++;
                score++;
                foodX = rand() % SCREEN_WIDTH;
                foodY = rand() % SCREEN_HEIGHT;
            }
        }

        // Draw
        consoleClear();
        printf("=== SNAKE 3DS ===\n");
        printf("Score: %d\n", score);
        printf("Use D-Pad to move | START to quit\n");

        if (gameOver) {
            printf("\nGAME OVER!\nPress A to restart.\n");
        } else {
            for (int y = 0; y < SCREEN_HEIGHT; y++) {
                for (int x = 0; x < SCREEN_WIDTH; x++) {
                    if (x == foodX && y == foodY) {
                        printf("O");
                    } else {
                        bool printed = false;
                        for (int i = 0; i < length; i++) {
                            if (snake[i].x == x && snake[i].y == y) {
                                printf(i == 0 ? "@" : "o");
                                printed = true;
                                break;
                            }
                        }
                        if (!printed) printf(" ");
                    }
                }
                printf("\n");
            }
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();

        // Slow down
        svcSleepThread(150000000); // ~0.15s per frame
    }

    gfxExit();
    return 0;
}
