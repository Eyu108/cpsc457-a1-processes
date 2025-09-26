/* a1p2.c — CPSC 457 A1 Part II (shared memory)
 * Compile: gcc -O2 -Wall src/a1p2.c -o a1p2 -lm
 * Run:     ./a1p2 <lower> <upper> <nchildren>
 *
 * Shared memory layout:
 *  For each child i (0..n-1) we allocate a block of (1 + block_len) ints:
 *    [0] = count of primes found by that child
 *    [1..count] = the primes
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

static int is_prime(int x) {
    if (x < 2) return 0;
    if (x == 2 || x == 3) return 1;
    if (x % 2 == 0) return 0;
    int r = (int) floor(sqrt((double)x));
    for (int d = 3; d <= r; d += 2) {
        if (x % d == 0) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <lower> <upper> <nchildren>\n", argv[0]);
        return 1;
    }
    long lower = strtol(argv[1], NULL, 10);
    long upper = strtol(argv[2], NULL, 10);
    int n = (int) strtol(argv[3], NULL, 10);

    if (lower > upper || n <= 0) {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }

    long count_nums = upper - lower + 1;
    long block_len = (count_nums + n - 1) / n;           // ceil division
    size_t per_child = 1 + (size_t) block_len;           // [count, primes...]
    size_t ints_total = (size_t) n * per_child;
    size_t bytes_total = ints_total * sizeof(int);

    // Create shared memory
    int shmid = shmget(IPC_PRIVATE, bytes_total, IPC_CREAT | 0600);
    if (shmid < 0) { perror("shmget"); return 1; }

    int *shm = (int *) shmat(shmid, NULL, 0);
    if (shm == (void *) -1) { perror("shmat"); shmctl(shmid, IPC_RMID, NULL); return 1; }

    // Zero out
    for (size_t i = 0; i < ints_total; ++i) shm[i] = 0;

    // Fork children
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // Best effort cleanup
            for (int k = 0; k < i; ++k) wait(NULL);
            shmdt(shm);
            shmctl(shmid, IPC_RMID, NULL);
            return 1;
        } else if (pid == 0) {
            // Child i
            long start = lower + (long) i * block_len;
            long end   = start + block_len - 1;
            if (end > upper) end = upper;

            size_t base = (size_t) i * per_child;
            int cnt = 0;

            for (long x = start; x <= end; ++x) {
                if (is_prime((int) x)) {
                    if ((size_t) cnt < per_child - 1) {
                        shm[base + 1 + (size_t) cnt] = (int) x;
                        ++cnt;
                    }
                }
            }
            shm[base + 0] = cnt;

#ifdef DEBUG
            // Print per-child primes in DEBUG mode only
            if (cnt > 0) {
                printf("Child %d: %d primes ->", i, cnt);
                for (int k = 0; k < cnt; ++k) printf(" %d", shm[base + 1 + (size_t) k]);
                printf("\n");
            } else {
                printf("Child %d: 0 primes\n", i);
            }
            fflush(stdout);
#endif
            _exit(0);
        }
    }

    // Parent: wait all
    for (int i = 0; i < n; ++i) wait(NULL);

    // Parent: summarize
    int total_primes = 0;
    for (int i = 0; i < n; ++i) {
        size_t base = (size_t) i * per_child;
        int cnt = shm[base];
        total_primes += cnt;
#ifndef DEBUG
        (void)cnt;   // silence unused warning in non-DEBUG
#endif
    }

    printf("Total primes in [%ld, %ld] with %d processes: %d\n",
           lower, upper, n, total_primes);

    // Cleanup
    if (shmdt(shm) < 0) perror("shmdt");
    if (shmctl(shmid, IPC_RMID, NULL) < 0) perror("shmctl(IPC_RMID)");
    return 0;
}
