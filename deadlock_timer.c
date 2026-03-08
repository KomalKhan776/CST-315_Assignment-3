#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_PROCESSES 3
#define STARVATION_LIMIT 3   // seconds before process is starved
#define MAX_CYCLES 3         // times each process will try to run

pthread_mutex_t resource;
pthread_mutex_t log_mutex;

FILE *logFile;

typedef struct {
    int id;
    char resource_name[20];
    int restarts;
} Process;

void write_log(const char *message) {
    pthread_mutex_lock(&log_mutex);
    fprintf(logFile, "%s\n", message);
    fflush(logFile);
    printf("%s\n", message);   // show on screen
    pthread_mutex_unlock(&log_mutex);
}

void *process_function(void *arg) {
    Process *p = (Process *)arg;
    int cycle;

    for (cycle = 1; cycle <= MAX_CYCLES; cycle++) {
        time_t start_wait = time(NULL);
        int acquired = 0;
        char buffer[200];

        sprintf(buffer,
                "Process P%d is Running and trying to access resource %s.",
                p->id, p->resource_name);
        write_log(buffer);

        while (!acquired) {
            if (pthread_mutex_trylock(&resource) == 0) {
                acquired = 1;

                sprintf(buffer,
                        "Process P%d accessed resource %s successfully. Resource available = NO. Starved = NO.",
                        p->id, p->resource_name);
                write_log(buffer);

                // simulate resource usage
                if (p->id == 1 && cycle == 1) {
                    sleep(5);  // hold longer to make others starve
                } else {
                    sleep(2);
                }

                sprintf(buffer,
                        "Process P%d released resource %s. Resource available = YES.",
                        p->id, p->resource_name);
                write_log(buffer);

                pthread_mutex_unlock(&resource);
            } else {
                time_t current_wait = time(NULL);
                double waited = difftime(current_wait, start_wait);

                sprintf(buffer,
                        "Process P%d waiting for resource %s. Resource available = NO. Wait time = %.0f second(s).",
                        p->id, p->resource_name, waited);
                write_log(buffer);

                if (waited >= STARVATION_LIMIT) {
                    sprintf(buffer,
                            "Process P%d is STARVED. Timer expired after %.0f second(s). Process stopped and restarted.",
                            p->id, waited);
                    write_log(buffer);

                    p->restarts++;

                    sprintf(buffer,
                            "Process P%d restart count = %d.",
                            p->id, p->restarts);
                    write_log(buffer);

                    // restart attempt
                    start_wait = time(NULL);
                }

                sleep(1);
            }
        }

        sleep(1);
    }

    char buffer[150];
    sprintf(buffer,
            "Process P%d finished execution with %d restart(s).",
            p->id, p->restarts);
    write_log(buffer);

    return NULL;
}

int main() {
    pthread_t threads[NUM_PROCESSES];
    Process processes[NUM_PROCESSES];

    logFile = fopen("activity_log.txt", "w");
    if (logFile == NULL) {
        printf("Error opening log file.\n");
        return 1;
    }

    pthread_mutex_init(&resource, NULL);
    pthread_mutex_init(&log_mutex, NULL);

    fprintf(logFile, "===== Deadlock avoidance =====\n\n");
    fflush(logFile);

    for (int i = 0; i < NUM_PROCESSES; i++) {
        processes[i].id = i + 1;
        snprintf(processes[i].resource_name, sizeof(processes[i].resource_name), "File_A");
        processes[i].restarts = 0;

        pthread_create(&threads[i], NULL, process_function, &processes[i]);
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pthread_join(threads[i], NULL);
    }

    fprintf(logFile, "\n===== done =====\n");ubuntu
    fclose(logFile);

    pthread_mutex_destroy(&resource);
    pthread_mutex_destroy(&log_mutex);

    printf("\nProgram finished. Check activity_log.txt for the full log.\n");

    return 0;
}
