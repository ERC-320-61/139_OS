#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <semaphore.h>
#include <stdbool.h> 

#define MAX_SIZE 100000000
#define MAX_THREADS 16
#define RANDOM_SEED 7649
#define MAX_RANDOM_NUMBER 3000
#define NUM_LIMIT 9973

// Global variables
long gRefTime; //For timing
int gData[MAX_SIZE]; //The array that will hold the data

int gThreadCount; //Number of threads
int gDoneThreadCount; //Number of threads that are done at a certain point. Whenever a thread is done, it increments this. Used with the semaphore-based solution
int gThreadProd[MAX_THREADS]; //The modular product for each array division that a single thread is responsible for
bool gThreadDone[MAX_THREADS]; //Is this thread done? Used when the parent is continually checking on child threads

// Semaphores
sem_t completed; //To notify parent that all threads have completed or one of them found a zero
sem_t mutex; //Binary semaphore to protect the shared variable gDoneThreadCount

int SqFindProd(int size); //Sequential FindProduct (no threads) computes the product of all the elements in the array mod NUM_LIMIT
void *ThFindProd(void *param); //Thread FindProduct but without semaphores
void *ThFindProdWithSemaphore(void *param); //Thread FindProduct with semaphores
int ComputeTotalProduct(); // Multiply the division products to compute the total modular product 
void InitSharedVars();
void GenerateInput(int size, int indexForZero); //Generate the input array
void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3]); //Calculate the indices to divide the array into T divisions, one division per thread
int GetRand(int min, int max);//Get a random number between min and max

//Timing functions
long GetMilliSecondTime(struct timeb timeBuf);
long GetCurrentTime(void);
void SetTime(void);
long GetTime(void);

int main(int argc, char *argv[]){

	pthread_t tid[MAX_THREADS];
	pthread_attr_t attr[MAX_THREADS];
	int indices[MAX_THREADS][3];
    int params[MAX_THREADS][3]; // Parameters for each thread
	int i, indexForZero, arraySize, prod;


	// Code for parsing and checking command-line arguments
	if(argc != 4){
		fprintf(stderr, "Invalid number of arguments!\n");
		exit(-1);
	}
	if((arraySize = atoi(argv[1])) <= 0 || arraySize > MAX_SIZE){
		fprintf(stderr, "Invalid Array Size\n");
		exit(-1);
	}
	gThreadCount = atoi(argv[2]);
	if(gThreadCount > MAX_THREADS || gThreadCount <=0){
		fprintf(stderr, "Invalid Thread Count\n");
		exit(-1);
	}
	indexForZero = atoi(argv[3]);
	if(indexForZero < -1 || indexForZero >= arraySize){
		fprintf(stderr, "Invalid index for zero!\n");
		exit(-1);
	}

    GenerateInput(arraySize, indexForZero);

    CalculateIndices(arraySize, gThreadCount, indices);

	// Code for the sequential part
	SetTime();
	prod = SqFindProd(arraySize);
	printf("Sequential multiplication completed in %ld ms. Product = %d\n", GetTime(), prod);

	// Threaded with parent waiting for all child threads
	InitSharedVars();
	SetTime();

/***************START OF MY CODE 2 ***************/
	 // Write your code here
  	 // Initialize threads, create threads, and then let the parent wait for all threads using pthread_join
	 // The thread start function is ThFindProd
	 // Don't forget to properly initialize shared variables

	// Initialize attributes for each thread
    for (i = 0; i < MAX_THREADS; i++) {
        pthread_attr_init(&attr[i]);
    }

	// Creating threads for section "START OF MY CODE 2"
    for (i = 0; i < gThreadCount; i++) {
        pthread_create(&tid[i], &attr[i], ThFindProd, (void*)&params[i]);
    }

	// Waiting for all threads to finish
	for (i = 0; i < gThreadCount; i++) {
		pthread_join(tid[i], NULL);
	}

	 // Destroy thread attributes after use
    for (i = 0; i < MAX_THREADS; i++) {
        pthread_attr_destroy(&attr[i]);
    }
/*************** END OF MY CODE 2 ***************/



    prod = ComputeTotalProduct();
	printf("Threaded multiplication with parent waiting for all children completed in %ld ms. Product = %d\n", GetTime(), prod);

	// Multi-threaded with busy waiting (parent continually checking on child threads without using semaphores)
	InitSharedVars();
	SetTime();

/*************** START OF MY CODE 3 ***************/
	// Write your code here
    // Don't use any semaphores in this part
	// Initialize threads, create threads, and then make the parent continually check on all child threads
	// The thread start function is ThFindProd
	// Don't forget to properly initialize shared variables


	// Initialize shared variables
	volatile bool all_done = false;

	// Busy waiting loop
	bool found_zero = false;
	while (!all_done) {
		all_done = true;  // Assume all threads are done unless found otherwise
		for (i = 0; i < gThreadCount; i++) {
			if (!gThreadDone[i]) {
				all_done = false;  // If any thread is not done, set all_done to false
				if (gThreadProd[i] == 0) {
					found_zero = true;  // If any thread finds zero, note it
					break;
				}
			}
		}
		if (found_zero) break;  // Exit loop if zero is found
	}

	// If zero is found, terminate all threads
	if (found_zero) {
		for (i = 0; i < gThreadCount; i++) {
			pthread_cancel(tid[i]);
		}
	}

	// Join all threads (important for proper cleanup and to avoid memory leaks)
	for (i = 0; i < gThreadCount; i++) {
		pthread_join(tid[i], NULL);
	}
/*************** END OF MY CODE 3 ***************/


    prod = ComputeTotalProduct();
	printf("Threaded multiplication with parent continually checking on children completed in %ld ms. Product = %d\n", GetTime(), prod);


	// Multi-threaded with semaphores

	InitSharedVars();
    // Initialize your semaphores here

	SetTime();

/*************** START OF MY CODE SECTION 4 ***************/
    // Write your code here
	// Initialize threads, create threads, and then make the parent wait on the "completed" semaphore
	// The thread start function is ThFindProdWithSemaphore
	// Don't forget to properly initialize shared variables and semaphores using sem_init


	// Initialize shared variables and semaphores
	sem_init(&completed, 0, 0);  // Semaphore to signal completion
	sem_init(&mutex, 0, 1);      // Binary semaphore for protecting shared variable gDoneThreadCount


	// Wait on the semaphore
	for (i = 0; i < gThreadCount; i++) {
		sem_wait(&completed);
	}

	// Check if any thread has found a zero, if so, cancel remaining threads
	bool zero_found = false;
	for (i = 0; i < gThreadCount; i++) {
		if (gThreadProd[i] == 0) {
			zero_found = true;
			break;
		}
	}

	if (zero_found) {
		for (i = 0; i < gThreadCount; i++) {
			pthread_cancel(tid[i]);
		}
	}

	// Join all threads
	for (i = 0; i < gThreadCount; i++) {
		pthread_join(tid[i], NULL);
	}

	// Destroy semaphores
	sem_destroy(&completed);
	sem_destroy(&mutex);

/*************** END OF MY CODE SECTION 4 ***************/



    prod = ComputeTotalProduct();
	printf("Threaded multiplication with parent waiting on a semaphore completed in %ld ms. Min = %d\n", GetTime(), prod);
}

/*************** START OF MY CODE SECTION 5 ***************/
// Write a regular sequential function to multiply all the elements in gData mod NUM_LIMIT
// REMEMBER TO MOD BY NUM_LIMIT AFTER EACH MULTIPLICATION TO PREVENT YOUR PRODUCT VARIABLE FROM OVERFLOWING
int SqFindProd(int size) {
    int product = 1;  // Initialize the product to 1

    for (int i = 0; i < size; i++) {
        if (gData[i] == 0) {
            return 0;  // If the element is zero, return 0 immediately
        }
        product = (product * gData[i]) % NUM_LIMIT;  // Update the product with the next element modulo NUM_LIMIT
    }

    return product;  // Return the final product
}
/*************** END OF MY CODE SECTION 5 ***************/

// Write a thread function that computes the product of all the elements in one division of the array mod NUM_LIMIT
// REMEMBER TO MOD BY NUM_LIMIT AFTER EACH MULTIPLICATION TO PREVENT YOUR PRODUCT VARIABLE FROM OVERFLOWING
// When it is done, this function should store the product in gThreadProd[threadNum] and set gThreadDone[threadNum] to true
void* ThFindProd(void *param) {
	int threadNum = ((int*)param)[0];

}

/*************** START OF MY CODE SECTION 6 ***************/
// Write a thread function that computes the product of all the elements in one division of the array mod NUM_LIMIT
// REMEMBER TO MOD BY NUM_LIMIT AFTER EACH MULTIPLICATION TO PREVENT YOUR PRODUCT VARIABLE FROM OVERFLOWING
// When it is done, this function should store the product in gThreadProd[threadNum]
// If the product value in this division is zero, this function should post the "completed" semaphore
// If the product value in this division is not zero, this function should increment gDoneThreadCount and
// post the "completed" semaphore if it is the last thread to be done
// Don't forget to protect access to gDoneThreadCount with the "mutex" semaphore
void* ThFindProdWithSemaphore(void *param) {
    int *parameters = (int*)param;
    int threadNum = parameters[0];
    int start = parameters[1];
    int end = parameters[2];
    int localProd = 1;

    // Calculate the product of elements in the designated section of the array
    for (int i = start; i <= end; i++) {
        if (gData[i] == 0) {
            localProd = 0;
            break;  // If zero is found, no need to continue multiplying
        }
        localProd = (localProd * gData[i]) % NUM_LIMIT;
    }

    // Store the result in the global array for thread products
    gThreadProd[threadNum] = localProd;

    if (localProd == 0) {
        // If product is zero, signal completion immediately
        sem_post(&completed);
    } else {
        // Protect access to gDoneThreadCount
        sem_wait(&mutex);
        gDoneThreadCount++;
        if (gDoneThreadCount == gThreadCount) {
            // If this is the last thread to finish, signal completion
            sem_post(&completed);
        }
        sem_post(&mutex);
    }

    return NULL;  // Thread completes execution
}
/*************** END OF MY CODE SECTION 6 ***************/

int ComputeTotalProduct() {
    int i, prod = 1;

	for(i=0; i<gThreadCount; i++)
	{
		prod *= gThreadProd[i];
		prod %= NUM_LIMIT;
	}

	return prod;
}

void InitSharedVars() {
	int i;

	for(i=0; i<gThreadCount; i++){
		gThreadDone[i] = false;
		gThreadProd[i] = 1;
	}
	gDoneThreadCount = 0;
}

/*************** END OF MY CODE SECTION 6 ***************/
// Write a function that fills the gData array with random numbers between 1 and MAX_RANDOM_NUMBER
// If indexForZero is valid and non-negative, set the value at that index to zero
void GenerateInput(int size, int indexForZero) {
    srand(RANDOM_SEED);  // Initialize random number generator
    for (int i = 0; i < size; i++) {
        gData[i] = GetRand(1, MAX_RANDOM_NUMBER);  // Generate random number between 1 and MAX_RANDOM_NUMBER
    }
    if (indexForZero >= 0 && indexForZero < size) {
        gData[indexForZero] = 0;  // Set specified index to zero if within bounds
    }
}
/*************** END OF MY CODE SECTION 6 ***************/


/*************** START OF MY CODE SECTION 8 ***************/
// Write a function that calculates the right indices to divide the array into thrdCnt equal divisions
// For each division i, indices[i][0] should be set to the division number i,
// indices[i][1] should be set to the start index, and indices[i][2] should be set to the end index
void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3]) {
    int divisionSize = arraySize / thrdCnt;  // Calculate the size of each division
    int remainder = arraySize % thrdCnt;    // Calculate the remainder to distribute extra elements
    int start = 0;

    for (int i = 0; i < thrdCnt; i++) {
        indices[i][0] = i;        // Set the division number
        indices[i][1] = start;    // Set the start index for this thread

        // Calculate the end index for this thread
        // If there's a remainder, distribute it among the first few threads
        if (remainder > 0) {
            indices[i][2] = start + divisionSize; // Include an extra element for this thread
            remainder--;  // Decrease remainder after distributing one extra element
        } else {
            indices[i][2] = start + divisionSize - 1;
        }

        start = indices[i][2] + 1;  // Update start index for the next thread
    }
}
/*************** END OF MY CODE SECTION 8 ***************/

// Get a random number in the range [x, y]
int GetRand(int x, int y) {
    int r = rand();
    r = x + r % (y-x+1);
    return r;
}

long GetMilliSecondTime(struct timeb timeBuf){
	long mliScndTime;
	mliScndTime = timeBuf.time;
	mliScndTime *= 1000;
	mliScndTime += timeBuf.millitm;
	return mliScndTime;
}

long GetCurrentTime(void){
	long crntTime=0;
	struct timeb timeBuf;
	ftime(&timeBuf);
	crntTime = GetMilliSecondTime(timeBuf);
	return crntTime;
}

void SetTime(void){
	gRefTime = GetCurrentTime();
}

long GetTime(void){
	long crntTime = GetCurrentTime();
	return (crntTime - gRefTime);
}

