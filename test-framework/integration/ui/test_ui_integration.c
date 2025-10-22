#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "src/ui.h"

int main() {
    printf("Testing UI functions...\n");
    
    // Test if we're in a TTY
    if (!isatty(STDOUT_FILENO) || !isatty(STDIN_FILENO)) {
        printf("Not in a TTY, testing non-interactive mode...\n");
        run_ui();
        printf("Non-interactive UI test completed!\n");
    } else {
        printf("In a TTY, this would test interactive mode...\n");
        printf("Skipping interactive test to avoid ncurses issues.\n");
    }
    
    return 0;
}
