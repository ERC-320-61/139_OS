#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> // For INT_MAX
#include <stdbool.h> // Needed for the 'bool' type

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

#endif 
