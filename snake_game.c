#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

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
#define BOARD_WIDTH 12
#define BOARD_HEIGHT 8
#define MAX_SNAKE_LENGTH 100

typedef struct {
    int x, y;
} Position;

typedef enum {
    DIR_NONE, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT
} Direction; 

static char board[BOARD_HEIGHT][BOARD_WIDTH];
static Position snake[MAX_SNAKE_LENGTH]; 
static int snake_length = 3;
static int snake_head = 0;  
static Direction current_direction = DIR_NONE;
static Position apple; 

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

static void init_snake() {
    Position start_pos = {BOARD_WIDTH / 2, BOARD_HEIGHT / 2};
    for (int i = 0; i < snake_length; i++) {
        snake[i].x = start_pos.x;
        snake[i].y = start_pos.y + i;
    }
}

static void process_input() {
    if (!has_input()) return;
    
    char key = read_char();

    #ifdef _WIN32
    if (key == 224) {  
        key = read_char();
        switch (key) {
            case 72: current_direction = DIR_UP; break;              
            case 80: current_direction = DIR_DOWN; break;     
            case 77: current_direction = DIR_RIGHT; break;      /
            case 75: current_direction = DIR_LEFT; break;               
        }
    }
    #else
    if (key == 27) {  
        if (has_input()) {
            char bracket = read_char();
            if (bracket == '[' && has_input()) {
                char direction = read_char();
                switch (direction) {
                    case 'A': current_direction = DIR_UP; break;     
                    case 'B': current_direction = DIR_DOWN; break;     
                    case 'C': current_direction = DIR_RIGHT; break;   
                    case 'D': current_direction = DIR_LEFT; break;               
                }
            }
        }
    }
    #endif

    switch (key) {
        case 'q': case 'Q':
            cleanup_terminal();
            printf("\nGame over! Thanks for playing.\n");
            exit(0);
            break;
        case 'w': case 'W': current_direction = DIR_UP; break;
        case 's': case 'S': current_direction = DIR_DOWN; break;
        case 'd': case 'D': current_direction = DIR_RIGHT; break;
        case 'a': case 'A': current_direction = DIR_LEFT; break;
        case ' ': current_direction = DIR_NONE; break;
    }
}

static void move_snake() {
    if (current_direction == DIR_NONE) return;

    Position new_head = snake[snake_head];
    
    switch(current_direction) {
        case DIR_UP: new_head.y--; break;
        case DIR_DOWN: new_head.y++; break;
        case DIR_LEFT: new_head.x--; break;
        case DIR_RIGHT: new_head.x++; break;
        case DIR_NONE: break;
    }

    snake_head = (snake_head + 1) % snake_length;
    snake[snake_head] = new_head;
}

static bool is_position_occupied(int x, int y) {
    for (int i = 0; i < snake_length; i++) {
        if (snake[i].x == x && snake[i].y == y) {
            return true;
        }
    }
    return false;
}

static void place_apple() {
    int attempts = 0;
    const int max_attempts = 100;

    do {
        apple.x = rand() % BOARD_WIDTH;   
        apple.y = rand() % BOARD_HEIGHT; 
        attempts++;
    } while (is_position_occupied(apple.x, apple.y) && attempts < max_attempts);

    if (attempts >= max_attempts) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            for (int x = 0; x < BOARD_WIDTH; x++) {
                if (!is_position_occupied(x, y)) {
                    apple.x = x;
                    apple.y = y;
                    return;
                }
            }
        }
        cleanup_terminal();
        printf("\n You Win!!!\n");
        exit(0);
    }
}

static void update_game() {
    process_input();
    move_snake();
    
    Position head = snake[snake_head];
    for (int i = 0; i < snake_length; i++) {
        if (i == snake_head) continue;
        if (snake[i].x == head.x && snake[i].y == head.y) {
            cleanup_terminal();
            printf("\nGame Over\n");
            exit(0);
        }
    }

     if (snake[snake_head].x < 0 || snake[snake_head].x >= BOARD_WIDTH ||
        snake[snake_head].y < 0 || snake[snake_head].y >= BOARD_HEIGHT) {
            cleanup_terminal();
            printf("\nGame Over\n");
            exit(0);
        }

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x] = '.';
        }
    }

    for (int i = 0; i < snake_length; i++) {
        board[snake[i].y][snake[i].x] = '@';
    }

    if (snake[snake_head].x == apple.x && snake[snake_head].y == apple.y) {
        int tail_index = (snake_head + 1) % snake_length;
        Position tail_pos = snake[tail_index];
        
        snake_length += 1;
        
        snake[tail_index] = tail_pos;
        
        place_apple(); 
    }
    
    board[apple.y][apple.x] = '#';  
}

static void render_frame() {
    system(CLEAR_CMD);
  
    printf("┌");
    for (int x = 0; x < BOARD_WIDTH; x++) printf("──");
    printf("┐\n");

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        printf("│");
        for (int x = 0; x < BOARD_WIDTH; x++) {
            printf("%c ", board[y][x]);
        }
        printf("│\n");
    }
    
    printf("└");
    for (int x = 0; x < BOARD_WIDTH; x++) printf("──");
    printf("┘\n");
    
    printf("\nControls: Arrow keys or WASD to move, Q to quit\n");
    printf("Head Position: (%d, %d) | Direction: %s | Snake Length: %d | Apple Position (%d,%d)\n", 
           snake[snake_head].x, snake[snake_head].y,
           current_direction == DIR_NONE ? "stopped" :
           current_direction == DIR_UP ? "up" :
           current_direction == DIR_DOWN ? "down" :
           current_direction == DIR_LEFT ? "left" : "right",
           snake_length, apple.x, apple.y);
}


int main() {
    printf("Starting terminal snake game...\n");
    printf("Setting up terminal for raw input...\n");
    
    srand(time(NULL));  
    setup_terminal();
    init_snake();  
    place_apple();      

    while (true) {
        update_game();
        render_frame();
        usleep(MICROSECONDS_PER_TICK);
    }
    
    cleanup_terminal();
    return 0;
}