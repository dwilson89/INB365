

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

#define MAXIMUM_AIRPORT_CAPACITY 10
#define AIRPLANE_CODE 6

// Not sure if this compiler allows bools
#define TRUE 1
#define FALSE 0 

// Struct to represent a single Airplane
struct Airplane{
	char *code;
	time_t parkTime;
};

// Array of Pointers to Airplanes to represent the Airport - shared resource
struct Airplane *airport[MAXIMUM_AIRPORT_CAPACITY];

// Variables to contain the odds of a plane landing and departing
int arrivalOdds = 0;
int departureOdds = 0;

int keep_running = 1;

pthread_mutex_t airport_mutex;
sem_t empty;
sem_t full;
sem_t runway;

// Function to allocate memory for each Airplane in the airport
void InitialiseAirport(){
	for(int i = 0; i < MAXIMUM_AIRPORT_CAPACITY; i++){
		airport[i] = malloc(sizeof(struct Airplane));
	}
}

void *UserControls(){

	printf("DEBUG: User Controls Started\n");

	// TODO: Add Code

	//pthread_exit(NULL);
}

// Creates an random 6 character code for the generated airplane 2 capital 
// letters and 4 numerical values
char *CreateAirplaneCode(){

	int ranNum = 0;
	int counter = 0;

	//ASCII for 'A' is 65, '0' is 48
	char *code;
	code = malloc(AIRPLANE_CODE*sizeof(char));

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

// Checks if a new plane is to be generated
int IsPlaneGenerated(){

	int ranNum = 0;
	int isGenerated = 0;

	// Calculate a value between 0 and 100
	ranNum = 100 * (rand() / (RAND_MAX + 1.0));

	// If value is in arrivalOdds range 0 - arrivalOdds (it has that chance to be in that range)
	if(ranNum < arrivalOdds) {

		isGenerated = 1;
	}

	return isGenerated;
}

// Randomly Assigns a empty landing bay for the landed airplane
int AssignLandingBay(){

	int isBayFull = 1;
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

void generate_airplane(){

	int landingBay;

	// Create a new plane	
	struct Airplane newPlane;
	newPlane.code = CreateAirplaneCode();
	// if landing printf("DEBUG: Plane %s is landing", newPlane.code);
	printf("DEBUG: Plane %s is landing", newPlane.code);
	// Sleep for 2 seconds? or just assign current time? Time for landing
	sleep(2000);

	//Acquire mutex lock to protect airport
	pthread_mutex_lock(&airport_mutex);

	// Need to randomly generate a empty landing bay number
	landingBay = AssignLandingBay();

	// if landed printf("DEBUG: Plane %s parked in landing bay %d", assignedBay);
	printf("DEBUG: Plane %s parked in landing bay %d",newPlane.code, landingBay);
	newPlane->parkTime = time(NULL);
	airport[landingBay] = newPlane;
	
	// Release mutex lock and Full Semaphore
	pthread_mutex_unlock(&airport_mutex);

}

// Producer
void *AirportArrival(){

	printf("DEBUG: Airport Arrival Started\n");

	// TODO: Add Code for semaphore and mutexes

	// Keeps track of airport capacity
	int currentAirportCapacity = 0;

	while(keep_running){

		// Get the current count for the full semaphore to indicate airport capacity
		sem_getvalue(&full, &currentAirportCapacity);

		sleep(500);

		// Is a new plane to be generated
		if(IsPlaneGenerated()){
			
			// Report to console if Airport is full
			if (currentAirportCapacity == MAXIMUM_AIRPORT_CAPACITY){
				printf("DEBUG: The airport is full");
			}
			// Free up runway
			isRunwayFree = TRUE;
		}*/

			// Acquire Empty and Runway Semaphore
			sem_wait(&empty); // This should block if 0 or full
			sem_wait(&runway);
			
			generate_airplane();
			
			// Release Full and Runway Semaphore
			sem_post(&runway);
			sem_post(&full);
		}
	}
	
	//pthread_exit(NULL);
}

// Function to determine whether a plane should be departing or not

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

// Calculates the terminal/dock from which the plane will depart
int CalculateDepartureDock(){

	int terminalsUsed[currentAirportCapacity];
	int j = 0;
	int ranNum = 0;

	for(int i = 0; i < MAXIMUM_AIRPORT_CAPACITY; i++){
		if (airport[i] != NULL){
			terminalsUsed[j] = i;
			j++;
		}
	}

	ranNum = currentAirportCapacity * (rand() / (RAND_MAX + 1.0));
	return ranNum;
}


// Calculates how long a plane has been parked
double GetStayTime(int dock){
	double timeTaken = 0;
	struct Airplane* currentPlane = airport[dock];

	return difftime(time(NULL), currentPlane->parkTime);
}


// Consumer
void *AirportDepart(){

	printf("DEBUG: Airport Depart Started\n");

	while(keep_running){
		if(currentAirportCapacity == 0){
			printf("The airport is empty");

			while(currentAirportCapacity == 0){
				// Blocking
			}
		}

		if(isRunwayFree && IsPlaneDeparting()){

			int departureDock = CalculateDepartureDock();
			struct Airplane* selectedPlane;
			selectedPlane = airport[departureDock];
			printf("After staying at bay %d for %f seconds, plane %s is taking off...", departureDock, GetStayTime(departureDock), selectedPlane->code); 
			isRunwayFree = FALSE;
			sleep(2);
			printf("Plane %s has finished taking off", selectedPlane->code);
			free(airport[departureDock]);
			airport[departureDock] = NULL;
			isRunwayFree = TRUE;			
		}

		sleep(0.5);

	}
}


int main(int argc, char *argv[]){

	if(argc != 3){
		printf("Error, please enter in two parameters\n");
		return 0;
	}
	else{
		arrivalOdds = atoi(argv[1]);
		departureOdds = atoi(argv[2]);
		printf("Welcome to the airport simulator.\n");
		printf("Press p or P followed by return to display the state of the airport\n");
		printf("Press q or Q followed by return to terminate the simulation\n");
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

	//pthread_exit(NULL);

	return 0;

}



/*		Notes


	To Define a new plane:
	struct Airplane newPlane;

	To Set the Airplane parkedTime;
	newPlane.parkTime = time(NULL);

	To find how long it was parked for;
	timeParked = time(NULL) - newPlane.parkTime

*/