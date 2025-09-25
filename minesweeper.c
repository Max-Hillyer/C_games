#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #define CLEAR_CMD "cls"
#else 
    #include <termios.h>
    #include <fcntl.h>
    #define CLEAR_CMD "clear"
#endif

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 10
#define TICK_RATE 60
#define MICROSECONDS_PER_TICK (1000000 / TICK_RATE)
#define MINES 15
#define MAX_FLAGS MINES

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position pos;
    bool flagged;
    bool clicked; 
    bool mine;
    int close; 
} Square;

Square board[BOARD_HEIGHT][BOARD_WIDTH];
Position player_pos = {BOARD_WIDTH / 2, BOARD_HEIGHT / 2};
Position last_player_pos = {-1, -1}; 
bool board_changed = true; 
bool mines_planted = false;
bool loss = false; 
int flags_placed = 0;

#ifndef _WIN32
static struct termios original_termios;

static void setup_terminal() {
    struct termios raw_termios;
    
    tcgetattr(STDIN_FILENO, &original_termios);
    
    raw_termios = original_termios;
    raw_termios.c_lflag &= ~(ICANON | ECHO);
    
    tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios);
    
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

static void cleanup_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

static bool has_input() {
    int ch = getchar();
    if (ch != EOF) {
        ungetc(ch, stdin);
        return true;
    }
    return false;
}

static char read_char() {
    return getchar();
}

#else

static void setup_terminal() {
    
}

static void cleanup_terminal() {
   
}

static bool has_input() {
    return _kbhit();
}

static char read_char() {
    return _getch();
}
#endif

static void plant_mines(int safeX, int safeY){ 
    int placed = 0;
    while (placed < MINES) {
        int x = rand() % BOARD_WIDTH;
        int y = rand() % BOARD_HEIGHT;
        if (abs(x - safeX) <= 1 && abs(y - safeY) <= 1) continue;
        if (!board[y][x].mine) {
            board[y][x].mine = true; 
            placed++;
        }
    }
}

static int get_near(int x, int y) {
    int count = 0;
    for (int dy = y - 1; dy <= y + 1; dy++) {
        for (int dx = x - 1; dx <= x + 1; dx++) {
            if (dy < 0 || dy >= BOARD_HEIGHT || dx < 0 || dx >= BOARD_WIDTH) continue;
            if (dy == y && dx == x) continue;

            if (board[dy][dx].mine) {
                count++;
            }
        }
    }
    return count;
}

static void bounds_check() {
    if (player_pos.x >= BOARD_WIDTH) {
        player_pos.x = BOARD_WIDTH - 1;
    } else if (player_pos.x < 0) {
        player_pos.x = 0;
    }

    if (player_pos.y >= BOARD_HEIGHT) {
        player_pos.y = BOARD_HEIGHT - 1;
    } else if (player_pos.y < 0) {
        player_pos.y = 0;
    }
}

static bool win_check() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (!board[y][x].mine) {
                if (!board[y][x].clicked) return false;
            } 
        }
    }
    return true;
}

static void click_square(int x, int y) {
    if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT) return;
    if (board[y][x].flagged || board[y][x].clicked) return;

    board[y][x].clicked = true;
    board_changed = true;

    if (board[y][x].mine) loss = true; 
    if (get_near(x, y) != 0) return;

    for (int dy = y - 1; dy <= y + 1; dy++) {
        for (int dx = x - 1; dx <= x + 1; dx++) {
            if (dx == x && dy == y) continue; 
            click_square(dx, dy);
            if (win_check()) {
                printf("\nGame Over, You Win!\n");
                cleanup_terminal();
                exit(0);
            }
        }
    }
}

static void reset_board() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x].pos.x = x;
            board[y][x].pos.y = y;
            board[y][x].flagged = false;
            board[y][x].clicked = false;
            board[y][x].mine = false;
            board[y][x].close = 0;
        }
    }
    player_pos.x = BOARD_WIDTH / 2;
    player_pos.y = BOARD_HEIGHT / 2;
    flags_placed = 0;
    mines_planted = false;
    loss = false;
    board_changed = true;
}

static void process_input() {
    if (!has_input()) return;

    int key = (unsigned char) read_char();
    Position old_pos = player_pos;

    if (key == 27) { 
        if (has_input()) {
            int next = (unsigned char) read_char();
            if (next == '[' && has_input()) {
                int dir = (unsigned char) read_char();
                switch (dir) {
                    case 'A': player_pos.y--; break; 
                    case 'B': player_pos.y++; break;
                    case 'C': player_pos.x++; break; 
                    case 'D': player_pos.x--; break; 
                }
                bounds_check();
                if (player_pos.x != old_pos.x || player_pos.y != old_pos.y) {
                    board_changed = true;
                }
                return; 
            }
        }
        return; 
    }

    switch (key) {
        case 'w': case 'W': 
            player_pos.y--; 
            bounds_check();
            if (player_pos.x != old_pos.x || player_pos.y != old_pos.y) {
                board_changed = true;
            }
            break;
        case 's': case 'S': 
            player_pos.y++; 
            bounds_check();
            if (player_pos.x != old_pos.x || player_pos.y != old_pos.y) {
                board_changed = true;
            }
            break;
        case 'd': case 'D': 
            player_pos.x++; 
            bounds_check();
            if (player_pos.x != old_pos.x || player_pos.y != old_pos.y) {
                board_changed = true;
            }
            break;
        case 'a': case 'A': 
            player_pos.x--; 
            bounds_check();
            if (player_pos.x != old_pos.x || player_pos.y != old_pos.y) {
                board_changed = true;
            }
            break;

        case 'f': case 'F': 
            if (!board[player_pos.y][player_pos.x].clicked) {
                if (!board[player_pos.y][player_pos.x].flagged && flags_placed < MAX_FLAGS) {
                    board[player_pos.y][player_pos.x].flagged = true; 
                    flags_placed++;
                } else if (board[player_pos.y][player_pos.x].flagged) {
                    board[player_pos.y][player_pos.x].flagged = false;
                    flags_placed--;
                }
                board_changed = true;
            }
            break;

        case ' ': 
            if (!mines_planted) {
                plant_mines(player_pos.x, player_pos.y);
                mines_planted = true; 
            }
            click_square(player_pos.x,player_pos.y);
            if (win_check()) {
                printf("\nGame Over, You Win!\n");
                cleanup_terminal();
                exit(0);
            }
            break;

        case 'r': case 'R':
            reset_board();
            break;

        case 'q': case 'Q':
            cleanup_terminal();
            printf("\nGame Over\n");
            exit(0);
            break;

        default:
            break;
    }
}

static void render() {
    printf("┌");
    for (int x = 0; x < BOARD_WIDTH; x++) {
        printf("───");
        if (x < BOARD_WIDTH - 1) printf("┬");
    }
    printf("┐\n");
    
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        printf("│");
        for (int x = 0; x < BOARD_WIDTH; x++) {
            bool is_cursor = (x == player_pos.x && y == player_pos.y);
            
            if (is_cursor) printf("\033[7m"); 

            if (loss && board[y][x].mine) {
                printf("\033[31m * \033[0m");
            } else if (!board[y][x].clicked && !board[y][x].flagged) {
                printf("   ");  
            } else if (board[y][x].clicked) {
                if (board[y][x].mine) {
                    printf("\033[31m * \033[0m");
                    if (is_cursor) printf("\033[7m");
                } else {
                    int nearby = get_near(x, y);
                    printf(" %d ", nearby);
                }
            } else if (board[y][x].flagged) {
                printf("\033[32m f \033[0m");
                if (is_cursor) printf("\033[7m");
            }
            
            if (is_cursor) printf("\033[0m"); 
            printf("│");

        }
        printf("\n");

        if (y < BOARD_HEIGHT - 1) {
            printf("├");
            for (int x = 0; x < BOARD_WIDTH; x++) {
                printf("───");
                if (x < BOARD_WIDTH - 1) printf("┼");
            }
            printf("┤\n");
        }
    }

    printf("└");
    for (int x = 0; x < BOARD_WIDTH; x++) {
        printf("───");
        if (x < BOARD_WIDTH - 1) printf("┴");
    }
    printf("┘\n");

    printf("Position: (%d,%d) | Flags Remaining: %d\n Controls\n WASD / Arrow to Move\n Space to Click\n F to Flag\n Q to Quit\n", player_pos.x, player_pos.y, MAX_FLAGS - flags_placed);
    fflush(stdout); 
}

int main() {
    srand(time(NULL));
    setup_terminal();
    reset_board();
    system(CLEAR_CMD);
    render();
    board_changed = false;

    while (!loss) {
        process_input();
        if (board_changed) {
            system(CLEAR_CMD);
            render();
            board_changed = false;
        }
        
        usleep(MICROSECONDS_PER_TICK);
    }
    
    cleanup_terminal();
    return 0;
}