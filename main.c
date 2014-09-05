

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
		airport[i] = NULL;
	}
}


// Calculates how long a plane has been parked
double GetStayTime(int dock){
	double timeTaken = 0;
	struct Airplane* currentPlane = airport[dock];

	return difftime(time(NULL), currentPlane->parkTime);
}


void ExitProgram(){
	keep_running = 0;
}

void PrintCurrentState(){
	printf("Airport State:\n");
	for (int i = 0; i < MAXIMUM_AIRPORT_CAPACITY; i++){
		printf("%d: ", i);
		if(airport[i] == NULL){
			printf("Empty\n");
		}
		else{
			printf("%s (has parked for %f seconds)\n", airport[i]->code, GetStayTime(i));
		}
	}
	
}

void *UserControls(){

	//printf("DEBUG: User Controls Started\n");
	
	while(keep_running){

		char inputLetter = NULL;
		scanf(" %c", &inputLetter);

		if(inputLetter == 'q' || inputLetter == 'Q'){
			printf("Debug: Exit\n");
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
	struct Airplane* newPlane = malloc(sizeof(struct Airplane));
	newPlane->code = CreateAirplaneCode();
	// Plance Landing 
	printf("Plane %s is landing", newPlane->code);
	// Sleep for 2 seconds? or just assign current time? Time for landing
	sleep(2);

	//Acquire mutex lock to protect airport
	pthread_mutex_lock(&airport_mutex);

	// Need to randomly generate a empty landing bay number
	landingBay = AssignLandingBay();

	// if landed printf("DEBUG: Plane %s parked in landing bay %d", assignedBay);
	printf("Plane %s parked in landing bay %d\n",newPlane->code, landingBay);
	newPlane->parkTime = time(NULL);
	airport[landingBay] = newPlane;
	
	// Release mutex lock and Full Semaphore
	pthread_mutex_unlock(&airport_mutex);

}

// Producer
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
		sleep(0.5);
	}
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

	
	int j = 0;
	int ranNum = 0;
	int numberFree = 0;
	sem_getvalue(&full, &numberFree);

	int terminalsUsed[numberFree];


	for(int i = 0; i < MAXIMUM_AIRPORT_CAPACITY; i++){
		if (airport[i] != NULL){
			terminalsUsed[j] = i;
			j++;
		}
	}

	ranNum = numberFree * (rand() / (RAND_MAX + 1.0));
	printf("RanNum is %d\n", ranNum);
	return terminalsUsed[ranNum];
}





// Consumer
void *AirportDepart(){

	printf("DEBUG: Airport Depart Started\n");

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
			printf("Dock is %d\n", departureDock);

			struct Airplane* selectedPlane;
			selectedPlane = airport[departureDock];

			printf("After staying at bay %d for %f seconds, plane %s is taking off...\n", departureDock, GetStayTime(departureDock), selectedPlane->code); 
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
			printf("Empty is release\n");			
		}
		

		sleep(0.5);

	}
}


int checkParameters(){
	if(arrivalOdds < 1 || arrivalOdds > 90){
		return 0;
	}

	if(departureOdds < 1 || departureOdds > 90){
		return 0;
	}

	return 1;
}

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

/*
	// DEBUG For testing
	struct Airplane* newPlane = malloc(sizeof(struct Airplane));
	newPlane->code = "AS0220";
	newPlane->parkTime = time(NULL);
	printf("Plane 1 is %s\n", ctime(&newPlane->parkTime));

	sem_wait(&empty);
	sem_wait(&runway);
	airport[5] = newPlane;
	sem_post(&runway);
	sem_post(&full);


	struct Airplane* newPlane2 = malloc(sizeof(struct Airplane));
	newPlane2->code = "AS2222";
	newPlane2->parkTime = time(NULL);
	printf("Plane 2 is %s\n", ctime(&newPlane2->parkTime));

	sem_wait(&empty);
	sem_wait(&runway);
	airport[6] = newPlane2;
	sem_post(&runway);
	sem_post(&full);*/

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