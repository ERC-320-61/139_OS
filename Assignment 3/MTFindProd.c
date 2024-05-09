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
        * Initializes semaphores and shared variables that are critical for managing operations across multiple threads
        * Uses the 'completed' semaphore to synchronize thread completion, ensuring each thread signals when it has finished processing. This helps in maintaining an accurate count of active and completed threads
        * Actively monitors the computation results from each thread for a zero product, enabling early termination of the entire thread group
			- This is intended to prevent unnecessary computation once a determinative result (zero product) is found, thus saving system resources
        * If a zero product is detected, all other threads are promptly cancelled to halt further computations, emphasizing efficiency and resource conservation
        * Ensures that all threads, whether cancelled or naturally completed, are joined back to the main thread, guaranteeing that no thread resources are left hanging, which could lead to memory leaks or dangling processes
        * Cleans up semaphore resources after use to prevent resource leaks, ensuring that the system remains efficient and that semaphore limits are not breached
*/


printf("\nSTART OF SEMAPHORE SYNCHRONIZATION AND THREAD MANAGEMENT: \n");

InitSharedVars();
SetTime();
sem_init(&completed, 0, 0);
sem_init(&mutex, 0, 1);

int active_threads = 0;
for (int i = 0; i < gThreadCount; i++) {																							// Loop over the number of configured threads to be started
    if (pthread_create(&tid[i], NULL, ThFindProdWithSemaphore, &params[i]) == 0) {													// Attempt to create each thread; pass it the function and parameters it should use
        printf("Thread %d started with start: %d and end: %d\n", i, params[i][1], params[i][2]);									// On successful thread start, log and increment the count of actively started threads
        active_threads++;
    } else {
        printf("Failed to start thread %d\n", i);																					// If thread creation fails, log the failure
    }
}

int completed_threads = 0; 																											// Counter to track the number of threads that have completed their task
bool zero_found = false;   																											// Flag to indicate if a zero product has been detected

while (completed_threads < active_threads && !zero_found) {																			// Continue to loop until all active threads have completed or a zero product is found
    sem_wait(&completed);                																							// Block until a thread signals that it has completed its task
    completed_threads++;                 																							// Increment the count of completed threads

    																																// Check the products calculated by each thread
    for (int i = 0; i < gThreadCount; i++) {
        if (gThreadProd[i] == 0) {       																							// If a thread reports a zero product
            zero_found = true;           																							// Set the flag to indicate a zero product was found
            // FOR DEBUGGING printf("Zero product detected by thread %d.\n", i); 													// Log which thread found the zero
            break;                       																							// Exit the loop since we no longer need to check further
        }
    }


    if (zero_found) {																												// If a zero product was detected during the checks
        // FOR DEBUGGING printf("Cancelling all threads due to zero detection.\n"); 												// Log that all threads will be cancelled	
        for (int i = 0; i < gThreadCount; i++) {																					// Cancel all threads to stop any further computation
            pthread_cancel(tid[i]);      																							// Send cancellation request to each thread
        }
    }
}

for (int i = 0; i < gThreadCount; i++) {																							// Ensure all threads are properly joined back to the main process
    pthread_join(tid[i], NULL);          																							// Wait for each thread to finish cleanup
    // Debugging print to confirm each thread has been joined
    // printf("Thread %d has been joined.\n", i);
}


sem_destroy(&completed);
sem_destroy(&mutex);

prod = ComputeTotalProduct();
printf("Multi-threaded multiplication with semaphores completed in %ld ms. Product: %d\n", GetTime(), prod);


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
		* Each thread is tasked with computing the product of a specific segment of the gData array, modulo NUM_LIMIT to manage overflow
		* The computed product is stored in the gThreadProd array at an index corresponding to the thread's assigned number
		* If a zero is encountered during computation, the thread stores '0' as the product and ceases further multiplication
		* Upon completing its computation task, regardless of the result (zero or non-zero), the thread increments the shared gDoneThreadCount variable
		This increment operation is protected by a 'mutex' semaphore to ensure thread-safe modifications
		* After updating the shared count, each thread signals the 'completed' semaphore to indicate it has finished its task
		* This signaling aids in synchronization by notifying a managing process or thread that a thread's computation has ended, which is critical for handling early termination or gathering final results
		* This approach guarantees that all threads will properly synchronize their completion, ensuring robust and safe concurrency management
*/

void* ThFindProdWithSemaphore(void *param) {
    int *parameters = (int*)param;
    int threadNum = parameters[0];
    int start = parameters[1];
    int end = parameters[2];
    int localProd = 1;

 																					//FOR DEBUGGING printf("Thread %d started computation with start: %d and end: %d\n", threadNum, start, end);

    for (int i = start; i <= end; i++) { 											// Loop through the assigned segment
        if (gData[i] == 0) {
            localProd = 0;
            break; 																	// Stop computation if zero is found
        }
        localProd = (localProd * gData[i]) % NUM_LIMIT;
    }

    gThreadProd[threadNum] = localProd; 											// Store result in global array
    																				// FOR DEBUGGING printf("Thread %d completed with product: %d\n", threadNum, localProd);
	/* FOR DEBUGGING 
    if (localProd == 0) {
        printf("Thread %d found zero product, signaling completion.\n", threadNum);
    }
	*/

    sem_wait(&mutex);
    gDoneThreadCount++;
    sem_post(&mutex);

    
    sem_post(&completed);															// Post the semaphore to signal this thread's completion
    																				// FOR DEBUGGING printf("Thread %d posted completion.\n", threadNum);

    return NULL;
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

