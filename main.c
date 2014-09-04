

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

#define MAXIMUM_AIRPORT_CAPACITY 10
#define AIRPLANE_CODE 6

// Not sure if this compiler allows bools
#define TRUE 1
#define FALSE 0 

// Struct to represent a single Airplane
struct Airplane{
	char code[AIRPLANE_CODE];
	time_t parkTime;
};

// Array of Pointers to Airplanes to represent the Airport - shared resource
struct Airplane *airport[MAXIMUM_AIRPORT_CAPACITY];

// Variables to contain the odds of a plane landing and departing
int arrivalOdds = 0;
int departureOdds = 0;

// Variable to determine if run way is free or occupied; has true for free, 
// false for occupied - shared resource
//bool isRunwayFree = true;
int isRunwayFree = TRUE;

// Keeps track of airport capacity
int currentAirportCapacity = 0;

int keep_running = 1;

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

char *CreateAirplaneCode(){

	int ranNum = 0;
	int counter = 0;

	//ASCII for 'A' is 65, '0' is 48
	char code[AIRPLANE_CODE];
	for(int i = 0; i < AIRPLANE_CODE; i++){
		airport[i] = malloc(sizeof(char));
	}

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

int AssignLandingBay(){

	int isBayFull = 1;

	int ranNum = 0;
	

	do{
		
		ranNum = 10 * (rand() / (RAND_MAX + 1.0));
		if(airport[ranNum] == NULL){

			isBayFull = 0;
		}

	} while(isBayFull);

	return ranNum;

}

void generate_airplane(){

	int landingBay;

	// check the Runway - might need to swap this around
	if(isRunwayFree){
		
		isRunwayFree = FALSE;
		// Is a new plane to be generated
		if(IsPlaneGenerated()){

		// Create a new plane	
			struct Airplane newPlane;
			newPlane->code = CreateAirplaneCode();
		// if landing printf("DEBUG: Plane %s is landing", newPlane.code);
			printf("DEBUG: Plane %s is landing", newPlane->code);
		// Sleep for 2 seconds? or just assign current time? Time for landing
			sleep(2000);
		// Need to randomly generate a empty landing bay number
			landingBay = AssignLandingBay();

		// if landed printf("DEBUG: Plane %s parked in landing bay %d", assignedBay);
			printf("DEBUG: Plane %s parked in landing bay %d",newPlane->code, landingBay);
			newPlane->parkTime = time(NULL);
			airport[landingBay] = newPlane;
			currentAirportCapacity++;

		}
		// Free up runway
		isRunwayFree = TRUE;
	}

}

// Producer
void *AirportArrival(){

	printf("DEBUG: Airport Arrival Started\n");

	// TODO: Add Code for semaphore and mutexes

	int landingBay;

	while(keep_running){

		// Possibly a loop that keeps it here until room - blocking
		while(currentAirportCapacity == MAXIMUM_AIRPORT_CAPACITY){
			printf("DEBUG: The airport is full");
			sleep(1);
		}

		sleep(500);
		
/*		// check the Runway - might need to swap this around
		if(isRunwayFree){
			
			isRunwayFree = FALSE;
			// Is a new plane to be generated
			if(IsPlaneGenerated()){

			// Create a new plane	
				struct Airplane newPlane;
				newPlane->code = CreateAirplaneCode();
			// if landing printf("DEBUG: Plane %s is landing", newPlane.code);
				printf("DEBUG: Plane %s is landing", newPlane->code);
			// Sleep for 2 seconds? or just assign current time? Time for landing
				sleep(2000);
			// Need to randomly generate a empty landing bay number
				landingBay = AssignLandingBay();

			// if landed printf("DEBUG: Plane %s parked in landing bay %d", assignedBay);
				printf("DEBUG: Plane %s parked in landing bay %d",newPlane->code, landingBay);
				newPlane->parkTime = time(NULL);
				airport[landingBay] = newPlane;
				currentAirportCapacity++;

			}
			// Free up runway
			isRunwayFree = TRUE;
		}*/

		generate_airplane();

	}
	
	//pthread_exit(NULL);
}


// Consumer
void *AirportDepart(){

	printf("DEBUG: Airport Depart Started\n");

	// TODO: Add Code

	//pthread_exit(NULL);
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