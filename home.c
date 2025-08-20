#include <stdio.h>
#include <stdlib.h> 
#ifdef _WIN32
    #define CLEAR_CMD "cls"
#else 
    #define CLEAR_CMD "clear"
#endif

int main() {
    system(CLEAR_CMD);
    int input = 0;
    printf("1 - snake game\n");
    printf("2 - dinosaur game\n");
    printf("> ");
    scanf("%d", &input);
    switch(input) {
        case 1: system("make snake_game; ./snake_game");
        case 2: system("make Dino; ./Dino");
    }
    return 0;
}