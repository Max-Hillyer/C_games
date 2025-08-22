#include <stdio.h>
#include <stdlib.h> 

int main() {
    int input = 0;
    printf("1 - snake game\n"); 
    printf("2 - dinosaur game\n");
    printf("3 - 2048\n");
    printf("> ");
    scanf("%d", &input);
    switch(input) {
        case 1: system("make snake_game; ./snake_game"); break;
        case 2: system("make Dino; ./Dino"); break;
        case 3: system("make 2048; ./2048"); break;
        default: printf("Invalid input\n"); break;
    }
    return 0;
}