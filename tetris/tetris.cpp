#include <ncurses.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

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
