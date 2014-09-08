

//	Solution for INB365 Assignment 1
// 
//	Authors:
//	Chris Rhodan n6862624
//	Dustin Wilson n6325157
//
//	Due Date:
// 	9th September 2014

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Define Globals being used
#define MAXIMUM_AIRPORT_CAPACITY 10
#define AIRPLANE_CODE 6
#define TRUE 1
#define FALSE 0 

// Struct to represent a single Airplane
struct Airplane{
	char *code;
	double parkTime;
};

// Array of Pointers to Airplanes to represent the Airport - shared resource
struct Airplane *airport[MAXIMUM_AIRPORT_CAPACITY];

// Variables to contain the odds of a plane landing and departing
int arrivalOdds = 0;
int departureOdds = 0;

// Global variable for termination 
int keep_running = TRUE;

// Semaphores and Mutex
pthread_mutex_t airport_mutex;
sem_t empty;
sem_t full;
sem_t runway;

// Author: Chris Rhodan
// Purpose: Function to allocate memory for each Airplane in the airport
// Pre: true
// Post: airport array memory allocated
void InitialiseAirport(){
	int i = 0;
	for(i = 0; i < MAXIMUM_AIRPORT_CAPACITY; i++){
		airport[i] = malloc(sizeof(struct Airplane));
		airport[i] = NULL;
	}
}

// Author: Chris Rhodan
// Purpose: Function that calculates how long a plane has been parked
// Parameter dock: dock number for airplane
// Pre: true
// Post: total time in landing bay: (time_in_mill - currentPlane->parkTime)/1000
double GetStayTime(int dock){
	double timeTaken = 0;
	struct Airplane* currentPlane = airport[dock];

	struct timeval tv;
	gettimeofday(&tv, NULL);
	double time_in_mill = ((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000); 


	return (time_in_mill - currentPlane->parkTime)/1000;
}


// Author: Chris Rhodan
// Purpose: Function sets the global for exit the program to zero
// Pre: if(inputLetter == 'q' || inputLetter == 'Q')
// Post: keep_running = 0;
void ExitProgram(){
	keep_running = FALSE;
}

// Author: Chris Rhodan
// Purpose: Function that prints the airports current status to the console
// Pre: (inputLetter == 'p' || inputLetter == 'P') or program is exitting 
// Post: prints airport status to console
void PrintCurrentState(){
	printf("Airport State:\n");
	int i = 0;
	for (i = 0; i < MAXIMUM_AIRPORT_CAPACITY; i++){
		printf("%d: ", i);
		if(airport[i] == NULL){
			printf("Empty\n");
		}
		else{
			printf("%s (has parked for %6.3f seconds)\n", airport[i]->code, GetStayTime(i));
		}
	}
	
}

// Author: Chris Rhodan
// Purpose: Thread for the user controls monitor
// Pre: true
// Post: keep_running = 0, pthread_join(userControls,NULL);
void *UserControls(){
	
	while(keep_running){

		char inputLetter = NULL;
		scanf(" %c", &inputLetter);

		if(inputLetter == 'q' || inputLetter == 'Q'){
			printf("Exiting Program, Joining Threads\n");
			ExitProgram();
		}
		else if (inputLetter == 'p' || inputLetter == 'P'){
			PrintCurrentState();
		}
		else{
			printf("Error: Not a valid Input\n");
		}
	}
}

// Author: Dustin Wilson
// Purpose: Function to creates an random 6 character code for the generated airplane 2 capital 
// 			letters and 4 numerical values
// Pre: true
// Post: returns a 6 alphanumerical airplane code
char *CreateAirplaneCode(){

	int ranNum = 0;
	int counter = 0;// Variable to keep track of code index
	char *code;
	code = malloc(AIRPLANE_CODE*sizeof(char));

	//ASCII for 'A' is 65, '0' is 48
	while (counter < AIRPLANE_CODE){

		// random generate 2 Alpha chars A-Z
		if(counter < 2) {

			ranNum = 26 * (rand() / (RAND_MAX + 1.0));
			ranNum = ranNum + 65;
			code[counter] = (char)ranNum;

		} else {// random generate 4 numerical chars 0-9

			ranNum = 10 * (rand() / (RAND_MAX + 1.0));
			ranNum = ranNum + 48;
			code[counter] = (char)ranNum;

		}
		counter++;
	}
	return code;
}

// Author: Dustin Wilson
// Purpose: Function that determines if a new plane is to be generated
// Pre: true
// Post: if(ranNum < arrivalOdds) isGenerated true, else false.
int IsPlaneGenerated(){
	int ranNum = 0;
	int isGenerated = FALSE;

	// Calculate a value between 0 and 100
	ranNum = 100 * (rand() / (RAND_MAX + 1.0));

	// If value is in arrivalOdds range 0 - arrivalOdds (it has that chance to be in that range)
	if(ranNum < arrivalOdds) {

		isGenerated = TRUE;
	}

	return isGenerated;
}

// Author: Dustin Wilson
// Purpose: Function that randomly Assigns a empty landing bay for the landed airplane
// Pre: true
// Post: returns a randomly generated landing bay option
int AssignLandingBay(){

	int ranNum = 0;
	int numberFree = 0;

	//get the number of free bays left
	sem_getvalue(&empty, &numberFree);

	int freeSpots[numberFree];
	
	int i; // Index for airport
	int counter = 0; // Index for freeSpots
	
	// Go through the airport array looking for empty bays
	for(i = 0; i < MAXIMUM_AIRPORT_CAPACITY; i++){

		if(airport[i] == NULL){

			freeSpots[counter] = i;
			counter++;
		}
	}

	// Generate a random index in freespots
	ranNum = numberFree * (rand() / (RAND_MAX + 1.0));

	// Return the value at the random index value
	return freeSpots[ranNum];

}

// Author: Dustin Wilson
// Purpose: Function that generates an airplane, lands it, then assigns it a bay
// Pre: if(IsPlaneGenerated())
// Post: airplane is added to airport landing bay
void generate_airplane(){

	int landingBay;

	// Create a new plane	
	struct Airplane* newPlane = malloc(sizeof(struct Airplane));
	newPlane->code = CreateAirplaneCode();
	// Plance Landing 
	printf("Plane %s is landing\n", newPlane->code);
	// Sleep for 2 seconds? or just assign current time? Time for landing
	sleep(2);

	//Acquire mutex lock to protect airport
	pthread_mutex_lock(&airport_mutex);

	// Need to randomly generate a empty landing bay number
	landingBay = AssignLandingBay();

	// if landed printf("DEBUG: Plane %s parked in landing bay %d", assignedBay);
	printf("Plane %s parked in landing bay %d\n",newPlane->code, landingBay);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	newPlane->parkTime = ((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000); 
	airport[landingBay] = newPlane;
	
	// Release mutex lock and Full Semaphore
	pthread_mutex_unlock(&airport_mutex);

}

// Author: Dustin Wilson
// Purpose: The producer thread, thread to generate airplanes arriving at the airport
// Pre: true
// Post: keep_running = 0, pthread_join(AirportArrival,NULL);
void *AirportArrival(){

	// Keeps track of airport capacity
	int currentAirportCapacity = 0;

	while(keep_running){

		// Get the current count for the full semaphore to indicate airport capacity
		sem_getvalue(&full, &currentAirportCapacity);

		// Is a new plane to be generated
		if(IsPlaneGenerated()){
			
			// Report to console if Airport is full
			if (currentAirportCapacity == MAXIMUM_AIRPORT_CAPACITY){
				printf("The airport is full");
			}
			
			// Acquire Empty and Runway Semaphore
			sem_wait(&empty); // This should block if 0 or full
			sem_wait(&runway);
			
			generate_airplane();

			// Release Full and Runway Semaphore
			sem_post(&runway);			
			sem_post(&full);

		}
		usleep(500000);
	}
}

// Author: Chris Rhodan
// Purpose: Function to determine whether a plane should be departing or not
// Pre: true 
// Post: if(ranNum < departureOdds) true, else false
int IsPlaneDeparting(){
	int ranNum	= 0;

	// Calculate a number between 0 and 100
	ranNum = 100 * (rand()/(RAND_MAX + 1.0));

	// If value is in departureOdds range 0 - departureOdds, then a plan should depart
	if(ranNum < departureOdds){
		return 1;
	}
	return 0;	
}

// Author: Chris Rhodan
// Purpose: Function to calculate the terminal/dock from which the plane will depart
// Pre: true
// Post: returns a randomly selected occupied dock for takeoff
int CalculateDepartureDock(){
	
	int j = 0;
	int ranNum = 0;
	int numberFree = 0;
	sem_getvalue(&full, &numberFree);

	int terminalsUsed[numberFree];

	int i = 0;
	for(i = 0; i < MAXIMUM_AIRPORT_CAPACITY; i++){
		if (airport[i] != NULL){
			terminalsUsed[j] = i;
			j++;
		}
	}

	ranNum = numberFree * (rand() / (RAND_MAX + 1.0));
	return terminalsUsed[ranNum];
}

// Author: Chris Rhodan
// Purpose: The consumer thread, thread to "consume/remove" airplanes departing from the airport
// Pre: true
// Post: keep_running = 0, pthread_join(AirportDepart,NULL);
void *AirportDepart(){

	while(keep_running){
		int numberFree = 0;
		int currentAirportCapacity = sem_getvalue(&empty, &numberFree);
		int semValue = 0;
		sem_getvalue(&full, &semValue);

		if(semValue == 0){
			printf("The airport is empty\n");
		}
		while(semValue == 0){
			sem_getvalue(&full, &semValue);
			if(!keep_running){
				break;
			}
		}

		if(IsPlaneDeparting()){
			if(!keep_running){
				break;
			}

			sem_wait(&full);
			sem_wait(&runway);

			int departureDock = CalculateDepartureDock();

			struct Airplane* selectedPlane;
			selectedPlane = airport[departureDock];

			printf("After staying at bay %d for %6.3f seconds, plane %s is taking off...\n", departureDock, GetStayTime(departureDock), selectedPlane->code); 
			sleep(2);
			printf("Plane %s has finished taking off\n", selectedPlane->code);
			
			//Acquire mutex lock to protect airport
			pthread_mutex_lock(&airport_mutex);

			free(airport[departureDock]);
			airport[departureDock] = NULL;

			// Release mutex lock and Full Semaphore
			pthread_mutex_unlock(&airport_mutex);

			sem_post(&runway);
			sem_post(&empty);		
		}
		usleep(500000);
	}
}

// Author: Chris Rhodan
// Purpose: Function to check if the parametres are whithin the correct range of allowed values
// Pre: true
// Post: False if (arrivalOdds < 1 || arrivalOdds > 90) or (departureOdds < 1 || departureOdds > 90)
int checkParameters(){
	if(arrivalOdds < 1 || arrivalOdds > 90){
		return FALSE;
	}

	if(departureOdds < 1 || departureOdds > 90){
		return FALSE;
	}

	return TRUE;
}

// Author: Chris Rhodan and Dustin Wilson
// Purpose: Main thread that initialise the simulation and the other threads, and runs the program
// Pre: true
// Post: prints out current airport status and joins all threads// Purpose: Main thread that initialise the simulation and the other threads, and runs the program
int main(int argc, char *argv[]){

	if(argc != 3){
		printf("Error: please enter in two parameters\n");
		return 0;
	}
	else{
		arrivalOdds = atoi(argv[1]);
		departureOdds = atoi(argv[2]);
		if(checkParameters()){
			printf("Welcome to the airport simulator.\n");
			printf("Press p or P followed by return to display the state of the airport\n");
			printf("Press q or Q followed by return to terminate the simulation\n");
			printf("\n Press Return to start the simulation\n");
			getchar();
		}
		else{
			printf("Error: Parameter can not be less than 1 or greater than 90\n");
			return 0;
		}	
	}

	InitialiseAirport();

	// Initialize the locks
	pthread_mutex_init(&airport_mutex, NULL);
	sem_init(&empty,0,10); // Empty - Starts at 10
	sem_init(&full,0,0); // Full - Starts at 0
	sem_init(&runway,0,1); // Runway - Starts at 1

	srand(time(0)); // Seed the rand 

	// Create PIDs for each of the threads
	pthread_t threads[3];

	int rc;

	rc = pthread_create(&threads[0], NULL, UserControls, 0);
	if(rc){
		// Error has occured in creating thread
		printf("\nERROR: Return Code from pthread_create is %d\n", rc);
		exit(-1);
	}

	rc = pthread_create(&threads[1], NULL, AirportArrival, 0);
	if(rc){
		// Error has occured in creating thread
		printf("\nERROR: Return Code from pthread_create is %d\n", rc);
		exit(-1);
	}

	rc = pthread_create(&threads[2], NULL, AirportDepart, 0);
	if(rc){
		// Error has occured in creating thread
		printf("\nERROR: Return Code from pthread_create is %d\n", rc);
		exit(-1);
	}

	pthread_join(threads[0],NULL);
	pthread_join(threads[1],NULL);
	pthread_join(threads[2],NULL);

	PrintCurrentState();

	int i = 0;
	for(i = 0; i < MAXIMUM_AIRPORT_CAPACITY; i++){
		free(airport[i]);
	}

	return 0;

}


