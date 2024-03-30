/*
CSC139 
Spring 2024
First Assignment
Delgado, Eric
Section #03
OSs Tested on: Linux Only
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>

// Size of shared memory block
// Pass this to ftruncate and mmap
#define SHM_SIZE 4096

// Global pointer to the shared memory block
// This should receive the return value of mmap
// Don't change this pointer in any function
void* gShmPtr;

// Function signatures
void Producer(int, int, int);
void InitShm(int, int);
void SetBufSize(int);
void SetItemCnt(int);
void SetIn(int);
void SetOut(int);
void SetHeaderVal(int, int);
int GetBufSize();
int GetItemCnt();
int GetIn();
int GetOut();
int GetHeaderVal(int);
void WriteAtBufIndex(int, int);
int ReadAtBufIndex(int);
int GetRand(int, int);

// Start main program
int main(int argc, char* argv[])
{
        // Define shared memory name and other necessary variables
        pid_t pid;
        int bufSize; // Bounded buffer size
        int itemCnt; // Number of items to be produced
        int randSeed; // Seed for the random number generator 

        // Write code to check the validity of the command-line arguments
        // Check for the correct number of command-line arguments
        if(argc != 4){
		fprintf(stderr, "Invalid number of command-line arguments\n");  // Use fprintf to stderr for error messages to ensure they are seen even if stdout is redirected.
		exit(1);
        }
	bufSize = atoi(argv[1]);
	itemCnt = atoi(argv[2]);
	randSeed = atoi(argv[3]);
	
        // Check the validity of the buffer size
        if(bufSize < 2 || bufSize > 600){
                fprintf(stderr, "Error: Buffer size must be between 2 and 600.\n"); // Use fprintf to stderr for error messages to ensure they are seen even if stdout is redirected.

                exit(1);
        }
        // Check the validity of the item count
        if(itemCnt <= 0){
                fprintf(stderr, "Error: Item count must be greater than 0.\n");    // Use fprintf to stderr for error messages to ensure they are seen even if stdout is redirected.

                exit(1);
        }  

        // Function that creates a shared memory segment and initializes its header
        InitShm(bufSize, itemCnt);        

	/* fork a child process */ 
	pid = fork();

	if (pid < 0) { // error occurred 
		fprintf(stderr, "Fork Failed\n");
		exit(1);
	}
	else if (pid == 0) { // child process 
		printf("Launching Consumer \n");
		execlp("./consumer", "consumer", argv[1], argv[2], argv[3], (char *)NULL);
	}
	else { // parent process, parent will wait for the child to complete 
		printf("Starting Producer\n");
		
               // The function that actually implements the production
               Producer(bufSize, itemCnt, randSeed);
		
	       printf("Producer done and waiting for consumer\n");
	       wait(NULL);		
	       printf("Consumer Completed\n");
        }
    
        return 0;
}
// End main program

void InitShm(int bufSize, int itemCnt)
{
        int in = 0;
        int out = 0;
        const char *name = "OS_HW1_EricDelgado"; //Name of shared memory object to be passed to shm_open

        // Write code here to create a shared memory block and map it to gShmPtr
        // Use the above name.
        // **Extremely Important: map the shared memory block for both reading and writing
        // Use PROT_READ | PROT_WRITE
        // Check for any erros when creating a shared memory block
        int fd = shm_open(name, O_RDWR | O_CREAT, 0666);

        // truncate file to set size
        // ftruncate(fd, SHM_SIZE);
        // Configure the size of the shared memory object
        if (ftruncate(fd, SHM_SIZE) == -1) {
                perror("Failure Point:ftruncate; Unable to resize shared memory...");    // Using perror instead of printf or fprintf since it prints out more info on an error

                close(fd);              // Cleans up resources with close(fd) and shm_unlink(name) on ftruncate or mmap failure to prevent leaks and ensure system cleanup.
                shm_unlink(name);       // Deletes a shared memory object name, and, once all processes have unmapped the object, deallocates and destroys the contents of the associated memory region

                exit(1);
        }

        //map to gShmPtr
        //addr, size, ptro, flags, fd, offset
        // Map the shared memory object for both reading and writing
        gShmPtr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (gShmPtr == MAP_FAILED) {
                perror("Failure Point:mmap;  Unable to map shared memory... ");         // Using perror instead of printf or fprintf since it prints out more info on an error
                
                close(fd);             // Cleans up resources with close(fd) and shm_unlink(name) on ftruncate or mmap failure to prevent leaks and ensure system cleanup.
                shm_unlink(name);      // Deletes a shared memory object name, and, once all processes have unmapped the object, deallocates and destroys the contents of the associated memory region

                exit(1);
        }

        // Write code here to set the values of the four integers in the header
        // Just call the functions provided below, like this SetBufSize(bufSize);
        // Initialize header values
        SetBufSize(bufSize);
        SetItemCnt(itemCnt);
        SetIn(0); // Initial index for production is 0
        SetOut(0); // Initial index for consumption is 0
}

void Producer(int bufSize, int itemCnt, int randSeed)
{
        int in = 0;
        int out = 0;

        srand(randSeed);

        // Write code here to produce itemCnt integer values in the range [0-3000]
        // Use the functions provided below to get/set the values of shared variables "in" and "out"
        // Use the provided function WriteAtBufIndex() to write into the bounded buffer

        // Produce itemCnt items
        for (int i = 0; i < itemCnt; i++) {
                // Wait if buffer is full
                while (((in + 1) % bufSize) == GetOut()) {
                        // Buffer is full, wait for consumer to consume an itemclear 
                        usleep(3000); // Sleep for 3ms
                }

                // Generate a random value
                int randValue = GetRand(0, 3000);

                // Write the item to the buffer
                WriteAtBufIndex(in, randValue);

                // Print production message
                printf("Producing Item %d with value %d at Index %d\n", i, randValue, in);

                /* FOR TROUBLE SHOOTING 
                // Print the number of items currently in the buffer 
                
                clout = GetOut(); // Fetch the latest 'out' value
                int itemsInBuffer = in >= out ? in - out : bufSize - (out - in);
                printf("Items in buffer: %d\n", itemsInBuffer);
                */

                // Update the index for the next item
                in = (in + 1) % bufSize;

                // Update the shared variable 'in'
                SetIn(in);
        }

        printf("Producer Completed\n");
}


// Set the value of shared variable "bufSize"
void SetBufSize(int val)
{
        SetHeaderVal(0, val);
}

// Set the value of shared variable "itemCnt"
void SetItemCnt(int val)
{
        SetHeaderVal(1, val);
}

// Set the value of shared variable "in"
void SetIn(int val)
{
        SetHeaderVal(2, val);
}

// Set the value of shared variable "out"
void SetOut(int val)
{
        SetHeaderVal(3, val);
}

// Get the ith value in the header
int GetHeaderVal(int i)
{
        int val;
        void* ptr = gShmPtr + i*sizeof(int);
        memcpy(&val, ptr, sizeof(int));
        return val;
}

// Set the ith value in the header
void SetHeaderVal(int i, int val) {
        // Explicitly checks if gShmPtr is NULL before proceeding
        if(gShmPtr == NULL)
        {
                perror("Failure Point: SetHeadrVal; Shared memory segment not present, cannot proceed to set header value..."); // Using perror instead of printf or fprintf since it prints out more info on an error
        }
        else
        {
                // gShmPtr points to the beginning of the shared memory segment, cast it to an integer pointer and then move to the ith integer position
                int* ptr = (int*) gShmPtr + i;
                *ptr = val; // Write the value to the shared memory
        }
}

// Get the value of shared variable "bufSize"
int GetBufSize()
{       
        return GetHeaderVal(0);
}

// Get the value of shared variable "itemCnt"
int GetItemCnt()
{
        return GetHeaderVal(1);
}

// Get the value of shared variable "in"
int GetIn()
{
        return GetHeaderVal(2);
}

// Get the value of shared variable "out"
int GetOut()
{             
        return GetHeaderVal(3);
}


// Write the given val at the given index in the bounded buffer 
void WriteAtBufIndex(int indx, int val)
{
        // Skip the four-integer header and go to the given index 
        void* ptr = gShmPtr + 4*sizeof(int) + indx*sizeof(int);
	memcpy(ptr, &val, sizeof(int));
}

// Read the val at the given index in the bounded buffer
int ReadAtBufIndex(int indx) {
        int value;  // Will hold (int) position
        int* ptr = (int*) gShmPtr + 4 + indx; // Calculate the address to access the indx-th integer after skipping a 4-integer-long header in the shared memory.
        return *ptr; // Read & return the value from the shared memory
}

// Get a random number in the range [x, y]
int GetRand(int x, int y)
{
	int r = rand();
	r = x + r % (y-x+1);
        return r;
}
