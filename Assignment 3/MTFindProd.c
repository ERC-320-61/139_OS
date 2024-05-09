/*
CSC139
Section #03
Spring 2024
Third Assignment: Multithreading and Synchronization
Delgado, Eric
OSs Tested on: Linux Only
Dedicated VM CPU: 13th Gen Intel(R) Core(TM) i9-13900KF CPU at 3.00GHz, 8 physical cores and 16 logical threads
ECS Systems CPU: Intel(R) Xeon(R) Gold 6254 CPU at 3.10GHz, 4 logical processors, VM
*/

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
	if (argc != 4) {
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


	
	/*************** START OF THREAD INITIALIZATION AND SYNCHRONIZATION ***************/

	/*
		Thread Initialization and Synchronization:
			* This block initializes and synchronizes threads according to specified parameters.
			* Each thread is set up with distinct attributes and assigned a unique segment of the data array to process.
			* Threads are created to execute the ThFindProd function, each receiving a pointer to its specific parameters.
			* After creation, the parent process waits for all threads to complete, ensuring all data segments are processed.
			* This approach ensures that shared variables are properly managed across multiple threads.
			* Thread attributes are safely destroyed after all threads complete, to clean up allocated resources.
	*/
	printf("\nSTART OF PARENT WAITING AND THREAD MANAGEMENT: \n");

	// Initialize and create threads
    for (i = 0; i < gThreadCount; i++) {
        pthread_attr_init(&attr[i]);  											// Initialize thread attributes for each thread
        params[i][0] = i;  														// Set thread number
        params[i][1] = indices[i][1];  											// Set start index for each thread
        params[i][2] = indices[i][2];  											// Set end index for each thread
        pthread_create(&tid[i], &attr[i], ThFindProd, (void*)&params[i]);		// Create each thread to process its part of the array
    }

	for (i = 0; i < gThreadCount; i++) {										// Wait for each thread to finish execution
		pthread_join(tid[i], NULL);												
	}

	for (i = 0; i < gThreadCount; i++) {										// Safely destroy thread attributes after each use
    	pthread_attr_destroy(&attr[i]);											
	}

	/*************** END OF THREAD INITIALIZATION AND SYNCHRONIZATION ***************/


	prod = ComputeTotalProduct();
	printf("Threaded multiplication with parent waiting for all children completed in %ld ms. Product = %d\n", GetTime(), prod);


	// Multi-threaded with busy waiting (parent continually checking on child threads without using semaphores)
	InitSharedVars();
	SetTime();

	
	/*************** START OF BUSY WAITING AND THREAD MANAGEMENT ***************/
	/*
		Busy Waiting and Thread Management:
		* Initialize and manage threads without using semaphores for synchronization.
		* Create threads to execute the ThFindProd function, ensuring each has the required parameters.
		* Continuously monitor all threads to check their completion status and detect any zeros in their output.
		* If a zero is detected, exit the monitoring loop immediately to halt further processing.
		* Cancel and join all threads ensuring no resources are left hanging, providing a clean exit and memory management.
	*/
	printf("\nSTART OF BUSY WAITING AND THREAD MANAGEMENT: \n");

	volatile bool all_done = false;                // Flag to check if all threads have completed
	bool found_zero = false;                       // Flag to indicate if zero product is found

	// Start all threads
	for (i = 0; i < gThreadCount; i++) {
		if (pthread_create(&tid[i], NULL, ThFindProd, &params[i])) {
			fprintf(stderr, "Error creating thread %d\n", i);
			exit(1);
		}
		// FOR DEBUGGING printf("Thread %d started\n", i);
	}

	// Busy waiting loop to monitor threads
	do {
		all_done = true; 														// Assume all threads are completed unless found otherwise
		for (i = 0; i < gThreadCount; i++) {
			if (!gThreadDone[i]) {
				all_done = false;  												// One or more threads are still processing
																				// FOR DEBUGGING printf("Checking thread %d: still active\n", i);
				if (gThreadProd[i] == 0) {
					found_zero = true;  // Zero product found
																				// FOR DEBUGGING printf("Zero product detected by thread %d\n", i);
					break;  													// Break the inner loop if zero is found
				}
			}
		}

		if (found_zero) break;													// If zero found, exit from the monitoring loop
	} while (!all_done);

	
	if (found_zero || all_done) {
		for (i = 0; i < gThreadCount; i++) {
			pthread_cancel(tid[i]);  											// Cancel and join all threads if zero is detected or all are done
																				// FOR DEBUGGING printf("Cancelling thread %d\n", i);
		}
		for (i = 0; i < gThreadCount; i++) {
			pthread_join(tid[i], NULL);  // Wait for each thread to terminate
																				// FOR DEBUGGING printf("Thread %d joined\n", i);
		}
	}

																				// Calculate the total product if no zero was found
	if (!found_zero) {
		int totalProduct = ComputeTotalProduct();
																				// FOR DEBUGGING printf("Total product computed: %d\n", totalProduct);
	}

	printf("Threaded multiplication with busy waiting completed in %ld ms. Product = %d\n", GetTime(), prod);
/*************** END OF BUSY WAITING AND THREAD MANAGEMENT ***************/




    prod = ComputeTotalProduct();

	InitSharedVars();
    // Initialize my SEMAPHORES BELOW

	SetTime();

/*************** START OF SEMAPHORE SYNCHRONIZATION AND THREAD MANAGEMENT ***************/
/*
    Semaphore Synchronization and Thread Management:
        * Initialize semaphores and shared variables critical for managing multi-threaded operations.
        * Employ the 'completed' semaphore to synchronize thread completion, ensuring all threads signal when they finish.
        * Monitor threads that calculate a zero product to enable early termination of all threads.
        * If a zero is detected, cancel all other threads to halt further processing and save resources.
        * Join all threads post-cancellation or completion to reclaim all system resources.
        * Finally, clean up the semaphore resources to prevent leaks.
*/

printf("\nSTART OF SEMAPHORE SYNCHRONIZATION AND THREAD MANAGEMENT: \n");

InitSharedVars();  															// Initialize shared variables
SetTime();         															// Start timing before threads start

// Initialize semaphores
sem_init(&completed, 0, 0);
sem_init(&mutex, 0, 1);

// Start threads and print their range information
for (int i = 0; i < gThreadCount; i++) {
    printf("Thread %d started with start: %d and end: %d\n", i, params[i][1], params[i][2]);
    pthread_create(&tid[i], NULL, ThFindProdWithSemaphore, &params[i]);
}

int completed_threads = 0;
bool zero_found = false;

while (completed_threads < gThreadCount && !zero_found) {					// Wait for any thread to signal completion
    sem_wait(&completed);

    for (int i = 0; i < gThreadCount; i++) {
        if (gThreadProd[i] == 0) {											// Check if any thread found a zero product
            zero_found = true;
            break;
        }
    }

    if (zero_found) {
        for (int i = 0; i < gThreadCount; i++) {
            pthread_cancel(tid[i]);											// If zero found, cancel all threads
        }
    }

    completed_threads++;
}


for (int i = 0; i < gThreadCount; i++) {
    pthread_join(tid[i], NULL);												// Ensure all threads are joined
}

sem_destroy(&completed);													// Clean up semaphore resources
sem_destroy(&mutex);

prod = ComputeTotalProduct();												// Calculate and display the final product

printf("Threaded multiplication with parent waiting on a semaphore completed in %ld ms. Product = %d\n", GetTime(), prod);

/*************** END OF SEMAPHORE SYNCHRONIZATION AND THREAD MANAGEMENT ***************/


	// Exit the program
    exit(0);
}





/*************** START OF SEQUENTIAL FIND PRODUCT ***************/
/*
	Sequential Multiplication:
		* Computes the product of all elements in the global array gData, modulo NUM_LIMIT to prevent overflow
		* Early termination occurs if any element in the array is zero, returning zero immediately
		* This method ensures efficient handling of cases where the product is inherently zero due to the presence of any zero in the array
		* Utilizes a simple, linear iteration over the array to multiply elements, making it a straightforward, sequential approach
		* Acts as a baseline for performance comparison against threaded implementations, or used when threading is inapplicable
*/
int SqFindProd(int size) {
    int product = 1;  

    for (int i = 0; i < size; i++) {
        if (gData[i] == 0) {
            return 0;  									// If the element is zero, return 0 immediately
        }
        product = (product * gData[i]) % NUM_LIMIT;  	// Update the product with the next element modulo NUM_LIMIT
    }

    return product;  									// Return the final product
}




/*************** START OF THREAD FIND PRODUCT ***************/
/*
	Thread Product Computation:
		* This function is executed by each thread to compute the product of all elements in one division of the gData array, using modulo NUM_LIMIT to manage overflow
		* It operates on a slice of the array determined by start and end indices, calculating the product in such a way that numerical limits are not exceeded
		* The product result is stored in the gThreadProd array at the index corresponding to the thread's number
		* Upon completion, the thread updates the gThreadDone status to true, signaling that it has finished processing its segment of the array
		* For visibility of thread activity, the function logs its operation commencement, aiding in debugging and monitoring of thread execution
*/
void *ThFindProd(void *param) {
    int *indices = (int*) param;
    int threadNum = indices[0];
    int start = indices[1];
    int end = indices[2];

    printf("Thread %d started with start: %d and end: %d\n", threadNum, start, end);

    int localProd = 1;  																			// Assuming computation here
    for (int i = start; i <= end; i++) {
        localProd = (localProd * gData[i]) % NUM_LIMIT;
    }
    gThreadProd[threadNum] = localProd;
    gThreadDone[threadNum] = true;  																// Set this thread as done
    // FOR DEBUGGING printf("Thread %d finished with product %d\n", threadNum, localProd);

    return NULL;
}


/*************** START THE THREAD FIND PRODUCT WITH SEMAPHORE ***************/
/*
	Semaphore-Based Thread Product Computation:
    	* Each thread executes this function to compute the product of its assigned division of the gData array, applying the modulo NUM_LIMIT after each multiplication to prevent overflow
    	* The computed product is stored in the gThreadProd array at the index corresponding to the thread's number
    	* If a zero is detected in the product, the "completed" semaphore is immediately signaled to notify other processes that the final product is zero, stopping further computations
    	* For non-zero products, the thread increments the shared gDoneThreadCount variable. Access to this shared variable is protected by the "mutex" semaphore to ensure thread-safe operations
    	* If this thread is the last to finish its computation, it signals the "completed" semaphore to indicate that all threads have completed their tasks
    	* This approach ensures synchronization among threads using semaphores to handle concurrent access and modifications to shared variables effectively
*/
void* ThFindProdWithSemaphore(void *param) {
    int *parameters = (int*)param;
    int threadNum = parameters[0];
    int start = parameters[1];
    int end = parameters[2];
    int localProd = 1;

    for (int i = start; i <= end; i++) {							// For each element in the interval from 'start' to 'end', perform calculation
        if (gData[i] == 0) {
            localProd = 0;
            break;  												// If zero is found, no need to continue multiplying
        }
        localProd = (localProd * gData[i]) % NUM_LIMIT;
    }

    gThreadProd[threadNum] = localProd;								// Store the result in the global array for thread products

    if (localProd == 0) {											// If product is zero, signal completion immediately
        sem_post(&completed);
    } else {														// Else protect access to gDoneThreadCount
        sem_wait(&mutex);
        gDoneThreadCount++;
        if (gDoneThreadCount == gThreadCount) {						// If this is the last thread to finish, signal completion
            sem_post(&completed);
        }
        sem_post(&mutex);
    }

    return NULL;  													// Thread completes execution
}
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

/*************** START OF GENERATE INPUT ***************/
/*
	Input Array Initialization:
		* Populates the global array 'gData' with random numbers ranging from 1 to MAX_RANDOM_NUMBER
		     -This ensures variability in the data set used for multiplication
		* Initializes the random number generator with the RANDOM_SEED for reproducible results across different runs
		* If 'indexForZero' is a valid non-negative index within the array bounds, sets that position in the array to zero
		     -This allows testing scenarios where the product of the array elements should logically result in zero
*/
void GenerateInput(int size, int indexForZero) {
    srand(RANDOM_SEED);  
    for (int i = 0; i < size; i++) {
        gData[i] = GetRand(1, MAX_RANDOM_NUMBER);  
    }
    if (indexForZero >= 0 && indexForZero < size) {
        gData[indexForZero] = 0;  
    }
}



/*************** START OF CALCULATE INDICES ***************/
/*
	Array Division Calculation:
		* This function determines the indices to divide the global array into equal segments for multithreading
		* It calculates the division size based on the total size of the array and the number of threads (thrdCnt)
		* Each thread is assigned a start and end index ensuring that the divisions cover the entire array and distribute any remainder
		* The division information is stored in a 2D array where each thread has a designated start index, end index, and division number
*/
void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3]) {
    int divisionSize = arraySize / thrdCnt;
    int remainder = arraySize % thrdCnt;
    int currentStart = 0;

    for (int i = 0; i < thrdCnt; i++) {
        indices[i][0] = i;  										// Set division number
        indices[i][1] = currentStart;  								// Set start index
        int extra = (remainder > 0) ? 1 : 0;  						// Distribute any remaining elements
        indices[i][2] = currentStart + divisionSize + extra - 1;  	// Set end index

        currentStart = indices[i][2] + 1;  							// Update start index for next division
        remainder -= extra;  										// Decrement remainder
    }
}

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

