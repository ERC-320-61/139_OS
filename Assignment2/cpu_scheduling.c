/*
CSC139 
Spring 2024
Second Assignment: CPU Scheduling
Delgado, Eric
Section #03
OSs Tested on: Linux Only
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 100


/*
* Used `typedef struct` to define structure of a `Process` 
* These attributes make it easier to manage CPU Scheduling
*/
typedef struct {
    int process_number;         // Unique identifier for the process
    int arrival_time;           // Time when the process arrives and is ready to be scheduled
    int cpu_burst_time;         // Total time the process requires on the CPU
    int priority;               // Priority for scheduling (lower number means higher priority)
    int remaining_time;         // Time remaining for the process to complete execution
    int start_time;             // Time when the process starts its execution on the CPU
    int finish_time;            // Time when the process finishes its execution
    int response_time;          // Time from arrival until the first time the process is scheduled on the CPU
    int waiting_time;           // Total time the process has been in the ready queue
    int has_started;            // Boolean flag to indicate if the process has started execution
    int last_execution_time;    // The last time when the process was executed on the CPU (for response time in pre-emptive algorithms)
} Process;


/*
* The `Queue` structure holds an array of pointers to `Process` structures, 
* enabling the queue operations required for process management.
*/
typedef struct {
    Process *processes[MAX_PROCESSES];  // Array holding pointers to 'Process' instances, with 'MAX_PROCESSES' as its capacity.
    int count;                          // Counts the number of 'Process' pointers currently in the queue.
} Queue;


// Assignment Goal Specific Function declarations
void round_robin(Process procs[], int n, int quantum);
void sjf(Process procs[], int n);
void priority_no_PR(Process procs[], int n);
void priority_PR(Process procs[], int n);

// Logic Specific Function declarations
void calculate_waiting_times(Process procs[], int n, int is_preemptive);


/****** MAIN ******/
int main() {
    
    FILE *file_ptr;                 // Pointer to the file used for input operations
    char scheduling_algo[20];       // Array to store the name of the scheduling algorithm
    int quantum = 0, n, i;          // quantum: Time quantum for Round Robin, n: Number of processes, i: Loop index
    Process procs[MAX_PROCESSES];   // Array to hold process data

    file_ptr = fopen("input.txt", "r");
    if (!file_ptr) {
        perror("Error opening file, cannot proceed...");            // Use perror to display error message with errno info if file opening fails
        return -1;
    }

    // Read the scheduling algorithm
    if (fscanf(file_ptr, "%s", scheduling_algo) != 1) {
        fprintf(stderr, "Failed to read scheduling algorithm\n");   // Use fprintf to stderr to ensure error message is seen even if stdout is redirected
        fclose(file_ptr);
        return -1;
    }

    // Check if the scheduling algorithm is Round Robin ("RR"); if so, read the time quantum. 
    if (strcmp(scheduling_algo, "RR") == 0) {
        if (fscanf(file_ptr, "%d", &quantum) != 1) {
            fprintf(stderr, "Failed to read quantum for RR\n");     // Use fprintf to stderr to ensure error message is seen even if stdout is redirected
            fclose(file_ptr);
            return -1;
        }
    }

    // Read the number of processes
    if (fscanf(file_ptr, "%d", &n) != 1) {
        fprintf(stderr, "Failed to read number of processes\n");    // Use fprintf to stderr to ensure error message is seen even if stdout is redirected
        fclose(file_ptr);
        return -1;
    }

    // Read each process data
    for (i = 0; i < n; i++) {
        if (fscanf(file_ptr, "%d %d %d %d",
                &procs[i].process_number,
                &procs[i].arrival_time,
                &procs[i].cpu_burst_time,
                &procs[i].priority) != 4) {
            fprintf(stderr, "Failed to read data for process %d\n", i); // Use fprintf to stderr for error messages to ensure they are seen even if stdout is redirected
            fclose(file_ptr);
            return -1;
        }
        procs[i].remaining_time = procs[i].cpu_burst_time;  // Set remaining time to the initial CPU burst time
        procs[i].has_started = 0;                           // Initially, the process has not started
        procs[i].start_time = 0;                            // Start time will be set when the process first gets the CPU
        procs[i].finish_time = 0;                           // Finish time will be set when the process completes
        procs[i].response_time = 0;                         // Response time will be calculated during scheduling
        procs[i].waiting_time = 0;                          // Waiting time will be calculated based on scheduling dynamics
        procs[i].last_execution_time = 0;                   // Last execution time will be used for preemptive scheduling
    }


    fclose(file_ptr);                                           // Close the file to free system resources and flush output if necessary
    if (strcmp(scheduling_algo, "RR") == 0) { 
        round_robin(procs, n, quantum);                         // Execute Round Robin scheduling if algorithm is "RR"
    } else if (strcmp(scheduling_algo, "SJF") == 0) {
        sjf(procs, n);                                          // Execute Shortest Job First scheduling if algorithm is "SJF"
    } else if (strcmp(scheduling_algo, "PR_noPREMP") == 0) {
        priority_np(procs, n);                                  // Execute Non-Preemptive Priority scheduling if algorithm is "PR_noPREMP"
    } else if (strcmp(scheduling_algo, "PR_withPREMP") == 0) {
        priority_p(procs, n);                                   // Execute Preemptive Priority scheduling if algorithm is "PR_withPREMP"
    } else {
        fprintf(stderr, "Invalid scheduling algorithm\n");      // Print error to stderr if no valid algorithm is found
        return -1;                                              // Return -1 to indicate an error situation
    }
    return 0;                                                   // Return 0 to indicate successful execution

}

