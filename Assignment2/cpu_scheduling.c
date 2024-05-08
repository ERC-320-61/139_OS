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
      These attributes make it easier to manage CPU Scheduling
*/
typedef struct
{
    bool has_started;   // Boolean flag to indicate if the process has started execution
    int process_number; // Unique identifier
    int priority;       // Priority for scheduling
    int start_time;     // Time when the process starts its execution
    int finish_time;    // Time when the process finishes its execution
    bool is_complete;
    int arrival_time;        // Time when the process arrives
    int waiting_time;        // Total time the process has been in the ready queue
    int response_time;       // Time from arrival until the first time the process is scheduled on the CPU
    int remaining_time;      // Time remaining for the process to complete execution
    int cpu_burst_time;      // Time the process requires CPU
    int last_execution_time; // The last time when the process was executed on the CPU (for response time in pre-emptive algorithms)
} Process;

/*
    * The `Queue` structure holds an array of pointers to `Process` structures,
      enabling the queue operations required for process management.
*/
typedef struct
{
    Process *processes[MAX_PROCESSES]; // Array holding pointers to 'Process' instances, with 'MAX_PROCESSES' as its capacity.
    int count;                         // Counts the number of 'Process' pointers currently in the queue.
    int front;
    int rear;
} Queue;

/*  Declaration of algorithm Functions */
void sjf(Process procs[], int n);
void PR_PREMP(Process procs[], int n);
void pr_noPREMP(Process procs[], int n);
void round_robin(Process procs[], int n, int quantum);

/****** MAIN ******/
int main() {
    Process procs[MAX_PROCESSES];
    int n, quantum = 0;
    char scheduling_algo[25];
    char filename[50];
    FILE *file_ptr = NULL, *output_ptr = NULL;

    snprintf(filename, sizeof(filename), "input.txt");
    file_ptr = fopen(filename, "r");
    if (!file_ptr) {
        perror("Error opening input file");
        return EXIT_FAILURE;
    }

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

    if (fscanf(file_ptr, "%d", &n) != 1) {
        fprintf(stderr, "Failed to read number of processes\n");
        fclose(file_ptr);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < n; i++) {
        if (fscanf(file_ptr, "%d %d %d %d", &procs[i].process_number, &procs[i].arrival_time, &procs[i].cpu_burst_time, &procs[i].priority) != 4) {
            fprintf(stderr, "Failed to read data for process %d\n", i);
            fclose(file_ptr);
            return EXIT_FAILURE;
        }
        procs[i].remaining_time = procs[i].cpu_burst_time;
        procs[i].has_started = false;
        procs[i].is_complete = false;
    }
    fclose(file_ptr);

    output_ptr = freopen("output.txt", "w", stdout); // Redirect stdout to output.txt
    if (!output_ptr) {
        perror("Error opening output file");
        return EXIT_FAILURE;
    }

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
        fclose(output_ptr); // Close the file if scheduling algorithm is invalid
        return EXIT_FAILURE;
    }

    fclose(output_ptr); // Close the output file
    return 0;
}

/****** ROUND ROBIN SCHEDULING ******/
/*
    * This function implements the Round Robin scheduling algorithm with a specified quantum
    * It simulates a process scheduler that assigns CPU time in a cyclic manner, ensuring all processes receive equal time slices, typically termed as 'quantum'
    
    * Each process is represented with a structure containing details like arrival time, CPU burst time, and other status flags
    * The function manages processes using a circular queue structure to facilitate the round-robin ordering
    
    * Processes are checked and enqueued based on their arrival; If no process is ready, the function simply advances the simulation time
    * Once a process is dequeued for execution, it either runs for its remaining time or for the duration of the quantum, whichever is less
    * Waiting times are calculated for each process each time they are scheduled to run, accumulating total waiting times for later averaging
    * The scheduler continues until all processes have been completed, and then it calculates the average waiting time across all processes
 */
void round_robin(Process procs[], int n, int quantum) {
    int current_time = 0;                                      // Tracks the current time in the scheduler
    Queue queue = {0, 0, 0};                                   // Initialize an empty queue for processes
    double total_waiting_time = 0.0;                           // Sum of all waiting times for average calculation
    int completed = 0;                                         // Counter for completed processes

    printf("\n-RR- Quantum Slice: %d\n", quantum);             // Display the quantum size at the start
    printf("Time\tProcess\tEvent\n");
    printf("----------------------------\n");

    while (completed < n) {                                    // Continue until all processes are completed
        // Enqueue processes that have arrived by the current time
        for (int i = 0; i < n; i++) {
            if (procs[i].arrival_time <= current_time && !procs[i].has_started) {
                procs[i].waiting_time = 0;                     // Reset waiting time
                procs[i].has_started = true;                   // Mark process as started
                procs[i].last_execution_time = current_time;   // Set the last execution time to current time
                queue.processes[queue.rear] = &procs[i];       // Enqueue the process
                queue.rear = (queue.rear + 1) % MAX_PROCESSES; // Move the rear pointer in a circular manner
                queue.count++;                                 // Increment the count of processes in the queue
            }
        }

        if (queue.count == 0) {                                // If no process is in the queue, increment time
            current_time++;
            continue;
        }

        // Dequeue the first process to run it
        Process* proc_ptr = queue.processes[queue.front];      // Get the process at the front of the queue
        queue.front = (queue.front + 1) % MAX_PROCESSES;       // Move the front pointer in a circular manner
        queue.count--;                                         // Decrement the count of processes in the queue

        // Calculate waiting time since last executed
        proc_ptr->waiting_time += current_time - proc_ptr->last_execution_time;

        printf("%4d\t%4d\n", current_time, proc_ptr->process_number); // Print the time and process number

        int exec_time;                                                // Variable to hold the execution time for the current process

        // Determine execution time based on remaining time and quantum
        if (proc_ptr->remaining_time < quantum) {
            exec_time = proc_ptr->remaining_time;                     // If remaining time is less than quantum, use remaining time
        } else {
            exec_time = quantum;                                      // If remaining time is more than or equal to quantum, use quantum
        }


        current_time += exec_time;                                    // Advance current time by execution time
        proc_ptr->remaining_time -= exec_time;                        // Reduce remaining time of the process
        proc_ptr->last_execution_time = current_time;                 // Update last execution time

        // Check if the process is finished
        if (proc_ptr->remaining_time == 0) {
            proc_ptr->is_complete = true;                             // Mark process as complete
            completed++;                                              // Increment completed process count
            total_waiting_time += proc_ptr->waiting_time;             // Add to total waiting time
            // FOR DEBUGGING printf("Last process %d: Added %d to total waiting time, new total: %f\n", proc_ptr->process_number, proc_ptr->waiting_time, total_waiting_time);
        } else {
            // Re-enqueue if not finished
            queue.processes[queue.rear] = proc_ptr;                   // Put process back at the end of the queue
            queue.rear = (queue.rear + 1) % MAX_PROCESSES;            // Move the rear pointer in a circular manner
            queue.count++;                                            // Increment the count of processes in the queue
        }
    }

    double average_waiting_time = total_waiting_time / n;             // Calculate average waiting time
    printf("Average Waiting Time: %.2f\n", average_waiting_time);     // Print the average waiting time
}



/****** SHORTEST JOB FIRST ******/
/*
    * This function schedules processes based on their CPU burst times
    * The process with the shortest CPU burst time is selected next

    * Processes are stored in an array with their attributes: arrival time, CPU burst time, and status flags
      for completion

    * The function iterates over time units, checking for the process that can execute next
      focusing on the one with the shortest burst time available at the current time

    * If no process is ready to execute, the current time is incremented
    * When a process is selected, it runs to completion without interruption
    * Waiting time for each process is calculated directly as the difference between the current time and the process's arrival time

    * The function accumulates total waiting time for each process to calculate and print the average waiting
      time after all processes have completed, demonstrating the efficiency of the scheduling.
 */
void sjf(Process procs[], int n)
{
    int current_time = 0;
    bool completed[n];
    double total_waiting_time = 0.0; // For calculating average waiting time

    for (int i = 0; i < n; i++)
    {
        completed[i] = false;
    }
    int processes_completed = 0;

    printf("\n    -SJF-\n");
    printf(" Time   Process\n");
    printf("-------------------\n");
    while (processes_completed < n)
    {
        int shortest_time = INT_MAX;
        int idx = -1;
        for (int i = 0; i < n; i++)
        {
            if (!completed[i] && procs[i].arrival_time <= current_time && procs[i].cpu_burst_time < shortest_time)
            { // Find the shortest job that is ready to run
                shortest_time = procs[i].cpu_burst_time;
                idx = i;
            }
        }
        if (idx == -1)
        {
            current_time++; // Increment time if no process can be executed
        }
        else
        {
            Process *p = &procs[idx];
            printf("%4d\t%4d\n", current_time, p->process_number); // Print the current time and process number

            p->waiting_time = current_time - p->arrival_time; // Direct calculation of waiting time
            total_waiting_time += p->waiting_time;            // Accumulate waiting time for average calculation

            current_time += p->cpu_burst_time; // Execute the process
            p->finish_time = current_time;     // Set the finish time
            completed[idx] = true;             // Mark the process as completed
            processes_completed++;             // Increment the count of completed processes
        }
    }

    double average_waiting_time = (processes_completed > 0) ? total_waiting_time / processes_completed : 0.0; // Calculate and print average waiting time after all processes complete
    printf("AVG Waiting Time: %.2f\n", average_waiting_time);
}

/****** PRIORITY SCHEDULING WITHOUT PREEMPTION ******/
/*
    * This function schedules processes based on their priority without interrupting a running process until it completes

    * Processes are stored in an array with their attributes: arrival time, CPU burst time, priority,
      and status flags for completion and start

    * The function iterates over time units, checking for processes that can execute
    * It selects the process with the highest priority (lowest numerical value) that has arrived by the current time

    * If no process is ready to execute, the current time is incremented.
    * Its waiting time is calculated as the difference between its start time and arrival time

    * The function accumulates total waiting time for each process to calculate and print the average
      waiting time after all processes have completed, demonstrating the scheduling efficiency.
 */
void pr_noPREMP(Process procs[], int n)
{
    int current_time = 0;
    bool completed[n];
    double total_waiting_time = 0.0;

    for (int i = 0; i < n; i++)
    {
        completed[i] = false;
    }
    int processes_completed = 0;

    printf("\n -Pr_noPREMP- \n");
    printf(" Time   Process    \n");
    printf("-------------------\n");
    while (processes_completed < n)
    {
        int highest_priority = INT_MAX;
        int idx = -1;

        for (int i = 0; i < n; i++)
        {
            if (!completed[i] && procs[i].arrival_time <= current_time && procs[i].priority < highest_priority)
            { // Find the highest priority process that is ready to run
                highest_priority = procs[i].priority;
                idx = i;
            }
        }

        if (idx == -1)
        {
            current_time++; // No process is ready, increment the current time
        }
        else
        {
            Process *p = &procs[idx];
            printf("%4d\t%4d\n", current_time, p->process_number); // Print start time and process number
            p->start_time = current_time;                          // Record the start time
            current_time += p->cpu_burst_time;                     // Execute the process
            p->finish_time = current_time;
            p->waiting_time = p->start_time - p->arrival_time; // Direct calculation of waiting time
            total_waiting_time += p->waiting_time;             // Accumulate total waiting time for average calculation
            completed[idx] = true;
            processes_completed++;
        }
    }

    double average_waiting_time = (processes_completed > 0) ? total_waiting_time / processes_completed : 0.0; // Calculate and print average waiting time after all processes complete
    printf("AVG Waiting Time: %.2f\n", average_waiting_time);
}

/****** PRIORITY SCHEDULING WITH PREEMPTION ******/
/*
    * This function schedules the process with the highest priority  that has arrived by the current time
    * If a higher-priority process arrives while another is running, the current process is preempted

    * Processes are stored in an array with their attributes: arrival time, CPU burst time, priority,
      and status flags for completion and start

    * The function iterates over time units, checking for processes that can execute.
    * If a process starts or is preempted, it prints the current time and process number

    * Once a process completes, its waiting time is calculated as the difference between the current time
      and its arrival and burst times

    * The function tracks the last running process to detect preemptions
      and handles newly arriving higher priority processes at the start of each time unit

    * After all processes are complete, the function calculates and prints the average waiting time of all
      processes, demonstrating the efficiency of the scheduling
 */
void PR_PREMP(Process procs[], int n)
{
    int current_time = 0;
    int idx = -1;
    int last_process = -1;

    printf("\n PR_withPREMP\n");
    printf(" Time   Process    \n"); // Print header for results
    printf("-------------------\n");
    while (true)
    {
        int min_priority = INT_MAX;
        for (int i = 0; i < n; i++)
        {
            if (!procs[i].is_complete && procs[i].arrival_time <= current_time && procs[i].priority < min_priority)
            {
                min_priority = procs[i].priority;
                idx = i;
            }
        }

        if (idx != -1)
        { // There is a process that can run
            Process *p = &procs[idx];

            if (!p->has_started || (last_process != idx && p->remaining_time > 0))
            {                                                          // Ensure that the process is reported as starting whenever it is actually going to execute
                printf("%4d\t%4d\n", current_time, p->process_number); // Print when a process starts or is preempted
                p->has_started = true;
                last_process = idx; // Update last process to current
            }

            p->remaining_time--; // Update process for runtime to decrmenet
            if (p->remaining_time == 0)
            {
                p->is_complete = true;
                p->waiting_time = current_time + 1 - p->arrival_time - p->cpu_burst_time; // Directly calculate and store waiting time when a process completes
                // FOR DEBUGGING: printf("Completing: Time %d, Process %d, Waiting Time %d\n", current_time + 1, p->process_number, p->waiting_time);
            }

            for (int j = 0; j < n; j++)
            {
                if (!procs[j].is_complete && procs[j].arrival_time == current_time + 1 && procs[j].priority < p->priority)
                {            // Check for higher priority process arrivals for the next time unit
                    idx = j; // Preempt current process
                    break;
                }
            }
        }

        bool all_done = true;
        for (int i = 0; i < n; i++)
        { // Check for completion of all processes
            if (!procs[i].is_complete)
            {
                all_done = false;
                break;
            }
        }
        if (all_done)
            break;

        current_time++; // Move to the next time unit
    }

    double total_waiting_time = 0.0;
    for (int i = 0; i < n; i++)
    {
        total_waiting_time += procs[i].waiting_time; // Calculate and print average waiting time after all processes complete
    }
    double average_waiting_time = total_waiting_time / n;
    printf("AVG Waiting Time: %.2f\n", average_waiting_time);
}
