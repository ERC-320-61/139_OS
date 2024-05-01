#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

#define MAX_EVENTS 100
SchedulingEvent events[MAX_EVENTS];
int events_count = 0;


/***** CALCULATE AVERAGE WAITING TIME *****/
void calculate_waiting_average(Process procs[], int n) {
    double total_waiting_time = 0.0;                                 // Initialize total waiting time to zero
    for (int i = 0; i < n; i++) {                                    // Loop through all processes to sum up waiting times
        total_waiting_time += procs[i].waiting_time;                 
    }
    double average_waiting_time = total_waiting_time / n;            // Calculate average waiting time
    printf("\nAverage Waiting Time: %.2f\n", average_waiting_time);  // Print average waiting time
}

/***** CALCULATE WAITING TIME *****/
void calculate_waiting_time(Process *proc, int current_time) {
    proc->waiting_time = current_time - proc->arrival_time - proc->cpu_burst_time;  // Calculate waiting time for a process
}

/***** SHOW PROCESS SCHEDULING EVENTS *****/
void print_process_time_results(Process procs[], int n, const char* scheduling_algo) {
    double total_waiting_time = 0.0;

    // Print the scheduling algorithm name
    printf("%s:\n", scheduling_algo);
    printf("Time Point    Process Number\n");
    printf("---------------------------\n");

    // Assuming 'events' is an array or list that has been populated with each scheduling event
    for (int i = 0; i < events_count; i++) {
        printf("%5d %16d\n", events[i].time_point, events[i].process_number);
        if (i < n) {
            total_waiting_time += procs[i].waiting_time;
        }
    }
}

/***** SHOW PROCESSES DETAILS <FOR DEBUGGING>*****/
void print_process_details(Process procs[], int n, const char* algo) {
    // Check the scheduling algorithm to determine which headers to print
    if (strcmp(algo, "PR_noPREMP") == 0 || strcmp(algo, "PR_withPREMP") == 0) {
        printf("      Process       Burst (T)    Priority\n");          // Include Priority column for priority scheduling
        printf("-----------------------------------------\n");         
        for (int i = 0; i < n; i++) {                                    
            printf("%10d %15d %10d\n",                                 // Print process ID, CPU burst time, and priority
                   procs[i].process_number, procs[i].cpu_burst_time, procs[i].priority);
        }
    } else if (strcmp(algo, "SJF") == 0) {
        printf("      Process       Burst (T)    Arrival (T)\n");      // Include Arrival Time column for SJF scheduling
        printf("-------------------------------------------\n");       
        for (int i = 0; i < n; i++) {                                    
            printf("%10d %15d %12d\n",                                 // Print process ID, CPU burst time, and arrival time
                   procs[i].process_number, procs[i].cpu_burst_time, procs[i].arrival_time);
        }
    } else {                                                           // Default to showing only burst times if the algorithm doesn't need additional columns
        printf("      Process       Burst (T)\n");                     // Print header for burst times
        printf("-----------------------------\n");                       
        for (int i = 0; i < n; i++) {                                    
            printf("%10d %15d\n",                                      // Print process ID and CPU burst time
                   procs[i].process_number, procs[i].cpu_burst_time);
        }
    }
}


/***** EXECUTE SCHEDULING *****/
void execute_schedule(const char* algo, Process* procs, int n, int quantum) {
    // print_process_details(procs, n, algo);                           FOR DEBUGGING: Print process details based on the algorithm

    if (strcmp(algo, "RR") == 0) {                                   // If the algorithm is Round Robin
        round_robin(procs, n, quantum);                              // Execute Round Robin scheduling
    } else if (strcmp(algo, "SJF") == 0) {                           // If the algorithm is Shortest Job First
        sjf(procs, n);                                               // Execute Shortest Job First scheduling
    } else if (strcmp(algo, "PR_noPREMP") == 0) {                    // If the algorithm is Non-Preemptive Priority
        pr_noPREMP(procs, n);                                        // Execute Non-Preemptive Priority scheduling
    } else if (strcmp(algo, "PR_withPREMP") == 0) {                  // If the algorithm is Preemptive Priority
        PR_PREMP(procs, n);                                          // Execute Preemptive Priority scheduling
    } else {                                                         // If the algorithm is not recognized
        fprintf(stderr, "Invalid scheduling algorithm specified.\n");
        exit(EXIT_FAILURE);                                          // Print error message and exit with failure
    }
}

/***** PRIORITY COMPARISON *****/
// Sort processes by priority (lower number means higher priority)
int compare_priority(const void *a, const void *b) {
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    return (p1->priority - p2->priority);
}


/***** INITIALIZE SCHEDULING *****/
int initialize_scheduling(const char* filename_base, Process* procs, int* n, char* scheduling_algo, int* quantum) {
    char filename[50];                                               // Buffer to store filename
    FILE *file_ptr = NULL;                                           // File pointer initialized to NULL

    for (int file_index = -1; file_index < 100; file_index++) {      // Loop to attempt opening different formatted file names
        if (file_index == -1) {
            snprintf(filename, sizeof(filename), "%s.txt", filename_base);  // Format filename for the base case
        } else {
            snprintf(filename, sizeof(filename), "%s%d.txt", filename_base, file_index);  // Format filename with numbers
        }
        
        file_ptr = fopen(filename, "r");                             // Attempt to open file
        if (file_ptr) {                                              // If file opens successfully
            // printf("Opened %s successfully...\n", filename);         FOR DEBUGGING Print success message
            break;                                                   
        }
    }

    if (!file_ptr) {                                                // If file was not opened successfully
        perror("Error opening any input file, cannot proceed...");  // Print error message using perror
        return -1;                                                  // Return failure code
    }

    if (fscanf(file_ptr, "%s", scheduling_algo) != 1) {             // Attempt to read scheduling algorithm from file
        fprintf(stderr, "Failed to read scheduling algorithm\n");   // Print failure message
        fclose(file_ptr);                                           
        return -1;                                                  // Return failure code
    }

    if (strcmp(scheduling_algo, "RR") == 0) {                       // If algorithm is Round Robin, read quantum value
        if (fscanf(file_ptr, "%d", quantum) != 1) {
            fprintf(stderr, "Failed to read quantum for RR\n");     // Print failure message
            fclose(file_ptr);                                       
            return -1;                                              // Return failure code
        }
    }

    if (fscanf(file_ptr, "%d", n) != 1) {                           // Attempt to read number of processes
        fprintf(stderr, "Failed to read number of processes\n");    // Print failure message
        fclose(file_ptr);                                           
        return -1;                                                  // Return failure code
    }

    for (int i = 0; i < *n; i++) {                                  // Loop to read data for each process
        if (fscanf(file_ptr, "%d %d %d %d",                         // Attempt to read process data
                   &procs[i].process_number,
                   &procs[i].arrival_time,
                   &procs[i].cpu_burst_time,
                   &procs[i].priority) != 4) {
            fprintf(stderr, "Failed to read data for process %d\n", i);  // Print failure message
            fclose(file_ptr);                                            
            return -1;                                                   // Return failure code
        }
        procs[i].remaining_time = procs[i].cpu_burst_time;               // Set remaining time equal to CPU burst time
        procs[i].has_started = false;                                    // Mark process as not started
        procs[i].start_time = procs[i].finish_time = procs[i].response_time = procs[i].waiting_time = procs[i].last_execution_time = 0;  // Initialize all timing data
    }

    fclose(file_ptr);                                                    // Close file after reading data
    return 0;                                                            // Return success code
}
