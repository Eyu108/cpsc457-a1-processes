/* a1p2.c — CPSC 457 A1 Part II (shared memory)
 * Compile: gcc -O2 -Wall src/a1p2.c -o a1p2 -lm
 * Run:     ./a1p2 LOWER UPPER NPROCS
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stddef.h>
#include <errno.h>

static int is_prime(long x) {
    if (x < 2) return 0;
    if (x % 2 == 0) return x == 2;
    long r = (long) sqrt((double) x);
    for (long d = 3; d <= r; d += 2) {
        if (x % d == 0) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s LOWER UPPER NPROCS\n", argv[0]);
        return 1;
    }
    long lower = atol(argv[1]);
    long upper = atol(argv[2]);
    int n = atoi(argv[3]);

    if (upper < lower) { long t = lower; lower = upper; upper = t; }
    if (n < 1) n = 1;

    long total = upper - lower + 1;
    if (total < 1) {
        fprintf(stderr, "Empty range.\n");
        return 1;
    }
    if (n > total) n = (int) total;

    long block_len = (total + n - 1) / n;           // ceil division
    size_t per_child = 1 + (size_t) block_len;      // [count, primes...]
    size_t ints_total = (size_t) n * per_child;
    size_t bytes_total = ints_total * sizeof(int);

    int shmid = shmget(IPC_PRIVATE, bytes_total, IPC_CREAT | 0600);
    if (shmid < 0) { perror("shmget"); return 1; }

    int *shm = (int *) shmat(shmid, NULL, 0);
    if (shm == (void*) -1) { perror("shmat"); return 1; }
    for (size_t i = 0; i < ints_total; ++i) shm[i] = 0;

    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }
        if (pid == 0) {
            long start = lower + (long) i * block_len;
            long end   = start + block_len - 1;
            if (end > upper) end = upper;

            size_t base = (size_t) i * per_child;
            for (long x = start; x <= end; ++x) {
                if (is_prime(x)) {
                    int cnt = shm[base];
                    if ((size_t) cnt < per_child - 1) {
                        shm[base + 1 + (size_t) cnt] = (int) x;
                        shm[base] = cnt + 1;
                    }
                }
            }
            _exit(0);
        }
    }

    for (int i = 0; i < n; ++i) {
        int status;
        if (wait(&status) < 0) { perror("wait"); }
    }

    int total_primes = 0;
    for (int i = 0; i < n; ++i) {
        size_t base = (size_t) i * per_child;
        int cnt = shm[base];
        printf("Child %d: %d primes", i, cnt);
        if (cnt > 0) {
            printf(" ->");
            for (int k = 0; k < cnt; ++k) {
                printf(" %d", shm[base + 1 + (size_t) k]);
            }
        }
        printf("\n");
        total_primes += cnt;
    }
    printf("Total primes in [%ld, %ld] with %d processes: %d\n",
           lower, upper, n, total_primes);

    if (shmdt(shm) < 0) perror("shmdt");
    if (shmctl(shmid, IPC_RMID, NULL) < 0) perror("shmctl(IPC_RMID)");
    return 0;
}
