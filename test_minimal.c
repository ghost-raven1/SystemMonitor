#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <ncurses.h>

int main() {
    printf("Starting minimal test...\n");
    
    // Test basic CoreFoundation
    printf("Testing CoreFoundation...\n");
    
    // Test ncurses
    printf("Testing ncurses...\n");
    setlocale(LC_ALL, "");
    if (initscr() == NULL) {
        printf("Error: Unable to initialize ncurses\n");
        return 1;
    }
    printf("ncurses initialized successfully\n");
    
    noecho();
    curs_set(FALSE);
    
    printf("Clearing screen...\n");
    clear();
    
    printf("Printing test message...\n");
    mvprintw(0, 0, "Test message");
    refresh();
    
    printf("Ending ncurses...\n");
    endwin();
    
    printf("Test completed successfully\n");
    return 0;
}
