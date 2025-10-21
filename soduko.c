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

#define BOARD_WIDTH 9
#define BOARD_HEIGHT 9
#define TICK_RATE 60
#define MICROSECONDS_PER_TICK (1000000 / TICK_RATE)

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position pos;
    int real_num;
    int player_num;
    bool preloaded; 
} Square; 

Square board[BOARD_HEIGHT][BOARD_WIDTH];
Position player_pos = {BOARD_HEIGHT / 2, BOARD_WIDTH / 2};
Position last_player_pos = {-1, -1};
bool board_changed = true;

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

static void bounds_check() {
    if (player_pos.x >= BOARD_WIDTH) {
        player_pos.x = BOARD_WIDTH -1;
    } else if (player_pos.x < 0) {
        player_pos.x = 0;
    }
    if (player_pos.y >= BOARD_HEIGHT) {
        player_pos.y = BOARD_HEIGHT - 1;
    } else if (player_pos.y < 0) {
        player_pos.y = 0;
    }
}

static bool is_valid_pos(int num, int row, int col) {
    for (int x = 0; x < BOARD_WIDTH; x++) {
        if (x != col && board[row][x].real_num == num) {
            return false;
        }
    }

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        if (y != row && board[y][col].real_num == num) {
            return false;
        }
    }

    int box_start_row = (row / 3) * 3;
    int box_start_col = (col / 3) * 3;
    for (int y = box_start_row; y < box_start_row + 3; y++) {
        for (int x = box_start_col; x < box_start_col + 3; x++) {
            if ((y != row && x != col) && board[y][x].real_num == num) {
                return false;
            }
        }
    }
    return true;
}

static bool win_check() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x].player_num != board[y][x].real_num) {
                return false;
            }
        }
    }
    return true;
}

static bool fill_board(int row, int col) {
    if (row == BOARD_HEIGHT) return true; 

    int next_row = (col == BOARD_WIDTH -1) ? row + 1: row;
    int next_col = (col == BOARD_WIDTH -1) ? 0 : col + 1;

    int nums[9] = {1,2,3,4,5,6,7,8,9};

    for (int i = 8; i > 0; i--) {
        int j = rand() % (i+ 1);
        int temp = nums[i];
        nums[i] = nums[j];
        nums[j] = temp;
    }

    for (int i = 0; i < 9; i++) {
        int num = nums[i];
        if (is_valid_pos(num,row,col)) {
            board[row][col].real_num = num;

            if (fill_board(next_row, next_col)) {
                return true;
            }
            board[row][col].real_num = 0;
        }
    }
    return false;
}

static void gen_board() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x].pos.x = x;
            board[y][x].pos.y = y;
            board[y][x].real_num = 0;
            board[y][x].player_num = 0;
            board[y][x].preloaded = true;
        }
    }

    fill_board(0,0);

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x].player_num = board[y][x].real_num;
        }
    }

    int cells_to_remove = 40;

    for (int i = 0; i < cells_to_remove; i++) {
        int y = rand() % BOARD_HEIGHT;
        int x = rand() % BOARD_WIDTH;

        while (board[y][x].player_num == 0) {
            y = rand() % BOARD_HEIGHT;
            x = rand() % BOARD_WIDTH;
        }
        board[y][x].player_num = 0;
        board[y][x].preloaded = false; 
    }

}

static void render() {
    printf("╔");
    for (int x = 0; x < BOARD_WIDTH; x++) {
        printf("═══");
        if (x < BOARD_WIDTH - 1) {
            printf("%s", (x % 3 == 2) ? "╦" : "╤");
        }
    }
    printf("╗\n");

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        printf("║");
        for (int x = 0; x < BOARD_WIDTH; x++) {
            bool is_cursor = (x == player_pos.x && y == player_pos.y);

            if (is_cursor) printf("\033[7m");
            
            if (board[y][x].player_num == 0) {
                printf("   ");
            } else {
                printf(" %d ", board[y][x].player_num);
            }

            if (is_cursor) printf("\033[0m");

            if (x % 3 == 2) {
                printf("║");
            } else {
                printf("│");
            }
        }
        printf("\n");

        if (y < BOARD_HEIGHT - 1) {
            printf("%s", (y % 3 == 2) ? "╠" : "╟");

            for (int x = 0; x < BOARD_WIDTH; x++) {
                if (y % 3 == 2) { 
                    printf("═══");
                } else {
                    printf("───");
                }
                if (x < BOARD_WIDTH - 1) {
                    if (x % 3 == 2 && y % 3 == 2) {
                        printf("╬");
                    } else if(x % 3 == 2) {
                        printf("╫");
                    } else if (y % 3 == 2) {
                        printf("╪");
                    } else {
                        printf("┼");
                    }
                }
            }
            printf("%s\n", (y % 3 == 2) ? "╣" : "╢");
        }
    }

    printf("╚");
    for (int x = 0; x < BOARD_WIDTH; x++) {
        printf("═══");
        if (x < BOARD_WIDTH - 1) {
            printf("%s", (x % 3 == 2) ? "╩" : "╧");
        }
    }
    printf("╝\n");

    printf("Controls: \nWASD/Arrow Keys to move\nAny number to place a number\nDelete to reset a square\nR to restart\n");

    fflush(stdout);
}

static void reset_game() {
    player_pos = (Position){BOARD_WIDTH / 2, BOARD_HEIGHT /2};
    last_player_pos = (Position){-1,-1};
    board_changed = true;
    gen_board();
    system(CLEAR_CMD); 
    render();
    board_changed = false;
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
                switch(dir) {
                    case 'A': player_pos.y--; break;
                    case 'B': player_pos.y++; break; 
                    case 'C': player_pos.x++; break;
                    case 'D': player_pos.x--; break;
                }
                bounds_check();
                if (old_pos.x != player_pos.x || old_pos.y != player_pos.y) {
                    board_changed = true; 
                    }
                return;
            }
        }
        return;
    }

    switch(key) {
        case 'w': case 'W': player_pos.y--; break;
        case 's': case 'S': player_pos.y++; break;
        case 'd': case 'D': player_pos.x++; break;
        case 'a': case 'A': player_pos.x--; break;
        case 'q': case 'Q': 
            cleanup_terminal();
            exit(0);
            break;
        case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            if (board[player_pos.y][player_pos.x].preloaded) break;
            key -= '0';
            board[player_pos.y][player_pos.x].player_num = key; 
            board_changed = true; 
            break;
        
        case 127:
            if (board[player_pos.y][player_pos.x].preloaded) break;
            board[player_pos.y][player_pos.x].player_num = 0; board_changed = true; break;
        
        case 'r': case 'R': 
            reset_game();
            break; 
        default: break;
    }

    bounds_check();
    if (old_pos.x != player_pos.x || old_pos.y != player_pos.y) {
        board_changed = true; 
    }
}




int main() {
    srand(time(NULL));
    setup_terminal();
    gen_board();
    system(CLEAR_CMD);
    render();
    board_changed = false;

    while (true) {
        process_input();
        if (board_changed) {
            system(CLEAR_CMD);
            render();
            if (win_check()) {
                printf("Game Over! You Win!");
                break;
            }
            board_changed = false;
        }

        usleep(MICROSECONDS_PER_TICK);
    }

    cleanup_terminal();
    return 0;
}