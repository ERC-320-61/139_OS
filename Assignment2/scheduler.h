#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h> // Necessary for the 'bool' type

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
void print_process_time_results(Process procs[], int n);
void calculate_waiting_average(Process procs[], int n);
void print_process_burst_times(Process procs[], int n);
int initialize_scheduling(const char* filename_base, Process* procs, int* n, char* scheduling_algo, int* quantum);


#endif // SCHEDULER_H
