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
#include <stdbool.h>
#include <limits.h>

#define MAX_PROCESSES 100


/*
* Used `typedef struct` to define structure of a `Process` 
* These attributes make it easier to manage CPU Scheduling
*/
typedef struct {
    bool has_started;           // Boolean flag to indicate if the process has started execution
    int process_number;         // Unique identifier
    int priority;               // Priority for scheduling
    int start_time;             // Time when the process starts its execution
    int finish_time;            // Time when the process finishes its execution
    int arrival_time;           // Time when the process arrives
    int waiting_time;           // Total time the process has been in the ready queue
    int response_time;          // Time from arrival until the first time the process is scheduled on the CPU
    int remaining_time;         // Time remaining for the process to complete execution
    int cpu_burst_time;         // Time the process requires CPU
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


// Declaration of algorithm Functions
void sjf(Process procs[], int n);
void PR_PREMP(Process procs[], int n);
void pr_noPREMP(Process procs[], int n);
void round_robin(Process procs[], int n, int quantum);

// Declaration of helper functions
void calculate_waiting_times(Process procs[], int n, int is_preemptive);
void print_process_time_results(Process procs[], int n);
void calculate_waiting_average(Process procs[], int n);
void print_process_burst_times(Process procs[], int n);
int initialize_scheduling(const char* filename_base, Process* procs, int* n, char* scheduling_algo, int* quantum);




/****** MAIN ******/
int main() {
    Process procs[MAX_PROCESSES];
    int n, quantum;
    char scheduling_algo[25];

    if (initialize_scheduling("input", procs, &n, scheduling_algo, &quantum) != 0) {
        return EXIT_FAILURE;
    }

    execute_schedule(scheduling_algo, procs, n, quantum);

    return 0;  // Return 0 to indicate successful execution
}





/****** SCHEDULING ALGORITHMS ******/

/* ROUND ROBIN */
void round_robin(Process procs[], int n, int quantum) {
    int current_time = 0;
    int exec_time;
    int completed = 0;
    Queue queue;
    queue.count = 0;


    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival_time <= current_time && !procs[i].has_started) {           // Check if process has arrived AND not started
                queue.processes[queue.count++] = &procs[i];                                 // Enqueue the process
                procs[i].has_started = 1;                                                   // Mark as started
            }
        }

        if (queue.count == 0) {                                                             // Check If Queue Is Empty
            current_time++;                                                                 // Increment the current time
            continue;                                                                       // Skip to the next iteration of the loop
        }

        Process *proc_ptr = queue.processes[0];                                             // Create pointer for curent process & get the first process in the queue
        for (int i = 0; i < queue.count - 1; i++) {                                         // Loop to shift all processes in the queue up by one position
            queue.processes[i] = queue.processes[i + 1];                                    // Move each process one position forward in the queue
        }
        queue.count--;                                                                      // Decrease count as the first process is taken out for execution

        if (proc_ptr->remaining_time < quantum) {                                           // If remaining time is less than the quantum
            exec_time = proc_ptr->remaining_time;                                           // Assign remaining time to exec_time
        } else {
            exec_time = quantum;                                                            // Else assign quantum to exec_time if remaining time is more
        }
        proc_ptr->remaining_time -= exec_time;                                              // Subtract the execution time from the process's remaining time
        current_time += exec_time;                                                          // Increment the current time by the execution time

        
        if (proc_ptr->remaining_time > 0) {                                                 // Check if there is still execution time left for the process
            queue.processes[queue.count++] = proc_ptr;                                      // Reinsert the process into the queue if it has time left
        } else {
            completed++;                                                                    // Increment count of completed processes
            proc_ptr->finish_time = current_time;                                           // Set finish time for the process
            proc_ptr->waiting_time = proc_ptr->finish_time - proc_ptr->arrival_time - proc_ptr->cpu_burst_time;  // Calculate waiting time
        }

    }

    print_process_time_results(procs, n);  // Call to print results
    calculate_waiting_average(procs, n);   // Calculate and print the average waiting time
}


/* SHORTEST JOB FIRST */
void sjf(Process procs[], int n) {
    int current_time = 0;
    bool completed[n];
    for (int i = 0; i < n; i++) {
        completed[i] = false;
    }
    int processes_completed = 0;

    while (processes_completed < n) {
        int shortest_time = INT_MAX;
        int idx = -1;
        for (int i = 0; i < n; i++) {
            if (!completed[i] && procs[i].arrival_time <= current_time && procs[i].cpu_burst_time < shortest_time) {
                shortest_time = procs[i].cpu_burst_time;
                idx = i;
            }
        }
        if (idx == -1) {
            current_time++;
        } else {
            current_time += procs[idx].cpu_burst_time;
            procs[idx].finish_time = current_time;
            procs[idx].waiting_time = current_time - procs[idx].arrival_time - procs[idx].cpu_burst_time;
            completed[idx] = true;
            processes_completed++;
        }
    }

    print_process_time_results(procs, n);  // Call to print results
    calculate_waiting_average(procs, n);   // Calculate and print the average waiting time
}



/* PRIORITY SCHEDULING WITHOUT PREEMPTION */
void pr_noPREMP(Process procs[], int n) {
    int current_time = 0;
    bool completed[n];
    for (int i = 0; i < n; i++) {
        completed[i] = false;
    }
    int processes_completed = 0;

    while (processes_completed < n) {
        int highest_priority = INT_MAX;
        int idx = -1;

        // Find the process with the highest priority that has arrived
        for (int i = 0; i < n; i++) {
            if (!completed[i] && procs[i].arrival_time <= current_time && procs[i].priority < highest_priority) {
                highest_priority = procs[i].priority;
                idx = i;
            }
        }

        if (idx == -1) {
            current_time++;
        } else {
            // Run the selected process
            current_time += procs[idx].cpu_burst_time;
            procs[idx].finish_time = current_time;
            procs[idx].waiting_time = current_time - procs[idx].arrival_time - procs[idx].cpu_burst_time;
            completed[idx] = true;
            processes_completed++;
        }

    }

    print_process_time_results(procs, n);  // Call to print results
    calculate_waiting_average(procs, n);   // Calculate and print the average waiting time
}


//* PRIORITY SCHEDULING WITH PREEMPTION */
void PR_PREMP(Process procs[], int n) {
    int current_time = 0;
    int current_process_idx = -1;
    int current_process_end_time = INT_MAX;

    // Initialize an array to keep track of completion status of processes
    bool completed[n];
    for (int i = 0; i < n; i++) {
        completed[i] = false;
        procs[i].has_started = false;  // Ensure that has_started is correctly initialized
    }

    while (true) {
        // Check for new arrivals and possible preemption
        bool found_higher_priority = false;
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival_time <= current_time && !procs[i].has_started && !completed[i]) {
                if (current_process_idx == -1 || (procs[i].priority < procs[current_process_idx].priority && procs[i].remaining_time > 0)) {
                    if (current_process_idx != -1 && current_time < current_process_end_time) {
                        procs[current_process_idx].remaining_time = current_process_end_time - current_time;  // Update remaining time for preempted process
                    }
                    current_process_idx = i;
                    current_process_end_time = current_time + procs[i].cpu_burst_time;
                    procs[i].has_started = true;
                    found_higher_priority = true;
                }
            }
        }

        // Process the execution of the current process
        if (current_process_idx != -1 && !found_higher_priority) {
            if (current_time >= current_process_end_time) {
                procs[current_process_idx].finish_time = current_time;
                procs[current_process_idx].waiting_time = current_time - procs[current_process_idx].arrival_time - procs[current_process_idx].cpu_burst_time;
                completed[current_process_idx] = true;  // Mark as completed
                current_process_idx = -1;  // Reset current process
            }
        }

        // Check if all processes are completed
        bool all_done = true;
        for (int i = 0; i < n; i++) {
            if (!completed[i]) {
                all_done = false;
                break;
            }
        }
        if (all_done) {
            break;  // Exit if all processes are done
        }

        current_time++;  // Increment the simulation time
    }

    print_process_time_results(procs, n);  // Call to print results
    calculate_waiting_average(procs, n);  // Call to calculate and print average waiting time
}





/****** HELPER FUNCTIONS ******/
void calculate_waiting_average(Process procs[], int n) {
    double total_waiting_time = 0.0;
    for (int i = 0; i < n; i++) {
        total_waiting_time += procs[i].waiting_time;
    }
    double average_waiting_time = total_waiting_time / n;
    printf("\nverage Waiting Time: %.2f\n", average_waiting_time);
}


void calculate_waiting_times(Process procs[], int n, int is_preemptive) {
    double total_waiting_time = 0.0;
    for (int i = 0; i < n; i++) {
        if (!is_preemptive) {
            // For non-preemptive scheduling, waiting time is straightforward
            procs[i].waiting_time = procs[i].finish_time - procs[i].arrival_time - procs[i].cpu_burst_time;
        } else {
            // For preemptive scheduling, consider last execution times or similar
            // Here we need additional logic if we keep track of all context switches
            procs[i].waiting_time = procs[i].finish_time - procs[i].arrival_time - procs[i].cpu_burst_time;
            // This can be more complex based on how context switches are handled
        }
        total_waiting_time += procs[i].waiting_time;
        printf("Process %d: Waiting Time = %d\n", procs[i].process_number, procs[i].waiting_time);
    }
    double avg_waiting_time = total_waiting_time / n;
    printf("Average Waiting Time = %.2f\n", avg_waiting_time);
}


/* PRINT HEADERS */
void print_process_time_results(Process procs[], int n) {
    printf("      Process         Waiting (T)    Finish (T)\n");    // Print header
    printf("-------------------------------------------\n");
    for (int i = 0; i < n; i++) {                                   // Print each process's data with fixed-width fields for alignment
        printf("%10d %15d %15d\n", 
               procs[i].process_number, procs[i].waiting_time, procs[i].finish_time);
    }
}

void print_process_burst_times(Process procs[], int n) {
    printf("      Process       Burst (T)\n");                  // Print header
    printf("-----------------------------\n");                  
    for (int i = 0; i < n; i++) {
        printf("%10d %15d\n",                                   // Print each process's data with fixed-width fields for alignment
               procs[i].process_number, procs[i].cpu_burst_time);
    }
}


/* HANDLE RUNNING THE ALGORITHMS HERE */
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
        exit(EXIT_FAILURE); // or handle error differently based on your design
    }

}


/* HANDLE ALL FILE OPERATIONS HERE */
int initialize_scheduling(const char* filename_base, Process* procs, int* n, char* scheduling_algo, int* quantum) {
    char filename[50];
    FILE *file_ptr = NULL;
    
    // Try to open "input.txt" or "input#.txt" where # is 0-99
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


