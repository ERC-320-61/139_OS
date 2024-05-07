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
    bool is_complete;
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

typedef struct {
    Process **queue;
    int size;
    int capacity;
} PriorityQueue;

// Declaration of algorithm Functions
void sjf(Process procs[], int n);
void PR_PREMP(Process procs[], int n);
void pr_noPREMP(Process procs[], int n);
void round_robin(Process procs[], int n, int quantum);

// Declaration of helper function
void print_process_burst_times(Process procs[], int n);                         // Function to print the CPU burst times for all processes  
void calculate_waiting_average(Process procs[], int n);                         // Function to calculate and print the average waiting time for all processes
void print_process_time_results(Process procs[], int n);                        // Function to print the results for process times
void calculate_waiting_time(Process *proc, int current_time);                   // Function to calculate the waiting time for a process based on current simulation time
void execute_schedule(const char* algo, Process* procs, int n, int quantum);    // Function to execute scheduling based on the specified algorithm
int initialize_scheduling(const char* filename_base, Process* procs, int* n, char* scheduling_algo, int* quantum);      // Function to initialize scheduling from a specified file base name

void initQueue(PriorityQueue *pq, int capacity);

void enqueue(PriorityQueue *pq, Process *proc);

bool isEmpty(PriorityQueue *pq);

Process* dequeue(PriorityQueue *pq);



/****** MAIN ******/
int main() {
    Process procs[MAX_PROCESSES];
    int n, quantum = 0;
    char scheduling_algo[25];
    char filename[50];
    FILE *file_ptr = NULL;

    // Try to open a specific input file
    snprintf(filename, sizeof(filename), "input.txt");
    file_ptr = fopen(filename, "r");
    if (!file_ptr) {
        perror("Error opening input file");
        return EXIT_FAILURE;
    }

    // Read scheduling algorithm type and possibly quantum
    if (fscanf(file_ptr, "%s", scheduling_algo) != 1) {
        fprintf(stderr, "Failed to read scheduling algorithm\n");
        fclose(file_ptr);
        return EXIT_FAILURE;
    }
    if (strcmp(scheduling_algo, "RR") == 0) {
        if (fscanf(file_ptr, "%d", &quantum) != 1) {
            fprintf(stderr, "Failed to read quantum for RR\n");
            fclose(file_ptr);
            return EXIT_FAILURE;
        }
    }

    // Read number of processes
    if (fscanf(file_ptr, "%d", &n) != 1) {
        fprintf(stderr, "Failed to read number of processes\n");
        fclose(file_ptr);
        return EXIT_FAILURE;
    }

    // Read process data
    for (int i = 0; i < n; i++) {
        if (fscanf(file_ptr, "%d %d %d %d",
                   &procs[i].process_number,
                   &procs[i].arrival_time,
                   &procs[i].cpu_burst_time,
                   &procs[i].priority) != 4) {
            fprintf(stderr, "Failed to read data for process %d\n", i);
            fclose(file_ptr);
            return EXIT_FAILURE;
        }
        procs[i].remaining_time = procs[i].cpu_burst_time;
        procs[i].has_started = false;
        procs[i].is_complete = false;
    }
    fclose(file_ptr);

    // Execute scheduling based on the algorithm specified
    if (strcmp(scheduling_algo, "RR") == 0) {
        round_robin(procs, n, quantum);
    } else if (strcmp(scheduling_algo, "SJF") == 0) {
        sjf(procs, n);
    } else if (strcmp(scheduling_algo, "PR_noPREMP") == 0) {
        pr_noPREMP(procs, n);
    } else if (strcmp(scheduling_algo, "PR_withPREMP") == 0) {
        PR_PREMP(procs, n);
    } else {
        fprintf(stderr, "Invalid scheduling algorithm specified.\n");
        return EXIT_FAILURE;
    }

    return 0;  // Indicate successful execution
}





 

/****** ROUND ROBIN ******/
void round_robin(Process procs[], int n, int quantum) {
    int current_time = 0;
    Queue queue = { .count = 0 };
    double total_waiting_time = 0.0;
    int completed = 0;

    while (completed < n) {
        // Enqueue newly arrived processes
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival_time <= current_time && !procs[i].has_started) {
                queue.processes[queue.count++] = &procs[i];
                procs[i].has_started = true;
            }
        }

        if (queue.count == 0) {
            current_time++;
            continue;
        }

        // Dequeue the first process in the queue
        Process *proc_ptr = queue.processes[0];
        for (int i = 0; i < queue.count - 1; i++) {
            queue.processes[i] = queue.processes[i + 1];
        }
        queue.count--;

        // Determine execution time for this quantum
        int exec_time = (proc_ptr->remaining_time < quantum) ? proc_ptr->remaining_time : quantum;
        proc_ptr->remaining_time -= exec_time;
        current_time += exec_time;

        // Check if the process has finished execution
        if (proc_ptr->remaining_time == 0) {
            proc_ptr->finish_time = current_time;
            proc_ptr->waiting_time = proc_ptr->finish_time - proc_ptr->arrival_time - proc_ptr->cpu_burst_time;
            total_waiting_time += proc_ptr->waiting_time;
            completed++;
        } else {
            queue.processes[queue.count++] = proc_ptr; // Re-enqueue if not completed
        }
    }

    // Print results directly
    printf("Processes completion and waiting times:\n");
    printf("Process\tFinish Time\tWaiting Time\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t\t%d\n", procs[i].process_number, procs[i].finish_time, procs[i].waiting_time);
    }

    // Calculate and print the average waiting time
    double average_waiting_time = total_waiting_time / n;
    printf("Average Waiting Time: %.2f\n", average_waiting_time);
}



/****** SHORTEST JOB FIRST ******/
void sjf(Process procs[], int n) {
    int current_time = 0;
    bool completed[n];
    double total_waiting_time = 0.0;  // For calculating average waiting time

    for (int i = 0; i < n; i++) {
        completed[i] = false;
    }
    int processes_completed = 0;

    printf("\n    -SJF-\n");
    printf(" Time   Process\n");
    printf("-------------------\n");   
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
            current_time++;  // Increment time if no process can be executed
        } else {
            Process *p = &procs[idx];
            printf("%4d\t%4d\n", current_time, p->process_number);

            p->waiting_time = current_time - p->arrival_time;  // Direct calculation of waiting time
            total_waiting_time += p->waiting_time;  // Accumulate waiting time for average calculation

            current_time += p->cpu_burst_time; // Execute the process
            p->finish_time = current_time;    // Set the finish time
            completed[idx] = true;  // Mark the process as completed
            processes_completed++;  // Increment the count of completed processes
        }
    }

    // Calculate and print average waiting time after all processes complete
    double average_waiting_time = (processes_completed > 0) ? total_waiting_time / processes_completed : 0.0;
    printf("AVG Waiting Time: %.2f\n", average_waiting_time);
}



/****** PRIORITY SCHEDULING WITHOUT PREEMPTION ******/
void pr_noPREMP(Process procs[], int n) {
    int current_time = 0;
    bool completed[n];
    double total_waiting_time = 0.0;  // Moved here for direct calculation

    for (int i = 0; i < n; i++) {
        completed[i] = false;
    }
    int processes_completed = 0;

    printf("\n -Pr_noPREMP- \n");
    printf(" Time   Process    \n");
    printf("-------------------\n");   
    while (processes_completed < n) {
        int highest_priority = INT_MAX;
        int idx = -1;

        for (int i = 0; i < n; i++) {
            if (!completed[i] && procs[i].arrival_time <= current_time && procs[i].priority < highest_priority) {
                highest_priority = procs[i].priority;
                idx = i;
            }
        }

        if (idx == -1) {
            current_time++;  // No process is ready, increment the current time
        } else {
            Process *p = &procs[idx];
            printf("%4d\t%4d\n", current_time, p->process_number);  // Print start time and process number
            p->start_time = current_time;  // Record the start time
            current_time += p->cpu_burst_time;  // Execute the process
            p->finish_time = current_time;
            p->waiting_time = p->start_time - p->arrival_time;  // Direct calculation of waiting time
            total_waiting_time += p->waiting_time;  // Accumulate total waiting time for average calculation
            completed[idx] = true;
            processes_completed++;
        }
    }

    // Calculate and print average waiting time after all processes complete
    double average_waiting_time = (processes_completed > 0) ? total_waiting_time / processes_completed : 0.0;
    printf("AVG Waiting Time: %.2f\n", average_waiting_time);
}




/****** PRIORITY SCHEDULING WITH PREEMPTION ******/
void PR_PREMP(Process procs[], int n) {
    int current_time = 0;
    int idx = -1;
    int last_process = -1;

    printf("\n PR_withPREMP\n");
    printf(" Time   Process    \n");      // Print header for results
    printf("-------------------\n");   
    while (true) {
        int min_priority = INT_MAX;
        for (int i = 0; i < n; i++) {
            if (!procs[i].is_complete && procs[i].arrival_time <= current_time && procs[i].priority < min_priority) {
                min_priority = procs[i].priority;
                idx = i;
            }
        }

        if (idx != -1) {  // There is a process that can run
            Process *p = &procs[idx];

            // Ensure that the process is reported as starting whenever it is actually going to execute
            if (!p->has_started || (last_process != idx && p->remaining_time > 0)) {
                printf("%4d\t%4d\n", current_time, p->process_number);  // Print when a process starts or is preempted
                p->has_started = true;
                last_process = idx;  // Update last process to current
            }

            // Run this process for one unit of time
            p->remaining_time--;
            if (p->remaining_time == 0) {
                p->is_complete = true;
                // Directly calculate and store waiting time when a process completes
                p->waiting_time = current_time + 1 - p->arrival_time - p->cpu_burst_time;
                // FOR DEBUGGING: printf("Completing: Time %d, Process %d, Waiting Time %d\n", current_time + 1, p->process_number, p->waiting_time);
            }

            // Check for higher priority process arrivals for the next time unit
            for (int j = 0; j < n; j++) {
                if (!procs[j].is_complete && procs[j].arrival_time == current_time + 1 && procs[j].priority < p->priority) {
                    idx = j;  // Preempt current process
                    break;
                }
            }
        }

        // Check for completion of all processes
        bool all_done = true;
        for (int i = 0; i < n; i++) {
            if (!procs[i].is_complete) {
                all_done = false;
                break;
            }
        }
        if (all_done) break;

        current_time++;  // Move to the next time unit
    }

    // Calculate and print average waiting time after all processes complete
    double total_waiting_time = 0.0;
    for (int i = 0; i < n; i++) {
        total_waiting_time += procs[i].waiting_time;
    }
    double average_waiting_time = total_waiting_time / n;
    printf("AVG Waiting Time: %.2f\n", average_waiting_time);
}

