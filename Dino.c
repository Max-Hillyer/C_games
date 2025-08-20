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

#define TICK_RATE 60
#define MICROSECONDS_PER_TICK (1000000 / TICK_RATE)
#define FLOOR_LENGTH 50
#define GAME_HEIGHT 5
#define DINO_X_POSITION 5
#define MAX_JUMP_HEIGHT 3
#define MAX_OBSTACLES 10
#define MIN_OBSTACLE_SPACING 8
#define OBSTACLE_SPAWN_CHANCE 5

typedef struct {
    int x;
    bool active;
    int move_timer;
} Obstacle;

static struct termios original_termios;
bool jumping = false;
int ticks_since_jump = 0;
Obstacle obstacles[MAX_OBSTACLES];
int last_obstacle_x = FLOOR_LENGTH + MIN_OBSTACLE_SPACING;
int score = 0;


#ifndef _WIN32
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
    // Windows terminal setup if needed
}

static void cleanup_terminal() {
    // Windows terminal cleanup if needed
}

static bool has_input() {
    return _kbhit();
}

static char read_char() {
    return _getch();
}
#endif

static void init_obstacles() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i].active = false;
        obstacles[i].x = 0;
        obstacles[i].move_timer = 0;
    }
}

static void spawn_obstacle() {
    if (last_obstacle_x < MIN_OBSTACLE_SPACING) {
        return;
    }
    if (rand() % 100 >= OBSTACLE_SPAWN_CHANCE) {
        return;
    }

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) {
            obstacles[i].active = true;
            obstacles[i].x = FLOOR_LENGTH - 1;
            obstacles[i].move_timer = 0;
            last_obstacle_x = 0; 
            break;
        }
    }
}

static void update_obstacles() {
    last_obstacle_x++;
    
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            obstacles[i].move_timer++;

            if (obstacles[i].move_timer >= 2) {
                obstacles[i].x--;
                obstacles[i].move_timer = 0;
            }
            
            if (obstacles[i].x < 0) {
                obstacles[i].active = false;
            }
        }
    }
    
    spawn_obstacle();
}

static int get_dino_y_position() {
    if (!jumping) {
        return GAME_HEIGHT - 1;
    }
    
    int jump_progress = ticks_since_jump;
    int half_jump = 5;
    
    if (jump_progress <= half_jump) {
        return (GAME_HEIGHT - 1) - (jump_progress * MAX_JUMP_HEIGHT / half_jump);
    } else {
        int descent = jump_progress - half_jump;
        return (GAME_HEIGHT - 1 - MAX_JUMP_HEIGHT) + (descent * MAX_JUMP_HEIGHT / half_jump);
    }
}

static bool check_collision() {
    int dino_y = get_dino_y_position();
    
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            if (obstacles[i].x == DINO_X_POSITION && dino_y == GAME_HEIGHT - 1) {
                return true;
            }
        }
    }
    return false;
}



static void process_input() {
    if (!has_input()) return;
    
    char key = read_char();
    
    switch (key) {
        case 'q': case 'Q':
            cleanup_terminal();
            printf("\nGame Over\n");
            exit(0);
            break;
        case ' ':
            if (!jumping) {
                jumping = true;
                ticks_since_jump = 0;
            }
            break;
    }
}

static void update_game() {
    process_input();
    
    if (jumping) {
        if (ticks_since_jump >= 10) {
            jumping = false;
            ticks_since_jump = 0;
        } else {
            ticks_since_jump++;
        }
    }
    
    update_obstacles();
    
    if (check_collision()) {
        cleanup_terminal();
        printf("\nCOLLISION! Game Over\n");
        exit(0);
    }
}

static void render_frame() {
    system(CLEAR_CMD);
    
    printf("\n");
    
    // Top border
    printf("┌");
    for (int x = 0; x < FLOOR_LENGTH; x++) printf("──");
    printf("┐\n");
    
    int dino_y = get_dino_y_position();
    
    for (int y = 0; y < GAME_HEIGHT; y++) {
        printf("│");
        for (int x = 0; x < FLOOR_LENGTH; x++) {
            bool obstacle_here = false;
            
            for (int i = 0; i < MAX_OBSTACLES; i++) {
                if (obstacles[i].active && obstacles[i].x == x && y == GAME_HEIGHT - 1) {
                    printf("\033[32m|\033[0m ");
                    obstacle_here = true;
                    break;
                }
            }
            
            if (!obstacle_here) {
                if (x == DINO_X_POSITION && y == dino_y) {
                    printf("@ ");
                } else {
                    printf("  ");
                }
            }
        }
        printf("│\n");
    }
    
    // Bottom border
    printf("└");
    for (int x = 0; x < FLOOR_LENGTH; x++) printf("──");
    printf("┘\n");
    
    // Status information
    int active_obstacles = 0;
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) active_obstacles++;
    }
    
    printf("\nDino Y: %d | Jumping: %s | Active obstacles: %d\n",
           dino_y, jumping ? "True" : "False", active_obstacles);
    printf("Controls: Space to jump, Q to quit\n");
}

int main() {
    srand(time(NULL));
    
    setup_terminal();
    init_obstacles();
    
    while(true) {
        score++;
        update_game();
        render_frame();
        usleep(MICROSECONDS_PER_TICK);
    }
    
    cleanup_terminal();
    return 0;
}