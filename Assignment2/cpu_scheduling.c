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
#include "scheduler.h"
#include "scheduler_utils.h"



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





 

/****** ROUND ROBIN ******/
void round_robin(Process procs[], int n, int quantum) {
    int current_time = 0;                                                                   // Initialize current time to 0 for the start of scheduling
    int exec_time;                                                                          // Declare a variable to hold the execution time of a process during a quantum slice
    int completed = 0;                                                                      // Counter for the number of processes that have completed execution
    Queue queue;                                                                            // Declare a queue to manage the processes ready for execution
    queue.count = 0;                                                                        // Initialize the process count in the queue to 0


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
            calculate_waiting_time(proc_ptr, current_time);                                 // Calculate waiting time
        }

    }

    print_process_time_results(procs, n);                                                   // Call to print results
    calculate_waiting_average(procs, n);                                                    // Calculate and print the average waiting time
}



/****** SHORTEST JOB FIRST ******/
void sjf(Process procs[], int n) {
    int current_time = 0;
    bool completed[n];
    for (int i = 0; i < n; i++) {
        completed[i] = false;
    }
    int processes_completed = 0;

    printf("\n    -SJF-\n");  // Header for SJF output
    printf(" Time   Process    \n");      // Print header for results
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
            // Print the current time and the process number about to start execution
            printf("%4d\t%4d\n", current_time, procs[idx].process_number);

            current_time += procs[idx].cpu_burst_time; // Execute the process
            procs[idx].finish_time = current_time;    // Set the finish time
            calculate_waiting_time(&procs[idx], current_time); // Calculate its waiting time
            completed[idx] = true;  // Mark the process as completed
            processes_completed++;  // Increment the count of completed processes
        }
    }
    calculate_waiting_average(procs, n);  // Print average waiting time
}



/****** PRIORITY SCHEDULING WITHOUT PREEMPTION ******/
void pr_noPREMP(Process procs[], int n) {
    int current_time = 0;
    bool completed[n];
    for (int i = 0; i < n; i++) {
        completed[i] = false;
    }
    int processes_completed = 0;

    printf("\n -Pr_noPREMP- \n");
    printf(" Time   Process    \n");      // Print header for results
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
            printf("%4d\t%4d\n", current_time, procs[idx].process_number);  // Print start time and process number
            current_time += procs[idx].cpu_burst_time;  // Execute the process
            procs[idx].finish_time = current_time;
            calculate_waiting_time(&procs[idx], current_time);  // Calculate waiting time
            completed[idx] = true;
            processes_completed++;
        }
    }

    calculate_waiting_average(procs, n);  // Calculate and print the average waiting time
}



/****** PRIORITY SCHEDULING WITH PREEMPTION ******/
void PR_PREMP(Process procs[], int n) {
    int current_time = 0;
    int idx = -1;
    int last_process = -1;  // Track the last running process to handle preemption checks accurately.

    printf("PR_withPREMP\n");
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
                printf("%d\t%d\n", current_time, p->process_number);  // Print when a process starts or is preempted
                p->has_started = true;
                last_process = idx;  // Update last process to current
            }

            // Run this process for one unit of time
            p->remaining_time--;

            // FOR DEBBUGING: printf("Executing: Time %d, Process %d, Remaining Time %d\n", current_time, p->process_number, p->remaining_time);
            
            if (p->remaining_time == 0) {
                p->is_complete = true;
                calculate_waiting_time_prempt(p, current_time + 1);  // Process completes at the next time unit
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

    calculate_waiting_average(procs, n);
}
