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
    int current_time = 0;
    Queue queue;
    queue.count = 0;

    bool completed[n];
    memset(completed, false, sizeof(completed));  // Track completion of all processes

    int total_active = 0; // Track the number of active processes

    // Initialize queue with processes that are ready at time 0
    for (int i = 0; i < n; i++) {
        if (procs[i].arrival_time <= current_time) {
            queue.processes[queue.count++] = &procs[i];
            procs[i].has_started = true;
            total_active++;
        }
    }

    while (total_active > 0) {
        if (queue.count > 0) {
            Process *proc_ptr = queue.processes[0];

            // Shift queue to handle next process
            for (int i = 0; i < queue.count - 1; i++) {
                queue.processes[i] = queue.processes[i + 1];
            }
            queue.count--;

            int exec_time = proc_ptr->remaining_time < quantum ? proc_ptr->remaining_time : quantum;
            proc_ptr->remaining_time -= exec_time;

            // Log the process execution start
            if (events_count < MAX_EVENTS) {
                events[events_count].time_point = current_time;
                events[events_count].process_number = proc_ptr->process_number;
                events_count++;
            }

            current_time += exec_time; // Advance time by the slice executed

            if (proc_ptr->remaining_time > 0) {
                queue.processes[queue.count++] = proc_ptr; // Re-enqueue if not finished
            } else {
                proc_ptr->finish_time = current_time;
                calculate_waiting_time(proc_ptr, current_time);
                completed[proc_ptr->process_number - 1] = true;
                total_active--; // Reduce the count of active processes
            }
        } else {
            current_time++; // Increment time if no process was ready to execute
        }

        // Enqueue new processes that become ready
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival_time <= current_time && !procs[i].has_started && !completed[i]) {
                queue.processes[queue.count++] = &procs[i];
                procs[i].has_started = true;
                total_active++;
            }
        }
    }

    print_process_time_results(procs, n, "RR");
    calculate_waiting_average(procs, n);
}





/****** SHORTEST JOB FIRST ******/
void sjf(Process procs[], int n) {
    int current_time = 0;                                                                   // Initialize current time to 0
    bool completed[n];                                                                      // Create an array to track completion status of each process
    for (int i = 0; i < n; i++) {
        completed[i] = false;                                                               // Initialize all process completion statuses to false
    }
    int processes_completed = 0;                                                            // Counter for the number of processes that have completed

    while (processes_completed < n) {                                                       // Continue looping until all processes are completed
        int shortest_time = INT_MAX;                                                        // Initialize shortest_time with the maximum possible value
        int idx = -1;                                                                       // Initialize idx as -1, to store the index of the process with the shortest burst time
        for (int i = 0; i < n; i++) {                                                       // Iterate over all processes to find the one with the shortest CPU burst time
            if (!completed[i] && procs[i].arrival_time <= current_time && procs[i].cpu_burst_time < shortest_time) {    // If process i is not completed, has arrived, and has a burst time shorter than the current shortest
                shortest_time = procs[i].cpu_burst_time;                                    // Update shortest_time with the smallest burst time found
                idx = i;                                                                    // Update idx with the index of the process
            }
        }
        if (idx == -1) {                                                                    // If no process is ready to execute
            current_time++;                                                                 // Increment current time by 1 unit
        } else {                                                                            // If a process is ready to execute
            current_time += procs[idx].cpu_burst_time;                                      // Update current_time by adding the burst time of the selected process
            procs[idx].finish_time = current_time;                                          // Set the finish time for the process
            calculate_waiting_time(&procs[idx], current_time);                              // Calculate waiting time
            completed[idx] = true;                                                          // Mark this process as completed
            processes_completed++;                                                          // Increment the count of completed processes
        }
    }

    print_process_time_results(procs, n, "SJF");                             // Call to print the process time results
    calculate_waiting_average(procs, n);                                                    // Calculate and print the average waiting time for all processes
}



/****** PRIORITY SCHEDULING WITHOUT PREEMPTION ******/
void pr_noPREMP(Process procs[], int n) {
    int current_time = 0;                                                                   // Initialize current time to 0 for the start of scheduling
    bool completed[n];                                                                      // Array to track completion status of each process
    for (int i = 0; i < n; i++) {
        completed[i] = false;                                                               // Initialize all processes as not completed
    }
    int processes_completed = 0;                                                            // Counter for the number of processes that have completed execution

    while (processes_completed < n) {                                                       // Continue scheduling until all processes are completed
        int highest_priority = INT_MAX;                                                     // Start with the highest possible priority (lower number means higher priority)
        int idx = -1;                                                                       // Index of the currently selected process for execution

        for (int i = 0; i < n; i++) {                                                       // For all process find the process with the highest priority that has arrived
            if (!completed[i] && procs[i].arrival_time <= current_time && procs[i].priority < highest_priority) { // If process i is not completed, has arrived, and has a burst time shorter than the current shortest
                highest_priority = procs[i].priority;                                       // Update the highest priority found
                idx = i;                                                                    // Update the index of the process with the highest priority
            }
        }

        if (idx == -1) {                                                                    // If no process is ready to run, increment the current time
            current_time++;
        } else {                                                                            // If a process is found
            current_time += procs[idx].cpu_burst_time;                                      // Increase current time by the burst time of the selected process
            procs[idx].finish_time = current_time;                                          // Set finish time for the process
            calculate_waiting_time(&procs[idx], current_time);                              // Calculate waiting time
            completed[idx] = true;                                                          // Mark the process as completed
            processes_completed++;                                                          // Increment the count of completed processes
        }
    }

    print_process_time_results(procs, n, "PR_noPREMP");                                     // Print results for each process
    calculate_waiting_average(procs, n);                                                    // Calculate and print the average waiting time of all processes
}



/***** PRIORITY SCHEDULING WITH PREEMPTION *****/
void PR_PREMP(Process procs[], int n) {
    int current_time = 0;
    bool completed[n];
    for (int i = 0; i < n; i++) {
        completed[i] = false;
        procs[i].has_started = false;
    }

    // Sort processes by priority initially
    qsort(procs, n, sizeof(Process), compare_priority);

    while (true) {
        bool all_done = true;
        for (int i = 0; i < n; i++) {
            if (!completed[i] && procs[i].arrival_time <= current_time) {
                if (!procs[i].has_started || procs[i].remaining_time > 0) {
                    procs[i].has_started = true;
                    printf("Process %d runs from %d to %d\n", procs[i].process_number, current_time, current_time + procs[i].remaining_time);
                    current_time += procs[i].remaining_time;
                    procs[i].remaining_time = 0;
                    procs[i].finish_time = current_time;
                    calculate_waiting_time(&procs[i], current_time);
                    completed[i] = true;
                }
            }
            if (!completed[i]) all_done = false;
        }

        if (all_done) break;

        // Aging to prevent starvation: Increment priority of waiting processes
        for (int i = 0; i < n; i++) {
            if (!completed[i] && procs[i].arrival_time <= current_time) {
                procs[i].priority--;
            }
        }

        // Re-sort processes based on updated priorities
        qsort(procs, n, sizeof(Process), compare_priority);
    }

    print_process_time_results(procs, n, "PR_PREMP");
    calculate_waiting_average(procs, n);
}