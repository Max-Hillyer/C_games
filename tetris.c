#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #define CLEAR_CMD "cls"
#else
    #include <termios.h>
    #include <fcntl.h>
    #define CLEAR_CMD "clear"
#endif

#define TICK_RATE 60
#define MICROSECONDS_PER_TICK (1000000 / TICK_RATE)
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_TYPES 4

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position pos;
    int shape[4][4];
    int rotation;
    int color;
} Piece;

int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
int color[BOARD_HEIGHT][BOARD_WIDTH] = {0};
int fall_counter = 0;
int level = 1;
int rows_cleared = 19;
int fall_speed = 30;
int score = 0;
Piece next_piece;
bool paused = false; 
int block_appearance = 0;
int col_element = 31;
bool title_flash_pause = false;

int base_I_piece[4][4] = {
    {0, 0, 0, 0},
    {1, 1, 1, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};

int base_O_piece[4][4] = {
    {0, 0, 0, 0},
    {0, 1, 1, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
};

int base_S_piece[4][4] = {
    {0, 0, 0, 0},
    {0, 1, 1, 0},
    {1, 1, 0, 0},
    {0, 0, 0, 0}
};

int base_Z_piece[4][4] = {
    {0, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0}
};

int base_L_piece[4][4] = {
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {1, 0, 0, 0},
    {1, 1, 0, 0}
};

int base_J_piece[4][4] = {
    {0, 0, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 0},
    {0, 1, 1, 0}
};

int base_T_piece[4][4] = {
    {0, 0, 0, 0},
    {0, 1, 0, 0},
    {1, 1, 1, 0},
    {0, 0, 0, 0}
};


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

static void init_piece(Piece* piece, int base_shape[4][4]) {
    memcpy(piece->shape, base_shape, sizeof(piece->shape));
    piece->rotation = 0;
    piece->pos.x = BOARD_WIDTH / 2 - 2;
    piece->pos.y = 0;
    piece->color = (rand() % 6) + 1;
}

static bool is_valid_pos(Piece* piece, int new_x, int new_y, int test[4][4]) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (test[y][x]) {
                int board_x = new_x + x;
                int board_y = new_y + y;

                if (board_x < 0 || board_x >= BOARD_WIDTH || 
                    board_y < 0 || board_y >= BOARD_HEIGHT || 
                    board[board_y][board_x]) {
                    return false;
                }
            }
        }
    }
    return true;
}

static void rotate_piece(Piece* piece) {
    if (!paused) {
        int temp[4][4];
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                temp[x][3 - y] = piece->shape[y][x];
            }
        }

        if (is_valid_pos(piece, piece->pos.x, piece->pos.y, temp)) {
            memcpy(piece->shape, temp, sizeof(temp));
            piece->rotation = (piece->rotation + 1) % 4;
    } 
    }
}

static void increment_level() {
    if (fall_speed > 5) {
        if (level < 5) {
            fall_speed--;
        } else if (level < 10) {
            fall_speed -= 2;
        } else if (level < 15) {
            fall_speed -= 3;
        }
    }
    level = (rows_cleared / 10) + 1;
}

static void increment_score(int lines_in_turn) {
    switch(lines_in_turn) {
        case 1: score += (40 * (level + 1)); break;
        case 2: score += (100 * (level + 1)); break;
        case 3: score += (900 * (level + 1)); break;
        case 4: score += (1200 * (level + 1)); break;
    }
}

static void clear_lines() {
    int lines_in_turn = 0;
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        bool full_line = true;

        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 0) {
                full_line = false;
                break;
            }
        }

        if (full_line) {
            for (int move_y = y; move_y > 0; move_y--) {
                for (int x = 0; x < BOARD_WIDTH; x++) {
                    board[move_y][x] = board[move_y - 1][x];
                    color[move_y][x] = color[move_y - 1][x];
                }
            }
            
            for (int x = 0; x < BOARD_WIDTH; x++) {
                board[0][x] = 0;
                color[0][x] = 0;
            }
            y++;
            rows_cleared++;
            lines_in_turn++;
        }
    }
    if (lines_in_turn > 0 && rows_cleared % 10 == 0) {
            increment_level();
    }
    increment_score(lines_in_turn);
}

static void move_piece(Piece* piece, int dx, int dy) {
    if (!paused) {
        int x = piece->pos.x + dx;
        int y = piece->pos.y + dy;

        if (is_valid_pos(piece, x, y, piece->shape)) {
            piece->pos.x = x;
            piece->pos.y = y;
        } else {
            if (dy > 0) {
                for (int py = 0; py < 4; py++) {
                    for (int px = 0; px < 4; px++) {
                        if (piece->shape[py][px]) {
                            int board_x = piece->pos.x + px;
                            int board_y = piece->pos.y + py;
                            if (board_x >= 0 && board_x < BOARD_WIDTH && 
                                board_y >= 0 && board_y < BOARD_HEIGHT) {
                                board[board_y][board_x] = 1;
                                color[board_y][board_x] = piece->color;
                            }
                        }
                    }
                }
                clear_lines();

                *piece = next_piece;
                int new_piece = rand() % 7;
                switch(new_piece) {
                    case 0: init_piece(&next_piece, base_I_piece); break;
                    case 1: init_piece(&next_piece, base_O_piece); break;
                    case 2: init_piece(&next_piece, base_S_piece); break;
                    case 3: init_piece(&next_piece, base_Z_piece); break;
                    case 4: init_piece(&next_piece, base_L_piece); break;
                    case 5: init_piece(&next_piece, base_J_piece); break;
                    case 6: init_piece(&next_piece, base_T_piece); break;
                }
            }
        }
    }   
}

static bool is_game_over() {
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x]) {
                return true;
            }
        }
    }
    return false;
}

static void print_color_block(int color_val) {
    char* block = "[]";
    switch (block_appearance) {
        case 0: block = "██"; break;
        case 1: block = "[]"; break;
        case 2: block = "░░"; break;
        case 3: block = "##"; break;
    }
    switch(color_val) {
        case 1: printf("\033[32m%s\033[0m", block); break; // Green
        case 2: printf("\033[31m%s\033[0m", block); break; // Red
        case 3: printf("\033[34m%s\033[0m", block); break; // Blue
        case 4: printf("\033[93m%s\033[0m", block); break; // Yellow
        case 5: printf("\033[96m%s\033[0m", block); break; // Cyan
        case 6: printf("\033[95m%s\033[0m", block); break; // Magenta
        default: printf("  "); break;
    }
}

static void print_title() {
    if (fall_counter % 20 == 0 && !title_flash_pause) {
        int col_list[] = {32, 31, 34, 93, 96, 95};
        int list_size = sizeof(col_list) / sizeof(col_list[0]);
        col_element = col_list[rand() % list_size];
    } 
    printf("\033[%dm _____    _        _     \n", col_element);
    printf("|_   _|__| |_ _ __(_)___ \n");
    printf("  | |/ _ \\ __| '__| / __|\n");
    printf("  | |  __/ |_| |  | \\__ \\\n");
    printf("  |_|\\___|\\__|_|  |_|___/\n");
    printf("                         \n\033[0m");
}

static void render(Piece* piece, Piece* next_piece) {
    char disp_board[BOARD_HEIGHT][BOARD_WIDTH];

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            disp_board[y][x] = board[y][x] ? '#' : ' ';
        }
    }

    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (piece->shape[py][px]) {
                int board_x = piece->pos.x + px;
                int board_y = piece->pos.y + py;

                if (board_x >= 0 && board_x < BOARD_WIDTH && 
                    board_y >= 0 && board_y < BOARD_HEIGHT) {
                    disp_board[board_y][board_x] = '@';
                }
            }
        }
    } 
    print_title();

    printf("|");
    for (int x = 0; x < BOARD_WIDTH; x++) printf("──");
    printf("|\n");

    for (int y = 0; y < BOARD_HEIGHT; y++) {
    printf("|");
    for (int x = 0; x < BOARD_WIDTH; x++) {
        if (disp_board[y][x] == '#') {
            print_color_block(color[y][x]);
        } else if (disp_board[y][x] == '@') {
            print_color_block(piece->color); 
        } else {
            printf("  "); 
        }
    }
    printf("|\n");
}

    printf("|");
    for (int x = 0; x < BOARD_WIDTH; x++) printf("──");
    printf("|\n\n");
    printf("Next Piece:\n");
    printf("|");
    for (int x = 0; x < 4; x++) printf("──");
    printf("|\n");

    for (int y = 0; y < 4; y++) {
        printf("|");
        for (int x = 0; x < 4; x++) {
            if (next_piece->shape[y][x]) {
                print_color_block(next_piece->color);
            } else {
                printf("  ");
            }
        }
        printf("|");
        switch(y) {
            case 0: printf(" %s", paused ? "Paused" : ""); break;
            case 1: printf(" Score: %d", score); break;
            case 2: printf(" Level: %d", level);break;
            case 3: printf(" Rows Cleared: %d ", rows_cleared);break;
            default: break;
        }
        printf("\n");
    }

    printf("|");
    for (int x = 0; x < 4; x++) printf("──");
    printf("|\n");

    printf("Controls\n WASD/Arrow Keys to move\n W/Up to rotate\n R to reset\n Q to quit\n Space to pause\n E to change block appearance\n F to stop title flash\n");
    printf("\n");
}

static void process_input(Piece* current_piece) {
    if (!has_input()) return;
    
    char key = read_char();

    if (key == 27) {
        if (has_input()) {
            char bracket = read_char();
            if (bracket == '[' && has_input()) {
                char direction = read_char();
                switch (direction) {
                    case 'A': rotate_piece(current_piece); break; 
                    case 'B': move_piece(current_piece, 0,1); break;
                    case 'C': move_piece(current_piece, 1,0);break;
                    case 'D': move_piece(current_piece, -1,0);break;
                }
            }
        }
    }

    switch(key) {
        case 'w': case 'W': rotate_piece(current_piece); break;
        case 's': case 'S': move_piece(current_piece, 0, 1); break;
        case 'd': case 'D': move_piece(current_piece, 1, 0); break;
        case 'a': case 'A': move_piece(current_piece, -1, 0); break;
        case 'q': case 'Q':
            cleanup_terminal();
            printf("\nGame Over\n");
            exit(0);
            break;
        case 'r': case 'R':
            memset(board, 0, sizeof(board));
            memset(color, 0, sizeof(color));
            init_piece(current_piece, base_I_piece);
            score = 0;
            level = 0;
            rows_cleared = 0;
            fall_counter = 0;
            fall_speed = 30;
            break;
        case ' ': paused = !paused; break;
        case 'E': case 'e': 
            block_appearance = (block_appearance + 1) % BLOCK_TYPES;
            break;
        case 'F': case 'f': title_flash_pause = !title_flash_pause; break;
    }
}

int main() {
    srand(time(NULL));
    setup_terminal();
    Piece current_piece;
     
    int new_piece = rand() % 7;
    switch(new_piece) {
                case 0: init_piece(&current_piece, base_I_piece); break;
                case 1: init_piece(&current_piece, base_O_piece); break;
                case 2: init_piece(&current_piece, base_S_piece); break;
                case 3: init_piece(&current_piece, base_Z_piece); break;
                case 4: init_piece(&current_piece, base_L_piece); break;
                case 5: init_piece(&current_piece, base_J_piece); break;
                case 6: init_piece(&current_piece, base_T_piece); break;
            }
    int new_next_piece = rand() % 7;
    switch(new_next_piece) {
                case 0: init_piece(&next_piece, base_I_piece); break;
                case 1: init_piece(&next_piece, base_O_piece); break;
                case 2: init_piece(&next_piece, base_S_piece); break;
                case 3: init_piece(&next_piece, base_Z_piece); break;
                case 4: init_piece(&next_piece, base_L_piece); break;
                case 5: init_piece(&next_piece, base_J_piece); break;
                case 6: init_piece(&next_piece, base_T_piece); break;
            }

    while (true) {
        process_input(&current_piece);
        if (!paused) {
            system(CLEAR_CMD);
            render(&current_piece, &next_piece);
            fall_counter++;
            if (fall_counter >= fall_speed) {
                move_piece(&current_piece, 0, 1);
                fall_counter = 0;
            }

            if (is_game_over()) {
                printf("\nGame Over\n");
                break;
            }

            usleep(MICROSECONDS_PER_TICK);
        } else {
            title_flash_pause = false;
            system(CLEAR_CMD);
            render(&current_piece, &next_piece);
            usleep(MICROSECONDS_PER_TICK * 100);
        }
    }
    cleanup_terminal();
    return 0;
}