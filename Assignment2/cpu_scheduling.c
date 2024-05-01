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
            if (!completed[i] && procs[i].arrival_time <= current_time && procs[i].cpu_burst_time < shortest_time) {
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

    print_process_time_results(procs, n);                                                   // Call to print the process time results
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

        // Find the process with the highest priority that has arrived
        for (int i = 0; i < n; i++) {
            if (!completed[i] && procs[i].arrival_time <= current_time && procs[i].priority < highest_priority) {
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

    print_process_time_results(procs, n);                                                   // Print results for each process
    calculate_waiting_average(procs, n);                                                    // Calculate and print the average waiting time of all processes
}



/****** PRIORITY SCHEDULING WITH PREEMPTION ******/
void PR_PREMP(Process procs[], int n) {
    int current_time = 0;                                                                   // Initialize current simulation time
    int current_process_idx = -1;                                                           // Index for tracking the currently executing process
    int current_process_end_time = INT_MAX;                                                 // End time for the currently executing process
    bool completed[n];                                                                      // Initialize an array to keep track of completion status of processes

    for (int i = 0; i < n; i++) {
        completed[i] = false;                                                               // Initialize all processes as not completed
        procs[i].has_started = false;                                                       // Mark all processes as not started
    }

    while (true) {                                                                          // While all process aren't done, check for new arrivals and possible preemption
        bool found_higher_priority = false;                                                 // Flag to check if a higher priority process is found
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival_time <= current_time && !procs[i].has_started && !completed[i]) {            // Check if the process is ready to run and has not been completed or started
                if (current_process_idx == -1 || (procs[i].priority < procs[current_process_idx].priority && procs[i].remaining_time > 0)) {    // Check for no current process or a higher priority process is found
                    if (current_process_idx != -1 && current_time < current_process_end_time) {               // Preempt the current process if a higher priority process is found
                        procs[current_process_idx].remaining_time = current_process_end_time - current_time;  // Save the remaining time for the preempted process
                    }
                    current_process_idx = i;                                                // Update the current process index
                    current_process_end_time = current_time + procs[i].cpu_burst_time;      // Set the end time for the current process
                    procs[i].has_started = true;                                            // Mark the process as started
                    found_higher_priority = true;                                           // Set the flag for higher priority found
                }
            }
        }

        if (current_process_idx != -1 && !found_higher_priority) {                          // If no higher priority process was found, execute the current process 
            if (current_time >= current_process_end_time) {                                 // Check if the current time has reached the process end time
                procs[current_process_idx].finish_time = current_time;                      // Set the finish time for the process
                calculate_waiting_time(&procs[current_process_idx], current_time);          // Calculate waiting time
                completed[current_process_idx] = true;                                      // Mark the process as completed
                current_process_idx = -1;                                                   // Reset the current process index
            }
        }

        // Check if all processes are completed
        bool all_done = true;
        for (int i = 0; i < n; i++) {
            if (!completed[i]) {
                all_done = false;                                                           // If any process is not completed, continue the loop
                break;
            }
        }
        if (all_done) {                                                                     // if all procces are done
            break;                                                                          // Exit the loop 
        }

        current_time++;                                                                     // Increment the simulation time
    }

    print_process_time_results(procs, n);                                                   // Call to print results
    calculate_waiting_average(procs, n);                                                    // Calculate and print the average waiting time
}
