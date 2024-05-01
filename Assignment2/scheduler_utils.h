#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

void calculate_waiting_average(Process procs[], int n) {
    double total_waiting_time = 0.0;
    for (int i = 0; i < n; i++) {
        total_waiting_time += procs[i].waiting_time;
    }
    double average_waiting_time = total_waiting_time / n;
    printf("\nAverage Waiting Time: %.2f\n", average_waiting_time);
}

void calculate_waiting_time(Process *proc, int current_time) {
    proc->waiting_time = current_time - proc->arrival_time - proc->cpu_burst_time;
}


void print_process_time_results(Process procs[], int n) {
    printf("      Process         Waiting (T)    Finish (T)\n");
    printf("-------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf("%10d %15d %15d\n", procs[i].process_number, procs[i].waiting_time, procs[i].finish_time);
    }
}

void print_process_burst_times(Process procs[], int n) {
    printf("      Process       Burst (T)\n");
    printf("-----------------------------\n");                  
    for (int i = 0; i < n; i++) {
        printf("%10d %15d\n", procs[i].process_number, procs[i].cpu_burst_time);
    }
}

void execute_schedule(const char* algo, Process* procs, int n, int quantum) {
    printf("\nExecuting %s scheduling...\n", algo);
    print_process_burst_times(procs, n);

    if (strcmp(algo, "RR") == 0) {
        round_robin(procs, n, quantum);
    } else if (strcmp(algo, "SJF") == 0) {
        sjf(procs, n);
    } else if (strcmp(algo, "PR_noPREMP") == 0) {
        pr_noPREMP(procs, n);
    } else if (strcmp(algo, "PR_withPREMP") == 0) {
        PR_PREMP(procs, n);
    } else {
        fprintf(stderr, "Invalid scheduling algorithm specified.\n");
        exit(EXIT_FAILURE);
    }
}

int initialize_scheduling(const char* filename_base, Process* procs, int* n, char* scheduling_algo, int* quantum) {
    char filename[50];
    FILE *file_ptr = NULL;

    for (int file_index = -1; file_index < 100; file_index++) {
        if (file_index == -1) {
            snprintf(filename, sizeof(filename), "%s.txt", filename_base);
        } else {
            snprintf(filename, sizeof(filename), "%s%d.txt", filename_base, file_index);
        }
        
        file_ptr = fopen(filename, "r");
        if (file_ptr) {
            printf("Opened %s successfully...\n", filename);
            break;
        }
    }

    if (!file_ptr) {
        perror("Error opening any input file, cannot proceed...");
        return -1;
    }

    if (fscanf(file_ptr, "%s", scheduling_algo) != 1) {
        fprintf(stderr, "Failed to read scheduling algorithm\n");
        fclose(file_ptr);
        return -1;
    }

    if (strcmp(scheduling_algo, "RR") == 0) {
        if (fscanf(file_ptr, "%d", quantum) != 1) {
            fprintf(stderr, "Failed to read quantum for RR\n");
            fclose(file_ptr);
            return -1;
        }
    }

    if (fscanf(file_ptr, "%d", n) != 1) {
        fprintf(stderr, "Failed to read number of processes\n");
        fclose(file_ptr);
        return -1;
    }

    for (int i = 0; i < *n; i++) {
        if (fscanf(file_ptr, "%d %d %d %d",
                   &procs[i].process_number,
                   &procs[i].arrival_time,
                   &procs[i].cpu_burst_time,
                   &procs[i].priority) != 4) {
            fprintf(stderr, "Failed to read data for process %d\n", i);
            fclose(file_ptr);
            return -1;
        }
        procs[i].remaining_time = procs[i].cpu_burst_time;
        procs[i].has_started = false;
        procs[i].start_time = procs[i].finish_time = procs[i].response_time = procs[i].waiting_time = procs[i].last_execution_time = 0;
    }

    fclose(file_ptr);
    return 0; // Success
}
