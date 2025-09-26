// a1p1.c — CPSC 457 A1 Part I (no shared memory)
// Compile: gcc -O2 -Wall src/a1p1.c -o a1p1
// Run:     ./a1p1 < testN.txt   (N = 1..6)  or any 100x1000 int grid

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ROWS 100
#define COLS 1000

int main(void) {
    static int grid[ROWS][COLS];

    // Read 100x1000 ints
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (scanf("%d", &grid[r][c]) != 1) {
                fprintf(stderr, "Input parse error at [%d,%d]\n", r, c);
                return 1;
            }
        }
    }

    // Spawn one child per row
    for (int r = 0; r < ROWS; r++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        } else if (pid == 0) {
#ifdef DEBUG
            printf("Child %d (PID %d): Searching row %d\n", r, getpid(), r);
            fflush(stdout);
#endif
            int found = 0;
            for (int c = 0; c < COLS; c++) {
                if (grid[r][c] == 1) { found = 1; break; }
            }
            _exit(found ? (r + 1) : 0); // return r+1 if found, else 0
        }
    }

    // Collect results: choose the smallest row index that reported "found"
    int best_row = -1;
    pid_t winner_pid = -1;
    for (int i = 0; i < ROWS; i++) {
        int status = 0;
        pid_t w = wait(&status);
        if (w < 0) { perror("wait"); return 1; }
        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            if (code >= 1 && code <= ROWS) {
                int row = code - 1;
                if (best_row == -1 || row < best_row) {
                    best_row = row;
                    winner_pid = w;
                }
            }
        }
    }

    if (best_row == -1) {
        printf("Parent: No treasure found\n");
        return 0;
    }

    // Find first column with 1 in the winning row (for a precise report)
    int found_col = -1;
    for (int c = 0; c < COLS; c++) {
        if (grid[best_row][c] == 1) { found_col = c; break; }
    }

    printf("Parent: The treasure was found by child with PID %d at row %d and column %d\n",
           winner_pid, best_row, found_col);
    return 0;
}

