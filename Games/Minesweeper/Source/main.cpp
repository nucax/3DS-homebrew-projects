// main.cpp
// Minesweeper for 3DS using sf2d + sftd + libctru
// Build with devkitPro + devkitARM and include a TTF in romfs:/fonts/DejaVuSans.ttf

#include <3ds.h>
#include <sf2d.h>
#include <sftd.h>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>

using namespace std;

static const int SCR_W = 400;  // bottom screen width
static const int SCR_H = 240;  // bottom screen height

// game settings
int GRID_W = 9;
int GRID_H = 9;
int MINES  = 10;
int TILE_SIZE = 24;
int GRID_OFFSET_X = 20;
int GRID_OFFSET_Y = 10;

enum CellState { HIDDEN, REVEALED, FLAGGED };
struct Cell {
    bool isMine;
    int adj;
    CellState state;
};

vector<Cell> grid;
bool firstClick;
bool gameOver;
bool win;
u64 startTime = 0;

sftd_font* font = NULL;

inline int idx(int x, int y){ return y*GRID_W + x; }

void initGame(int w, int h, int mines){
    GRID_W = w; GRID_H = h; MINES = mines;
    grid.assign(GRID_W*GRID_H, {false,0,HIDDEN});
    firstClick = true;
    gameOver = false;
    win = false;
}

void placeMines(int safeX, int safeY){
    // place MINES mines randomly but avoid safeX,safeY and its neighbors
    srand((unsigned)time(nullptr));
    int placed = 0;
    while(placed < MINES){
        int x = rand() % GRID_W;
        int y = rand() % GRID_H;
        if (grid[idx(x,y)].isMine) continue;

        bool tooClose = false;
        for(int sx = safeX-1; sx <= safeX+1; ++sx)
            for(int sy = safeY-1; sy <= safeY+1; ++sy)
                if (sx>=0 && sx<GRID_W && sy>=0 && sy<GRID_H)
                    if (sx==x && sy==y) tooClose = true;
        if (tooClose) continue;

        grid[idx(x,y)].isMine = true;
        placed++;
    }
    // compute adjacent counts
    for(int x=0;x<GRID_W;x++){
        for(int y=0;y<GRID_H;y++){
            if (grid[idx(x,y)].isMine) { grid[idx(x,y)].adj = -1; continue; }
            int count=0;
            for(int dx=-1; dx<=1; dx++) for(int dy=-1; dy<=1; dy++){
                int nx=x+dx, ny=y+dy;
                if (nx>=0 && nx<GRID_W && ny>=0 && ny<GRID_H)
                    if (grid[idx(nx,ny)].isMine) count++;
            }
            grid[idx(x,y)].adj = count;
        }
    }
}

void revealFlood(int sx, int sy){
    // BFS or recursive flood to reveal zero-adj areas
    if (sx<0||sx>=GRID_W||sy<0||sy>=GRID_H) return;
    if (grid[idx(sx,sy)].state == REVEALED || grid[idx(sx,sy)].state == FLAGGED) return;
    grid[idx(sx,sy)].state = REVEALED;
    if (grid[idx(sx,sy)].adj != 0) return;
    for(int dx=-1; dx<=1; dx++) for(int dy=-1; dy<=1; dy++){
        int nx=sx+dx, ny=sy+dy;
        if (nx==sx && ny==sy) continue;
        if (nx>=0 && nx<GRID_W && ny>=0 && ny<GRID_H)
            if (grid[idx(nx,ny)].state != REVEALED)
                revealFlood(nx,ny);
    }
}

void revealAllMines(){
    for(int x=0;x<GRID_W;x++) for(int y=0;y<GRID_H;y++){
        if (grid[idx(x,y)].isMine) grid[idx(x,y)].state = REVEALED;
    }
}

bool checkWin(){
    for(int i=0;i<GRID_W*GRID_H;i++){
        if (!grid[i].isMine && grid[i].state != REVEALED) return false;
    }
    return true;
}

void handleTap(int tx, int ty, bool flagMode){
    // Convert touch pos to grid coords
    int gx = (tx - GRID_OFFSET_X) / TILE_SIZE;
    int gy = (ty - GRID_OFFSET_Y) / TILE_SIZE;
    if (gx < 0 || gx >= GRID_W || gy < 0 || gy >= GRID_H) return;

    Cell &c = grid[idx(gx,gy)];

    if (flagMode){
        if (c.state == HIDDEN) c.state = FLAGGED;
        else if (c.state == FLAGGED) c.state = HIDDEN;
        return;
    }

    if (c.state == FLAGGED || c.state == REVEALED) return;

    if (firstClick){
        placeMines(gx, gy);
        firstClick = false;
        startTime = svcGetSystemTick(); // start timer (ticks)
    }

    if (c.isMine){
        // explode -> game over
        c.state = REVEALED;
        gameOver = true;
        revealAllMines();
        return;
    } else {
        revealFlood(gx, gy);
    }

    if (checkWin()){
        win = true;
        gameOver = true;
    }
}

void drawTextSimple(int x, int y, const char* text, int size=12){
    if (!font) return;
    sftd_draw_text(font, x, y, size, text);
}

void draw(){
    // draw on bottom screen
    sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
    sf2d_clear_screen();
    // background
    sf2d_draw_rectangle(0,0, SCR_W, SCR_H, RGBA8(30,30,30,255));

    // draw grid
    for(int x=0;x<GRID_W;x++){
        for(int y=0;y<GRID_H;y++){
            int px = GRID_OFFSET_X + x * TILE_SIZE;
            int py = GRID_OFFSET_Y + y * TILE_SIZE;
            Cell &c = grid[idx(x,y)];

            // base tile
            if (c.state == HIDDEN){
                sf2d_draw_rectangle(px,py, TILE_SIZE-1, TILE_SIZE-1, RGBA8(120,120,120,255));
            } else if (c.state == FLAGGED){
                sf2d_draw_rectangle(px,py, TILE_SIZE-1, TILE_SIZE-1, RGBA8(200,160,0,255));
                // small F letter
                char s[2] = {'F',0};
                sftd_draw_text(font, px+6, py+2, 18, s);
            } else { // REVEALED
                sf2d_draw_rectangle(px,py, TILE_SIZE-1, TILE_SIZE-1, RGBA8(200,200,200,255));
                if (c.isMine){
                    // draw mine as a filled circle (approx using rectangle cross)
                    sf2d_draw_rectangle(px+6, py+6, TILE_SIZE-13, 4, RGBA8(0,0,0,255));
                    sf2d_draw_rectangle(px+6+4, py+3, 4, TILE_SIZE-6, RGBA8(0,0,0,255));
                } else if (c.adj > 0){
                    char buf[8];
                    snprintf(buf, sizeof(buf), "%d", c.adj);
                    sftd_draw_text(font, px+6, py+2, 18, buf);
                }
            }
            // tile borders
            sf2d_draw_rectangle(px, py, TILE_SIZE-1, 1, RGBA8(0,0,0,255)); // top line
            sf2d_draw_rectangle(px, py, 1, TILE_SIZE-1, RGBA8(0,0,0,255)); // left
        }
    }

    // draw HUD: mines left, timer, instructions
    char hud[128];
    int flags = 0;
    for(auto &c : grid) if (c.state == FLAGGED) flags++;
    int minesLeft = MINES - flags;
    u64 ticks = (startTime==0) ? 0 : svcGetSystemTick() - startTime;
    u64 seconds = ticks ? (ticks / 268123480) : 0; // approx ticks -> seconds for 3DS (approx; can be adapted)
    snprintf(hud, sizeof(hud), "Mines: %d  Time: %llu s", minesLeft, seconds);
    sftd_draw_text(font, 10, SCR_H - 20, 14, hud);

    if (gameOver){
        const char* msg = win ? "You Win! Press START to restart." : "Game Over! Press START to restart.";
        sftd_draw_text(font, 10, SCR_H - 40, 16, msg);
    } else {
        sftd_draw_text(font, 10, SCR_H - 40, 14, "Tap to reveal. Hold or press R to flag.");
    }

    sf2d_end_frame();
    sf2d_swapbuffers();
}

int main(int argc, char* argv[]){
    gfxInitDefault();
    sf2d_init();
    sftd_init();
    romfsInit();
    font = sftd_load_font_file("romfs:/fonts/DejaVuSans.ttf");
    if (!font) {
        // fallback: try smaller path or continue without text
    }

    initGame(9,9,10);

    bool running = true;
    bool flagMode = false;
    u64 lastTouchTime = 0;
    bool touchHeld = false;

    while (aptMainLoop() && running){
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START){
            initGame(GRID_W, GRID_H, MINES);
        }
        if (kDown & KEY_SELECT){
            // quick change difficulty cycle (9x9 -> 16x16 -> custom)
            if (GRID_W==9){ initGame(16,16,40); TILE_SIZE = 12; GRID_OFFSET_X = 8; GRID_OFFSET_Y = 8; }
            else { initGame(9,9,10); TILE_SIZE = 24; GRID_OFFSET_X = 20; GRID_OFFSET_Y = 10; }
        }
        if (kDown & KEY_SELECT) { /* changed above */ }
        if (kDown & KEY_TOUCH) {
            touchPosition touch;
            hidTouchRead(&touch);
            int tx = touch.px;
            int ty = touch.py;
            // immediate tap: reveal; if R is held or long press -> flag
            bool isR = (kHeld & KEY_R);
            u64 now = svcGetSystemTick();
            // detect long-press
            if (!touchHeld){
                touchHeld = true;
                lastTouchTime = now;
            } else {
                u64 diff = now - lastTouchTime;
                // approx threshold for ~0.5s; svc ticks approx 268123480 per sec -> threshold = ~0.5*268M = 134M
                if (diff > 134061740) isR = true;
            }
            if (kDown & KEY_TOUCH){
                // on touch down
                handleTap(tx, ty, isR);
            }
        } else {
            touchHeld = false;
        }

        if (kDown & KEY_R){
            // Also use R as a toggle: short press toggles flag mode for next tap
            flagMode = !flagMode;
        }

        // update draw
        draw();

        // exit
        if (kDown & KEY_START && (hidKeysHeld() & KEY_L)) {
            // e.g., start+L to exit quickly (optional)
        }

        // handle home button to exit to menu
        if (kDown & KEY_HOME) running = false;

        // small delay is handled by sf2d_swapbuffers & vblank wait
    }

    // cleanup
    romfsExit();
    sftd_free_font(font);
    sftd_fini();
    sf2d_fini();
    gfxExit();
    return 0;
}
