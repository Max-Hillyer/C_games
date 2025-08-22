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

#define TICK_RATE 10
#define MICROSECONDS_PER_TICK (1000000 / TICK_RATE)
#define BOARD_WIDTH 4
#define BOARD_HEIGHT 4

typedef enum {
    DIR_NONE, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT
} Direction;

static int board[BOARD_HEIGHT][BOARD_WIDTH];
static Direction current_direction = DIR_NONE;
static int score = 0;
static bool game_over = false;
static bool won = false;

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

static void add_random_tile() {
    int empty_cells[BOARD_WIDTH * BOARD_HEIGHT][2];
    int empty_count = 0;
    
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 0) {
                empty_cells[empty_count][0] = y;
                empty_cells[empty_count][1] = x;
                empty_count++;
            }
        }
    }
    
    if (empty_count == 0) return;

    int random_index = rand() % empty_count;
    int y = empty_cells[random_index][0];
    int x = empty_cells[random_index][1];

    board[y][x] = (rand() % 10 == 0) ? 4 : 2;
}

static void init_board() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x] = 0;
        }
    }

    add_random_tile();
    add_random_tile();
}



static bool move_left() {
    bool moved = false;
    
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        int write_pos = 0;
        bool merged[BOARD_WIDTH] = {false};

        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] != 0) {
                if (write_pos > 0 && 
                    board[y][write_pos - 1] == board[y][x] && 
                    !merged[write_pos - 1]) {

                    board[y][write_pos - 1] *= 2;
                    score += board[y][write_pos - 1];
                    merged[write_pos - 1] = true;
                    board[y][x] = 0;
                    moved = true;

                    if (board[y][write_pos - 1] == 2048 && !won) {
                        won = true;
                    }
                } else {
                    if (write_pos != x) {
                        board[y][write_pos] = board[y][x];
                        board[y][x] = 0;
                        moved = true;
                    }
                    write_pos++;
                }
            }
        }
    }
    
    return moved;
}

static bool move_right() {
    bool moved = false;
    
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        int write_pos = BOARD_WIDTH - 1;
        bool merged[BOARD_WIDTH] = {false};

        for (int x = BOARD_WIDTH - 1; x >= 0; x--) {
            if (board[y][x] != 0) {
                if (write_pos < BOARD_WIDTH - 1 && 
                    board[y][write_pos + 1] == board[y][x] && 
                    !merged[write_pos + 1]) {

                    board[y][write_pos + 1] *= 2;
                    score += board[y][write_pos + 1];
                    merged[write_pos + 1] = true;
                    board[y][x] = 0;
                    moved = true;

                    if (board[y][write_pos + 1] == 2048 && !won) {
                        won = true;
                    }
                } else {
                    if (write_pos != x) {
                        board[y][write_pos] = board[y][x];
                        board[y][x] = 0;
                        moved = true;
                    }
                    write_pos--;
                }
            }
        }
    }
    
    return moved;
}

static bool move_up() {
    bool moved = false;
    
    for (int x = 0; x < BOARD_WIDTH; x++) {
        int write_pos = 0;
        bool merged[BOARD_HEIGHT] = {false};

        for (int y = 0; y < BOARD_HEIGHT; y++) {
            if (board[y][x] != 0) {
                if (write_pos > 0 && 
                    board[write_pos - 1][x] == board[y][x] && 
                    !merged[write_pos - 1]) {

                    board[write_pos - 1][x] *= 2;
                    score += board[write_pos - 1][x];
                    merged[write_pos - 1] = true;
                    board[y][x] = 0;
                    moved = true;

                    if (board[write_pos - 1][x] == 2048 && !won) {
                        won = true;
                    }
                } else {
                    if (write_pos != y) {
                        board[write_pos][x] = board[y][x];
                        board[y][x] = 0;
                        moved = true;
                    }
                    write_pos++;
                }
            }
        }
    }
    
    return moved;
}

static bool move_down() {
    bool moved = false;
    
    for (int x = 0; x < BOARD_WIDTH; x++) {
        int write_pos = BOARD_HEIGHT - 1;
        bool merged[BOARD_HEIGHT] = {false};

        for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
            if (board[y][x] != 0) {
                if (write_pos < BOARD_HEIGHT - 1 && 
                    board[write_pos + 1][x] == board[y][x] && 
                    !merged[write_pos + 1]) {

                    board[write_pos + 1][x] *= 2;
                    score += board[write_pos + 1][x];
                    merged[write_pos + 1] = true;
                    board[y][x] = 0;
                    moved = true;

                    if (board[write_pos + 1][x] == 2048 && !won) {
                        won = true;
                    }
                } else {
                    if (write_pos != y) {
                        board[write_pos][x] = board[y][x];
                        board[y][x] = 0;
                        moved = true;
                    }
                    write_pos--;
                }
            }
        }
    }
    
    return moved;
}

static bool can_move() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 0) {
                return true;
            }
        }
    }

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int current = board[y][x];
            if (x < BOARD_WIDTH - 1 && board[y][x + 1] == current) {
                return true;
            }
            if (y < BOARD_HEIGHT - 1 && board[y + 1][x] == current) {
                return true;
            }
        }
    }
    
    return false;
}

static void process_input() {
    if (!has_input()) return;
    
    char key = read_char();
    Direction new_direction = DIR_NONE;
    
    #ifdef _WIN32
    if (key == 224) {
        key = read_char();
        switch(key) {
            case 72: new_direction = DIR_UP; break;
            case 80: new_direction = DIR_DOWN; break;
            case 77: new_direction = DIR_RIGHT; break;
            case 75: new_direction = DIR_LEFT; break;
        }
    }
    #else
    if (key == 27) {
        if (has_input()) {
            char bracket = read_char();
            if (bracket == '[' && has_input()) {
                char direction = read_char();
                switch (direction) {
                    case 'A': new_direction = DIR_UP; break;
                    case 'B': new_direction = DIR_DOWN; break;
                    case 'C': new_direction = DIR_RIGHT; break;
                    case 'D': new_direction = DIR_LEFT; break;
                }
            }
        }
    }
    #endif
    
    switch(key) {
        case 'q': case 'Q':
            cleanup_terminal();
            printf("\nGame Over\n");
            exit(0);
            break;
        case 'w': case 'W': new_direction = DIR_UP; break;
        case 's': case 'S': new_direction = DIR_DOWN; break;
        case 'd': case 'D': new_direction = DIR_RIGHT; break;
        case 'a': case 'A': new_direction = DIR_LEFT; break;
        case 'r': case 'R': 
            score = 0;
            game_over = false;
            won = false;
            init_board();
            break;
    }
    
    if (!game_over && new_direction != DIR_NONE) {
        bool moved = false;
        
        switch(new_direction) {
            case DIR_UP: moved = move_up(); break;
            case DIR_DOWN: moved = move_down(); break;
            case DIR_LEFT: moved = move_left(); break;
            case DIR_RIGHT: moved = move_right(); break;
            default: break;
        }
        
        if (moved) {
            add_random_tile();
            if (!can_move()) {
                game_over = true;
            }
        }
    }
    
    current_direction = new_direction;
}

static void render() {
    system(CLEAR_CMD);
    
    printf("Score: %d\n", score);
    if (won) {
        printf("YOU WON! You reached 2048! Press 'r' to restart or 'q' to quit.\n");
    } else if (game_over) {
        printf("GAME OVER! Press 'r' to restart or 'q' to quit.\n");
    } else {
        printf("Use WASD or arrow keys to move, 'q' to quit, 'r' to restart\n");
    }
    printf("\n");
    
    printf("┌");
    for (int x = 0; x < BOARD_WIDTH; x++) {
        printf("─────┬");
    }
    printf("\b┐\n");
    
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        printf("│");
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int value = board[y][x];
            if (value == 0) {
                printf("     │");
            } else {
                switch(value) {
                    case 2: printf("\033[30;47m%4d\033[0m │", value); break;
                    case 4: printf("\033[30;43m%4d\033[0m │", value); break;
                    case 8: printf("\033[30;42m%4d\033[0m │", value); break;
                    case 16: printf("\033[30;41m%4d\033[0m │", value); break;
                    case 32: printf("\033[30;44m%4d\033[0m │", value); break;
                    case 64: printf("\033[30;45m%4d\033[0m │", value); break;
                    case 128: printf("\033[30;46m%4d\033[0m │", value); break;
                    case 256: printf("\033[30;100m%4d\033[0m │", value); break;
                    case 512: printf("\033[30;101m%4d\033[0m │", value); break;
                    case 1024: printf("\033[30;102m%4d\033[0m │", value); break;
                    case 2048: printf("\033[30;103m%4d\033[0m │", value); break;
                    default: printf("%5d │", value); break;
            }
                }
        }
        printf("\n");
        
        if (y < BOARD_HEIGHT - 1) {
            printf("├");
            for (int x = 0; x < BOARD_WIDTH; x++) {
                printf("─────┼");
            }
            printf("\b┤\n");
        }
    }
    
    printf("└");
    for (int x = 0; x < BOARD_WIDTH; x++) {
        printf("─────┴");
    }
    printf("\b┘\n");
}

int main() {
    srand(time(NULL));  
    setup_terminal();
    init_board();
    
    while (true) {
        process_input();
        render();
        usleep(MICROSECONDS_PER_TICK);
    }
    
    cleanup_terminal();
    return 0;
}