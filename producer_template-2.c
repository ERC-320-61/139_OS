/*
CSC139 
Spring 2024
First Assignment
Delgado, Eric
Section #03
OSs Tested on: such as Linux, Mac, etc.
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

        if(argc != 4){
		printf("Invalid number of command-line arguments\n");
		exit(1);
        }

        
	bufSize = atoi(argv[1]);
	itemCnt = atoi(argv[2]);
	randSeed = atoi(argv[3]);
	
	// Write code to check the validity of the command-line arguments

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
		execlp("./consumer","consumer",NULL);
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

     // Shared memory file descriptor
    int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("shm_open");
        exit(1);
    }

    // Configure the size of the shared memory object
    if (ftruncate(fd, SHM_SIZE) != 0) {
        perror("ftruncate");
        exit(1);
    }

    // Map the shared memory object
    gShmPtr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (gShmPtr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

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

    // Produce itemCnt items
    for (int i = 0; i < itemCnt; i++) {
        // Generate a random item
        int val = GetRand(0, 3000);

        // Wait if buffer is full
        while (((in + 1) % bufSize) == GetOut()) {
            // Buffer is full, wait for consumer to consume an item
            usleep(100000); // Sleep for 100ms
        }

        // Write the item to the buffer
        WriteAtBufIndex(in, val);

        // Print production message
        printf("Producing Item %d with value %d at Index %d\n", i, val, in);

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
    int *ptr = (int*) gShmPtr + i;
    *ptr = val;
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
    int val;
    // Skip the four-integer header and go to the given index 
    int* ptr = (int*) gShmPtr + 4 + indx;
    val = *ptr;
    return val;
}

// Get a random number in the range [x, y]
int GetRand(int x, int y)
{
	int r = rand();
	r = x + r % (y-x+1);
        return r;
}
