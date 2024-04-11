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
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>   // for close, ftruncate, usleep

// Size of shared memory block
// Pass this to ftruncate and mmap
#define SHM_SIZE 4096

// Global pointer to the shared memory block
// This should receive the return value of mmap
// Don't change this pointer in any function
void* gShmPtr;

// You won't necessarily need all the functions below
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

int main()
{
    const char *name = "OS_HW1_EricDelgado"; // Name of shared memory block to be passed to shm_open
    int bufSize; // Bounded buffer size
    int itemCnt; // Number of items to be consumed
    int in; // Index of next item to produce
    int out; // Index of next item to consume

        
        // Write code here to create a shared memory block and map it to gShmPtr
        // Use the above name
        // **Extremely Important: map the shared memory block for both reading and writing 
        // Use PROT_READ | PROT_WRITE
        int fd = shm_open(name, O_RDWR | O_CREAT, 0666);
        if (fd == -1) {
                perror("Failure Point:shm_open; Error creating shared memory...");     // Using perror instead of printf or fprintf since it prints out more info on an error    
                exit(1);
        }

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

        // Write code here to read the four integers from the header of the shared memory block 
        // These are: bufSize, itemCnt, in, out
        // Just call the functions provided below like this:
        bufSize = GetBufSize(); 
        itemCnt = GetItemCnt(); 
        in = GetIn();           
        out = GetOut();         

        // Check that the consumer has read the right values
        printf("Consumer reading: bufSize = %d, itemCnt = %d, in = %d, out = %d\n", bufSize, itemCnt, in, out);

        // Write code here to consume all the items produced by the producer
        // Code to consume all the items produced by the producer
        int consumedItems = 0;
        while (consumedItems < itemCnt) {
                // Busy-wait if buffer is empty
                while (GetIn() == out) {
                        usleep(1000); // Sleep for 1ms to reduce CPU usage
                        
                        // Re-check itemCnt in case of any updates indicating all items are produced.
                        itemCnt = GetItemCnt();
                        if (consumedItems >= itemCnt) {
                                // All items have been consumed; exit the loop.
                                break;
                        }
                }

                // Make sure we break out of the outer loop if all items are already consumed.
                if (consumedItems >= itemCnt) {
                        break;
                }

                // Read the item from the buffer at index 'out'
                int value = ReadAtBufIndex(out);

                // Report the consumption of an item
                printf("Consuming Item %d with value %d at Index %d\n", consumedItems, value, out);

                // Increment the 'out' index and wrap it if necessary
                out = (out + 1) % bufSize;

                // Increment counter
                consumedItems++;

                // Update the shared 'out' index for the producer to see
                SetOut(out);
        }

     // remove the shared memory segment 
     if (shm_unlink(name) == -1) {
	printf("Error removing %s\n",name);
	exit(-1);
     }

     return 0;
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

