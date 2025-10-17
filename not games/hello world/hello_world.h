#include <3ds.h>
#include <stdio.h>

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("Hello, 3DS World!");

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}
