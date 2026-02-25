#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

// ============================================================
// Windows Console — ncurses-compatible shim
// (replaces #include <ncurses.h>)
// ============================================================

// Key codes (replacing ncurses KEY_*)
#define KEY_LEFT   0x10000
#define KEY_RIGHT  0x10001
#define KEY_UP     0x10002
#define KEY_DOWN   0x10003
#define ERR        (-1)

// Attribute flags (replacing ncurses A_*)
#define A_BOLD        0x20000000
#define A_DIM         0x10000000
#define COLOR_PAIR(n) (0x40000000 | ((n) & 0xFF))

// ncurses color constants
#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

// Screen size
static const int SCR_W = 80;
static const int SCR_H = 30;

static HANDLE    hOut;
static HANDLE    hIn;
static CHAR_INFO screenBuf[SCR_H][SCR_W];
static WORD      colorPairs[16]  = {};
static int       g_curPair       = 0;
static bool      g_bold          = false;
static bool      g_dim           = false;
static bool      g_nodelay_mode  = false;
static void*     stdscr          = nullptr;

// Map ncurses color index -> Windows foreground bits
static WORD fgBits(int c) {
    const WORD t[8] = {
        0,
        FOREGROUND_RED,
        FOREGROUND_GREEN,
        FOREGROUND_RED | FOREGROUND_GREEN,
        FOREGROUND_BLUE,
        FOREGROUND_RED | FOREGROUND_BLUE,
        FOREGROUND_GREEN | FOREGROUND_BLUE,
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    };
    return (c >= 0 && c < 8) ? t[c] : 0;
}

// Map ncurses color index -> Windows background bits
static WORD bgBits(int c) {
    const WORD t[8] = {
        0,
        BACKGROUND_RED,
        BACKGROUND_GREEN,
        BACKGROUND_RED | BACKGROUND_GREEN,
        BACKGROUND_BLUE,
        BACKGROUND_RED | BACKGROUND_BLUE,
        BACKGROUND_GREEN | BACKGROUND_BLUE,
        BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
    };
    return (c >= 0 && c < 8) ? t[c] : 0;
}

static WORD currentWinAttr() {
    WORD w = colorPairs[g_curPair];
    if (g_bold) w |= FOREGROUND_INTENSITY;
    return w;
}

// ---- ncurses init / cleanup stubs ----

static void initscr() {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hIn  = GetStdHandle(STD_INPUT_HANDLE);

    // Disable echo and line-input
    SetConsoleMode(hIn, 0);
    SetConsoleTitleA("Tetris");

    // Shrink window first so buffer resize can succeed
    SMALL_RECT tiny = {0, 0, 1, 1};
    SetConsoleWindowInfo(hOut, TRUE, &tiny);

    COORD bufSize = {(SHORT)SCR_W, (SHORT)SCR_H};
    SetConsoleScreenBufferSize(hOut, bufSize);

    SMALL_RECT win = {0, 0, (SHORT)(SCR_W - 1), (SHORT)(SCR_H - 1)};
    SetConsoleWindowInfo(hOut, TRUE, &win);

    memset(screenBuf, 0, sizeof(screenBuf));
}

static void cbreak()           {}
static void noecho()           {}
static void keypad(void*, int) {}

static void curs_set(int) {
    CONSOLE_CURSOR_INFO ci = {};
    ci.dwSize   = 1;
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &ci);
}

static bool has_colors()  { return true; }
static void start_color() {}

static void init_pair(int n, int fg, int bg) {
    if (n >= 0 && n < 16)
        colorPairs[n] = fgBits(fg) | bgBits(bg);
}

static void nodelay(void*, int nd) { g_nodelay_mode = (nd != 0); }

// ---- Attribute control ----

static void attron(int a) {
    if (a & 0x40000000) g_curPair = a & 0xFF;
    if (a & A_BOLD)     g_bold    = true;
    if (a & A_DIM)      g_dim     = true;
}

static void attroff(int a) {
    if (a & 0x40000000) g_curPair = 0;   // reset to default pair
    if (a & A_BOLD)     g_bold    = false;
    if (a & A_DIM)      g_dim     = false;
}

// ---- Screen buffer helpers ----

static void scr_put(int y, int x, char ch) {
    if (y < 0 || y >= SCR_H || x < 0 || x >= SCR_W) return;
    screenBuf[y][x].Char.AsciiChar = ch;
    screenBuf[y][x].Attributes     = currentWinAttr();
}

static void mvaddch(int y, int x, char ch) { scr_put(y, x, ch); }

static void mvaddstr(int y, int x, const char* s) {
    while (*s) scr_put(y, x++, *s++);
}

static void mvprintw(int y, int x, const char* fmt, ...) {
    char    buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    mvaddstr(y, x, buf);
}

static void erase() {
    for (int r = 0; r < SCR_H; r++)
        for (int c = 0; c < SCR_W; c++) {
            screenBuf[r][c].Char.AsciiChar = ' ';
            screenBuf[r][c].Attributes     = 0;
        }
}

static void refresh() {
    COORD      sz  = {(SHORT)SCR_W, (SHORT)SCR_H};
    COORD      org = {0, 0};
    SMALL_RECT dst = {0, 0, (SHORT)(SCR_W - 1), (SHORT)(SCR_H - 1)};
    WriteConsoleOutputA(hOut, (CHAR_INFO*)screenBuf, sz, org, &dst);
}

static void napms(int ms) { Sleep((DWORD)ms); }

static void endwin() {
    CONSOLE_CURSOR_INFO ci = {};
    ci.dwSize   = 25;
    ci.bVisible = TRUE;
    SetConsoleCursorInfo(hOut, &ci);
    SetConsoleTextAttribute(hOut, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    SetConsoleMode(hIn, ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
}

// Non-blocking or blocking getch depending on g_nodelay_mode
static int getch() {
    if (g_nodelay_mode) {
        DWORD n = 0;
        if (!GetNumberOfConsoleInputEvents(hIn, &n) || n == 0) return ERR;
        INPUT_RECORD ir;
        DWORD        rd = 0;
        while (n-- > 0) {
            ReadConsoleInputA(hIn, &ir, 1, &rd);
            if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) continue;
            switch (ir.Event.KeyEvent.wVirtualKeyCode) {
                case VK_LEFT:  return KEY_LEFT;
                case VK_RIGHT: return KEY_RIGHT;
                case VK_UP:    return KEY_UP;
                case VK_DOWN:  return KEY_DOWN;
            }
            if (ir.Event.KeyEvent.uChar.AsciiChar)
                return (unsigned char)ir.Event.KeyEvent.uChar.AsciiChar;
        }
        return ERR;
    } else {
        for (;;) {
            INPUT_RECORD ir;
            DWORD        rd = 0;
            ReadConsoleInputA(hIn, &ir, 1, &rd);
            if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) continue;
            switch (ir.Event.KeyEvent.wVirtualKeyCode) {
                case VK_LEFT:  return KEY_LEFT;
                case VK_RIGHT: return KEY_RIGHT;
                case VK_UP:    return KEY_UP;
                case VK_DOWN:  return KEY_DOWN;
            }
            if (ir.Event.KeyEvent.uChar.AsciiChar)
                return (unsigned char)ir.Event.KeyEvent.uChar.AsciiChar;
        }
    }
}

// ============================================================
// Tetris game
// ============================================================

// Board dimensions
static const int BOARD_WIDTH  = 10;
static const int BOARD_HEIGHT = 20;

// Screen offsets
static const int BOARD_X = 2;
static const int BOARD_Y = 1;

// Each piece: 7 types, 4 rotations, 4 rows, 4 cols
// 1 = filled cell, 0 = empty
static const int PIECES[7][4][4][4] = {
    // I
    {
        {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}},
        {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}},
    },
    // O
    {
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    },
    // T
    {
        {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{1,1,1,0},{0,1,0,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}},
    },
    // S
    {
        {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
        {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
    },
    // Z
    {
        {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
    },
    // J
    {
        {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
        {{1,1,1,0},{0,0,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}},
    },
    // L
    {
        {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
        {{1,1,1,0},{1,0,0,0},{0,0,0,0},{0,0,0,0}},
        {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
    },
};

// ncurses color pairs
enum Colors {
    CP_EMPTY  = 0,
    CP_I      = 1,
    CP_O      = 2,
    CP_T      = 3,
    CP_S      = 4,
    CP_Z      = 5,
    CP_J      = 6,
    CP_L      = 7,
    CP_BORDER = 8,
    CP_GHOST  = 9,
    CP_UI     = 10,
};

struct Piece {
    int type;     // 0-6
    int rotation; // 0-3
    int x, y;     // board column/row of the piece's top-left corner
};

class Tetris {
public:
    int  board[BOARD_HEIGHT][BOARD_WIDTH]; // 0=empty, 1-7=piece color
    Piece current;
    Piece next;
    long long score;
    int  level;
    int  lines_cleared;
    bool game_over;
    bool paused;

private:
    std::mt19937 rng;

public:
    Tetris() : score(0), level(1), lines_cleared(0), game_over(false), paused(false) {
        memset(board, 0, sizeof(board));
        rng.seed((unsigned)std::chrono::steady_clock::now().time_since_epoch().count());
        next = make_piece();
        new_piece();
    }

    Piece make_piece() {
        Piece p;
        p.type     = rng() % 7;
        p.rotation = 0;
        p.x        = BOARD_WIDTH / 2 - 2;
        p.y        = 0;
        return p;
    }

    // Return whether board cell (col=bx, row=by) is filled by piece p
    bool cell(const Piece& p, int row, int col) const {
        return PIECES[p.type][p.rotation][row][col] != 0;
    }

    bool is_valid(const Piece& p) const {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (!cell(p, r, c)) continue;
                int bx = p.x + c;
                int by = p.y + r;
                if (bx < 0 || bx >= BOARD_WIDTH)  return false;
                if (by >= BOARD_HEIGHT)            return false;
                if (by >= 0 && board[by][bx])      return false;
            }
        }
        return true;
    }

    void new_piece() {
        current = next;
        next    = make_piece();
        if (!is_valid(current))
            game_over = true;
    }

    void lock_piece() {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (!cell(current, r, c)) continue;
                int bx = current.x + c;
                int by = current.y + r;
                if (by >= 0 && by < BOARD_HEIGHT && bx >= 0 && bx < BOARD_WIDTH)
                    board[by][bx] = current.type + 1;
            }
        }
        int cleared = clear_lines();
        update_score(cleared);
        new_piece();
    }

    int clear_lines() {
        int count = 0;
        for (int r = BOARD_HEIGHT - 1; r >= 0; ) {
            bool full = true;
            for (int c = 0; c < BOARD_WIDTH; c++) {
                if (!board[r][c]) { full = false; break; }
            }
            if (full) {
                for (int rr = r; rr > 0; rr--)
                    memcpy(board[rr], board[rr - 1], sizeof(board[0]));
                memset(board[0], 0, sizeof(board[0]));
                count++;
            } else {
                r--;
            }
        }
        return count;
    }

    void update_score(int cleared) {
        static const int base[] = {0, 100, 300, 500, 800};
        lines_cleared += cleared;
        score         += (long long)base[std::min(cleared, 4)] * level;
        level          = lines_cleared / 10 + 1;
    }

    int drop_interval() const {
        int ms = 1000 - (level - 1) * 80;
        return std::max(ms, 50);
    }

    Piece ghost_piece() const {
        Piece g = current;
        while (true) {
            Piece tmp = g;
            tmp.y++;
            if (!is_valid(tmp)) break;
            g = tmp;
        }
        return g;
    }

    bool move_left()  { Piece p = current; p.x--; if (is_valid(p)) { current = p; return true; } return false; }
    bool move_right() { Piece p = current; p.x++; if (is_valid(p)) { current = p; return true; } return false; }
    bool move_down()  { Piece p = current; p.y++; if (is_valid(p)) { current = p; return true; } return false; }

    void hard_drop() { while (move_down()); lock_piece(); }

    void rotate_cw() {
        Piece p = current;
        p.rotation = (p.rotation + 1) % 4;
        for (int kick : {0, -1, 1, -2, 2}) {
            p.x = current.x + kick;
            if (is_valid(p)) { current = p; return; }
        }
    }

    void rotate_ccw() {
        Piece p = current;
        p.rotation = (p.rotation + 3) % 4;
        for (int kick : {0, -1, 1, -2, 2}) {
            p.x = current.x + kick;
            if (is_valid(p)) { current = p; return; }
        }
    }

};

// ---- Drawing ---------------------------------------------------------------

void draw_cell(int scr_y, int scr_x, int cp) {
    attron(COLOR_PAIR(cp));
    mvaddstr(scr_y, scr_x, "[]");
    attroff(COLOR_PAIR(cp));
}

void draw_board(const Tetris& g) {
    // Border
    attron(COLOR_PAIR(CP_BORDER));
    for (int r = 0; r <= BOARD_HEIGHT; r++) {
        mvaddch(BOARD_Y + r, BOARD_X - 1, '|');
        mvaddch(BOARD_Y + r, BOARD_X + BOARD_WIDTH * 2, '|');
    }
    for (int c = -1; c <= BOARD_WIDTH * 2; c++)
        mvaddch(BOARD_Y + BOARD_HEIGHT, BOARD_X + c, '-');
    attroff(COLOR_PAIR(CP_BORDER));

    // Placed cells
    for (int r = 0; r < BOARD_HEIGHT; r++) {
        for (int c = 0; c < BOARD_WIDTH; c++) {
            int sy = BOARD_Y + r, sx = BOARD_X + c * 2;
            int color = g.board[r][c];
            if (color) {
                draw_cell(sy, sx, color);
            } else {
                attron(COLOR_PAIR(CP_EMPTY));
                mvaddstr(sy, sx, "  ");
                attroff(COLOR_PAIR(CP_EMPTY));
            }
        }
    }

    // Ghost
    Piece ghost = g.ghost_piece();
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (!g.cell(ghost, r, c)) continue;
            int bx = ghost.x + c, by = ghost.y + r;
            if (by < 0 || by >= BOARD_HEIGHT || bx < 0 || bx >= BOARD_WIDTH) continue;
            if (g.board[by][bx]) continue;
            attron(COLOR_PAIR(CP_GHOST) | A_DIM);
            mvaddstr(BOARD_Y + by, BOARD_X + bx * 2, "[]");
            attroff(COLOR_PAIR(CP_GHOST) | A_DIM);
        }
    }

    // Active piece
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (!g.cell(g.current, r, c)) continue;
            int bx = g.current.x + c, by = g.current.y + r;
            if (by < 0 || by >= BOARD_HEIGHT || bx < 0 || bx >= BOARD_WIDTH) continue;
            draw_cell(BOARD_Y + by, BOARD_X + bx * 2, g.current.type + 1);
        }
    }
}

void draw_next(const Tetris& g) {
    int px = BOARD_X + BOARD_WIDTH * 2 + 4;
    int py = BOARD_Y + 1;

    attron(COLOR_PAIR(CP_UI));
    mvprintw(py, px, "NEXT:");
    attroff(COLOR_PAIR(CP_UI));

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (g.cell(g.next, r, c))
                draw_cell(py + 1 + r, px + c * 2, g.next.type + 1);
            else {
                attron(COLOR_PAIR(CP_EMPTY));
                mvaddstr(py + 1 + r, px + c * 2, "  ");
                attroff(COLOR_PAIR(CP_EMPTY));
            }
        }
    }
}

void draw_ui(const Tetris& g) {
    int px = BOARD_X + BOARD_WIDTH * 2 + 4;
    int py = BOARD_Y + 7;

    attron(COLOR_PAIR(CP_UI));
    mvprintw(py,      px, "SCORE:");
    mvprintw(py + 1,  px, "%lld",  g.score);
    mvprintw(py + 3,  px, "LEVEL:");
    mvprintw(py + 4,  px, "%d",    g.level);
    mvprintw(py + 6,  px, "LINES:");
    mvprintw(py + 7,  px, "%d",    g.lines_cleared);
    mvprintw(py + 9,  px, "--- KEYS ---");
    mvprintw(py + 10, px, "Left/Right:Move");
    mvprintw(py + 11, px, "Up   : Rotate CW");
    mvprintw(py + 12, px, "z    : Rotate CCW");
    mvprintw(py + 13, px, "Down : Soft Drop");
    mvprintw(py + 14, px, "Space: Hard Drop");
    mvprintw(py + 15, px, "p    : Pause");
    mvprintw(py + 16, px, "q    : Quit");
    attroff(COLOR_PAIR(CP_UI));
}

// ---- Main ------------------------------------------------------------------

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        init_pair(CP_EMPTY,  COLOR_BLACK,   COLOR_BLACK);
        init_pair(CP_I,      COLOR_BLACK,   COLOR_CYAN);
        init_pair(CP_O,      COLOR_BLACK,   COLOR_YELLOW);
        init_pair(CP_T,      COLOR_BLACK,   COLOR_MAGENTA);
        init_pair(CP_S,      COLOR_BLACK,   COLOR_GREEN);
        init_pair(CP_Z,      COLOR_BLACK,   COLOR_RED);
        init_pair(CP_J,      COLOR_BLACK,   COLOR_BLUE);
        init_pair(CP_L,      COLOR_WHITE,   COLOR_WHITE);
        init_pair(CP_BORDER, COLOR_WHITE,   COLOR_BLACK);
        init_pair(CP_GHOST,  COLOR_WHITE,   COLOR_BLACK);
        init_pair(CP_UI,     COLOR_WHITE,   COLOR_BLACK);
    }

    Tetris game;
    auto last_drop = std::chrono::steady_clock::now();

    while (!game.game_over) {
        int ch = getch();

        if (!game.paused) {
            switch (ch) {
                case KEY_LEFT:          game.move_left();   break;
                case KEY_RIGHT:         game.move_right();  break;
                case KEY_DOWN:          game.move_down();   break;
                case KEY_UP:            game.rotate_cw();   break;
                case 'z': case 'Z':     game.rotate_ccw();  break;
                case ' ':               game.hard_drop();   break;
                case 'p': case 'P':     game.paused = true; break;
                case 'q': case 'Q':     game.game_over = true; break;
                default: break;
            }
        } else {
            if (ch == 'p' || ch == 'P') {
                game.paused = false;
                last_drop   = std::chrono::steady_clock::now();
            } else if (ch == 'q' || ch == 'Q') {
                game.game_over = true;
            }
        }

        if (!game.paused && !game.game_over) {
            auto now     = std::chrono::steady_clock::now();
            int  elapsed = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_drop).count();
            if (elapsed >= game.drop_interval()) {
                if (!game.move_down())
                    game.lock_piece();
                last_drop = std::chrono::steady_clock::now();
            }
        }

        erase();
        draw_board(game);
        draw_next(game);
        draw_ui(game);

        if (game.paused) {
            attron(COLOR_PAIR(CP_UI) | A_BOLD);
            mvprintw(BOARD_Y + BOARD_HEIGHT / 2, BOARD_X + 1, "  PAUSED  ");
            attroff(COLOR_PAIR(CP_UI) | A_BOLD);
        }

        refresh();
        napms(16); // ~60 fps
    }

    // Game over
    nodelay(stdscr, FALSE);
    attron(COLOR_PAIR(CP_UI) | A_BOLD);
    int mid = BOARD_Y + BOARD_HEIGHT / 2;
    mvprintw(mid - 1, BOARD_X, " GAME OVER! ");
    mvprintw(mid,     BOARD_X, " Score: %lld ", game.score);
    mvprintw(mid + 1, BOARD_X, " Press any key ");
    attroff(COLOR_PAIR(CP_UI) | A_BOLD);
    refresh();
    getch();

    endwin();
    return 0;
}
