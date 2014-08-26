

//	Solution for INB365 Assignment 1
// 
//	Authors:
//	Chris Rhodan n6862624
//	Dustin Wilson <add student number>
//
//	Due Date:
// 	9th September 2014

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

// Struct to represent a single Airplane
struct Airplane{
	char code[6];
	time_t parkTime;
};

// Array of Pointers to Airplanes to represent the Airport
struct Airplane *airport[10];

// Variables to contain the odds of a plane landing and departing
int arrivalOdds = 0;
int departureOdds = 0;

// Function to allocate memory for each Airplane in the airport
void InitialiseAirport(){
	for(int i = 0; i < 10; i++){
		airport[i] = malloc(sizeof(struct Airplane));
	}
}

void *UserControls(){

	printf("DEBUG: User Controls Started\n");

	// TODO: Add Code

	pthread_exit(NULL);
}

void *AirportArrival(){

	printf("DEBUG: Airport Arrival Started\n");

	// TODO: Add Code

	pthread_exit(NULL);
}

void *AirportDepart(){

	printf("DEBUG: Airport Depart Started\n");

	// TODO: Add Code

	pthread_exit(NULL);
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


	pthread_exit(NULL);

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