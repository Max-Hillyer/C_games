#include <stdio.h>
#include <stdlib.h> 

int main() {
    int input = 0;
    printf("1 - snake game\n"); 
    printf("2 - dinosaur game\n");
    printf("> ");
    scanf("%d", &input);
    switch(input) {
        case 1: system("make snake_game; ./snake_game"); break;
        case 2: system("make Dino; ./Dino"); break;
    }
    return 0;
}