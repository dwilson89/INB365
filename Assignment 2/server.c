/*
*  Server-side code for the INB365 Assignment 2
*	Authors:
*		Chris Rhodan n6862624
*		Dustin Wilson n6325157
*
*	Due Date: 21st October 2014
*/


#include <arpa/inet.h>
#include <stdio.h> 
#include <ctype.h>
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define MYPORT 12345    /* the default port users will be connecting to */
#define BACKLOG 10     /* how many pending connections queue will hold */
#define CALORIESENTRIES 959 // Number of unique entries in calories.csv, ideally would be dyanmic
#define SEARCHTERMLENGTH 128 /* max number of bytes we can get at once */
#define TRUE 1
#define FALSE 0
#define NEW_ITEM_LENGTH 256

int keep_running = TRUE;

//pthread_mutex_t rw_mutex;
//pthread_mutex_t mutex;
pthread_mutex_t q_mutex;

sem_t rw_mutex;
//sem_t q_mutex;
sem_t mutex;
int read_count = 0;

sem_t q_empty;
sem_t q_full;

int initialCalEntries;

// Struct to hold an entry from calories.csv

#pragma pack(1)
struct CalorieEntry {
	char name[128];
	char measure[32];
	int weight;
	int cal;
	int fat;
	int carb;
	int protein;

};
#pragma pack(0)

// Struct for a Queue
struct Queue {

	int socketQueue[BACKLOG];
	int frontOfQueue;
	int nextFreeSpot;

};

// Array to hold all the entries
struct CalorieEntry ** calorieEntries;

// Tracks how many entries have been added so far
int entriesAdded = 0;

// Stores the minimum number of commas for a given line in the calories.csv file (i.e. the
// the number of commas if the name doesn't include any commas in it)
int minCommas = 6;

// Queue for the threadpool
struct Queue comQueue;



// Function to compare two entries in order to determine position
int CompareItems(struct CalorieEntry * newEntry, struct CalorieEntry * oldEntry){

	int compare = strcmp(newEntry->name, oldEntry->name);

	int isNewEntrySpot = 0;

	//check names
	if(compare == 0){//if equal check the next property and so forth

		compare = strcmp(newEntry->measure, oldEntry->measure);

		if(compare == 0){

			isNewEntrySpot = 1;

		}
	} 

	if (compare < 0){

		isNewEntrySpot = 1;		

	} else if (compare > 0){

		isNewEntrySpot = 0;
	}

	//check the next and so forth till a conclusion is found
	return isNewEntrySpot;
}


// Returns 1 if the search term is found in the name, 0 if it is not
int CompareNames(char searchTerm[128], char foodName[128], int numSpaces){
	
	int count = numSpaces;

	for(int i = 0; i < 128; i++){
		// If we've reached the end of the word, either because we've run out of spaces
		// or reached a NULL character then break (and eventually, return true)
		if(count == 0 || foodName[i] == NULL){
			break;
		}
		// If there's a space, reduce the word count by 1
		if(foodName[i] == " "){
			count--;
		}
		else{
			// If the search term doesn't match the name of the food, return false
			if(searchTerm[i] != foodName[i]){
				return 0;
			}
		}
	}

	return 1;

}

// Searches for an item and sends results to client
void SearchForItem(int fd, char searchTerm[128]){

	// Get the number of words in the search term by counting the spaces
	int searchSpaceCount = 0;
	for (int i = 0; i < 128; i++){
		if (searchTerm[i] == ' '){
			searchSpaceCount = searchSpaceCount + 1;
		}
	}
	
	// Apply reader solution
	// reader-writer locked for reader
	sem_wait(&mutex);
	read_count++;
	if(read_count == 1)
		sem_wait(&rw_mutex);
	sem_post(&mutex);

	// Cycle through all entries
	for (int i = 0; i < entriesAdded; i++){
		// Count how many spaces are in the name of the current food
		int nameSpaceCount = 1;	

		for (int j = 0; j < strlen(calorieEntries[i]->name); j++){
			if(calorieEntries[i]->name[j] == ' '){
				nameSpaceCount++;
			}
		}

		// The search term has to be equal to or shorter than the name for
		// the search to work
		if (searchSpaceCount <= nameSpaceCount){

			// Because the algoirthm makes no practical sense, we start at the start of the word and 
			// ignore the same search term later in the name. I.e., searching for "cake" should return
			// "cake or pastry flour" but not "carrot cake" (making this probably the least practical
			// search tool ever conceived)

			// Shorten the number of words in the name so it matches the number of words of the searchTerm 
			char shortenedName[128];

			// As it's a pointer, the original name is overwritten by the shorter name, so we store
			// the original name here so we can recall it later once we've checked the shorter name
			char originalName[128];
			strcpy(originalName, calorieEntries[i]->name);

			// String Token
			char *token;

			// Create the initial Token
			token = strtok(calorieEntries[i]->name, " ");

			// Copy the intial token into the shortened name
			strcpy(shortenedName, token);

			// Add any additional words, based on whether theres spaces
			for(int i = 1; i < searchSpaceCount; i++){
				// Get the next token and append it after a space
				token = strtok(NULL, " ");
				strcat(shortenedName, " ");
				strcat(shortenedName, token);
			}

			// Add a newline character as the search term includes a new line character
			strcat(shortenedName, "\n");
			
			// Convert the shortened name to all lower case
			for(int i = 0; i < 128; i++){
				if(shortenedName[i] == ','){
					strtok(shortenedName, ",");
					strcat(shortenedName, "\n");
				}
				shortenedName[i] = tolower(shortenedName[i]);
			}
			
			// Convert the search term to all lower case
			for(int i = 0; i < 128; i++){
				searchTerm[i] = tolower(searchTerm[i]);
			}
			
			// Check if the search term matches the shortened name, and if so then send the details
			// to the client
			if(CompareNames(shortenedName, searchTerm, nameSpaceCount)){
				// Reset the food name back to it's original name
				strcpy(calorieEntries[i]->name, originalName);
				struct CalorieEntry temp = *calorieEntries[i];
				// Send the result to the client
				send(fd, &temp, sizeof(temp), 0);
				printf("Sending item: %s\n", calorieEntries[i]->name);
			}
			strcpy(calorieEntries[i]->name, originalName);

		}
	}
	
	// reader-writer unlocked for reader
	sem_wait(&mutex);
	read_count--;
	if(read_count == 0)
		sem_post(&rw_mutex);
	sem_post(&mutex);

	struct CalorieEntry endMessage;
	strcpy(endMessage.name, "End Message");
	send(fd, &endMessage, sizeof(endMessage), 0);

	printf("Search Complete\n");

}

// Function to add the new item to the correct spot in the array (alphabetically)
void AddNewItemArray(struct CalorieEntry * newEntry, int isNewEntry){
	
	if(isNewEntry){

		int isSorted = 0;
		int currentIndex = 0;
		int comparison = 0;

		// Read from the array
		sem_wait(&mutex);
		read_count++;
		if(read_count == 1)
			sem_wait(&rw_mutex);
		sem_post(&mutex);

		// Create a temp array and give it some memory, and copy in the old array
		struct CalorieEntry *tmpArray[entriesAdded];
		int i = 0;
		for(i = 0; i < entriesAdded; i++){
			tmpArray[i] = malloc(sizeof(struct CalorieEntry));
			*tmpArray[i] = *calorieEntries[i];
			//printf("%s has been added to the temp array\n", tmpArray[i]->name);
		}

		sem_wait(&mutex);
		read_count--;
		if(read_count == 0)
			sem_post(&rw_mutex);
		sem_post(&mutex);
		
		// Increment the entries to include the new addition
		entriesAdded++;
		
		// Lock read-write mutex
		//pthread_mutex_lock(&rw_mutex);
		sem_wait(&rw_mutex);

		// Resize calorieEntry to add in the new item
		calorieEntries = (struct CalorieEntry **)realloc(calorieEntries, entriesAdded * sizeof(struct CalorieEntry*));

		calorieEntries[entriesAdded-1] = malloc(sizeof(struct CalorieEntry));

		// While not sorted
		while(!isSorted){

			// Compare the new entry against the current position entry
			comparison = CompareItems(newEntry, tmpArray[currentIndex]);

			// If it comes before the current item alphabetically add it in that
			// position and set the flag to exit the loop
			if(comparison == 1){
				calorieEntries[currentIndex] = newEntry;
				//printf("%s\n", calorieEntries[currentIndex]->name);
				isSorted = 1;
			} else {
				//calorieEntries[currentIndex] = tmpArray[currentIndex];

				// free up none used memory
				free(tmpArray[currentIndex]);
			}

			currentIndex++;
		}

		// Shift the rest of the items down into thier new positions
		for (i = currentIndex; i < entriesAdded; i++){

			*calorieEntries[i] = *tmpArray[(i-1)];
			//printf("entry %d out of %d: %s\n", i, entriesAdded, calorieEntries[i]->name);
			free(tmpArray[(i-1)]); // free up none used memory
		}

		//printf("%s has been added\n", calorieEntries[currentIndex-1]->name);
		//pthread_mutex_unlock(&rw_mutex);
		sem_post(&rw_mutex);
		// not sure if I need this
		// free(tmpArray);
		// Unlock read-write mutex

	} else {

		// Save the current food's data into the array of entries and increment the counter for entries
		calorieEntries[entriesAdded] = newEntry;

		//printf("%s\n", calorieEntries[entriesAdded]->name); // I assume this is here for debugging
		entriesAdded++;
	}
	
}

// Create a new CalorieEntry struct from the given line from the Calories.csv file
void CreateCalorieEntry(char line[256], int entryType){
	// This entry will contain food data for the new entry
	struct CalorieEntry * newEntry;

	// Assign memory to the newly
	newEntry =  malloc(sizeof(struct CalorieEntry));

	// Clear strings
	memset(newEntry->name, 0, sizeof(newEntry->name));
	memset(newEntry->measure, 0, sizeof(newEntry->measure));
	
	// Count the number of "extra" commas in the line, which will be used when creating the name
	int commaCount = 0;
	for (int i = 0; i < 255; i++){
		// Once we've reached the end of the string, no reason to keep the loop going
		if(line[i] == NULL){
			break;
		}
		// If the entry is a comma, add one to the count
		if(line[i] == ','){
			commaCount = commaCount + 1;
		}
	}
	
	// Populate the Structure with Data from the line in the CSV file

	// Create a token which will contain the values from the file up until the next commas
	char *token;

	// Create the initial Token
	token = strtok(line, ",");

	// Copy the intial token into the name of the current food entry
	strcpy(newEntry->name, token);

	// Calculate how many commas are included in the name of the food, as these are in addition
	// to the usual 6 commas included, they are counted as negative entries
	int count = 0 -commaCount + minCommas;

	// While there is still text in the line that hasn't been processed

	while(token != NULL){
		// Get the next token
		token = strtok(NULL, ",");

		// If the current token is still part of the name of the food
		//printf("%d\n", count);
		if(count < 0){
			//printf("Less tha zero\n");
			// Add in the comma and space as they won't be included in the token
			strcat(newEntry->name, ", ");
			// Add the token to the name of the food
			strcat(newEntry->name, token);
		}
		else{
			//printf("%s\n", newEntry.name);
			switch (count){
				// Once the name is completed, the next 6 values should only have a single
				// comma seperating them, so can be populated using a switch and counter
				case 0:
					strcpy(newEntry->measure, token);
					break;
				case 1:
					newEntry->weight = atoi(token);
					break;
				case 2:
					newEntry->cal = atoi(token);
					break;		
				case 3:
					newEntry->fat = atoi(token);
					break;				
				case 4:
					newEntry->carb = atoi(token);
					break;
				case 5:
					newEntry->protein = atoi(token);
					break;
			}
		}
	count++;
	} 

	// Add it to the array, entryType indicates its an original item
	AddNewItemArray(newEntry, entryType);
}


void LoadCSV(){
	// Create a new file pointer than points to calories.csv
	FILE *file = fopen("calories.csv", "r");

	if(file != NULL){
		// Buffer to hold the current line of text from the csv file
		char line[256];

		// Return every line from the csv file until there are no more lines
		while(fgets(line, sizeof(line), file) != NULL){
			// Don't include comments
			if(line[0] != '#'){
				
				// Create new calorie entry
				CreateCalorieEntry(line, 0);
				
			}
		}		
		fclose(file);
	}

}

void GetInitialCalorieCount(){
	
	initialCalEntries = 0;
	// Create a new file pointer than points to calories.csv
	FILE *file = fopen("calories.csv", "r");

	if(file != NULL){
		// Buffer to hold the current line of text from the csv file
		char line[256];

		// Return every line from the csv file until there are no more lines
		while(fgets(line, sizeof(line), file) != NULL){
			// Don't include comments
			if(line[0] != '#'){
				
				// Create new calorie entry
				//CreateCalorieEntry(line, 0);
				initialCalEntries++;
			}
		}		
		fclose(file);
	}
}

// Function to update and save the .csv file
void UpdateAndSaveFile(){

	FILE *file = fopen("calories.csv","r+");
	fpos_t pos;

	int keep_reading = 1;

	if(file != NULL){

		char line[256];

		// Read in the '#' lines
		while((fgets(line, sizeof(line),file) != NULL) &&(keep_reading == 1)){

			if(line[0] != '#'){

				keep_reading = 0;

			} else {

				// Save the last known position (should be the start of the next line)
				fgetpos(file,&pos);
			}
		}

		// Set the last know position to current position
		fsetpos(file, &pos);

		// Start adding in every element back into the file in the new order
		int i;
		for(i = 0; i < entriesAdded; i++){

			// Turn each entrie back into a string
			sprintf(line, "%s,%s,%d,%d,%d,%d,%d", calorieEntries[i]->name,\
				calorieEntries[i]->measure,calorieEntries[i]->weight,calorieEntries[i]->cal,\
				calorieEntries[i]->fat,calorieEntries[i]->carb,calorieEntries[i]->protein);

			// Add each entry to the file
			fputs(line, file);

			// Add a newline if i does not equal last entry
			if(i != (entriesAdded-1)){

				fputs("\n",file);
			}

		}

		// Close the file
		fclose(file);
	}
}

// Function to Grab next item from the Queue
int GrabNextItemInQueue(){

	int nextSocket = comQueue.socketQueue[comQueue.frontOfQueue];

	comQueue.socketQueue[comQueue.frontOfQueue] = NULL;

	if(nextSocket != NULL){
		comQueue.frontOfQueue = (comQueue.frontOfQueue + 1) % BACKLOG;
	}

	return nextSocket;
}

// Function to add new connections to the queue
void AddNewItemToQueue(int socket){

	int newSocket = socket;

	comQueue.socketQueue[comQueue.nextFreeSpot] = newSocket;

	comQueue.nextFreeSpot = (comQueue.nextFreeSpot + 1) % BACKLOG;

}

// Function to Process the Connection
void* ProcessConnection(void *thread){

	int currentSocket = NULL;

	char request[2];
	int numbytes;

	char searchTerm[SEARCHTERMLENGTH];
	char newItem[NEW_ITEM_LENGTH];

	int is_connected = 1;

	while(1){

		printf("Thread %d grabbing Queued Item\n", (int)thread);

		sem_wait(&q_full);
		// Grab next item in queue
		pthread_mutex_lock(&q_mutex);
		//sem_wait(&q_mutex);
		currentSocket = GrabNextItemInQueue();
		//sem_post(&q_mutex);
		pthread_mutex_unlock(&q_mutex);
		sem_post(&q_empty);

		printf("Thread %d grabbing Queued Item: %d\n", (int)thread, currentSocket);

		if(currentSocket != NULL){
			
			printf("connected to: %d\n", currentSocket);

			is_connected = 1;

			while(is_connected) {
			
				// Check the first recieved message
				if ((numbytes=recv(currentSocket, request, 1, 0)) == -1) {
					perror("recv");
					exit(1);
				}
				
				request[numbytes] = '\0';

				printf("Request is: %s\n",request);

				// its a search request
				if(strcmp("s", request) == 0){

					if ((numbytes=recv(currentSocket, searchTerm, SEARCHTERMLENGTH, 0)) == -1) {
						perror("recv");
						exit(1);
					}

					searchTerm[numbytes] = '\0';

					printf("Received: %s",searchTerm);

					SearchForItem(currentSocket, searchTerm);
						
				} else if(strcmp("a", request) == 0) {// its a add new item request

					// grab the new item
					if ((numbytes=recv(currentSocket, newItem, NEW_ITEM_LENGTH, 0)) == -1) {
						perror("recv");
						exit(1);
					}

					newItem[numbytes] = '\0';

					printf("Received: %s",newItem);

					// create a new item
					CreateCalorieEntry(newItem, 1);

					UpdateAndSaveFile();//remove this later

					// Potential extension add code to send a message back for confirmation

				} else if(strcmp("q", request) == 0){
					// Close the current connection
					close(currentSocket);
					is_connected = 0;
				}
			}
		}
	}
}

// Function to Create the thread pool
void CreateThreadPool(pthread_t * threadPool){

	int rc;
	int i;

	for(i = 0; i < BACKLOG; i++){
		
		rc = pthread_create(&threadPool[i], NULL, ProcessConnection, (void *) i);
		
		if(rc){
			// Error has occured in creating thread
			printf("\nERROR: Return Code from pthread_create is %d\n", rc);
			exit(-1);
		}
	}
}

// Function to intialise the Queue
void InitialiseQueue(){

	int i;
	for( i = 0; i < BACKLOG; i++){

		comQueue.socketQueue[i] = NULL;
	}	

	comQueue.frontOfQueue = 0;
	comQueue.nextFreeSpot = 0;
}

// Function to initialise the Calorie Entries list
void InitialiseCalarieEntries(){

	printf("initial count: %d", initialCalEntries);

	calorieEntries = malloc(initialCalEntries * sizeof(struct calorieEntries*));

	int i = 0;
		for(i = 0; i < initialCalEntries; i++){
			calorieEntries[i] = malloc(sizeof(struct CalorieEntry));
			calorieEntries[i] = NULL;
	}
	printf("initial array created\n");

}

int main(int argc, char *argv[])
{
	int sockfd, new_fd, numbytes;  /* listen on sock_fd, new connection on new_fd */
	struct sockaddr_in my_addr;    /* my address information */
	struct sockaddr_in their_addr; /* connector's address information */
	socklen_t sin_size;
	char searchTerm[SEARCHTERMLENGTH];

	// Create PIDs for each of the threads
	pthread_t threadPool[10];
	CreateThreadPool(threadPool);
	
	// Initialize the locks
	//pthread_mutex_init(&rw_mutex, NULL);
	//pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&q_mutex, NULL);
	sem_init(&rw_mutex,0,1); // rw_mutex - Starts at 1
	//sem_init(&q_mutex,0,1); // q - Starts at 1
	sem_init(&mutex,0,1); // mutex- Starts at 1

	sem_init(&q_empty,0,10); // 
	sem_init(&q_full,0,0);

	// Initialise queue and calories array
	GetInitialCalorieCount();
	InitialiseCalarieEntries();
	InitialiseQueue();

	/* generate the socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	/* generate the end point */
	my_addr.sin_family = AF_INET;         /* host byte order */

	// Check if a port has been specified by the user and if so, open on the default port
	// otherwise, open with the specified one
	if(argc == 2){
		my_addr.sin_port = htons(atoi(argv[1]));     /* short, network byte order */
	}
	else{
		my_addr.sin_port = htons(MYPORT);     /* short, network byte order */
	}

	
	my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
		/* bzero(&(my_addr.sin_zero), 8);   ZJL*/     /* zero the rest of the struct */

	/* bind the socket to the end point */
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) \
	== -1) {
		perror("bind");
		exit(1);
	}

	/* start listnening */
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("Loading Entries\n");

	LoadCSV();

	printf("Entries Successfully Loaded\n");

	printf("server starts listnening ...\n");

	/* repeat: accept, send, close the connection */
	/* for every accepted connection, use a sepetate process or thread to serve it */
	while(keep_running) {  /* main accept() loop */
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, \
		&sin_size)) == -1) {
			perror("accept");
			continue;
		}

		printf("server: got connection from %s\n", \
			inet_ntoa(their_addr.sin_addr));

		printf("Adding to Queue socket: %d\n", new_fd);

		sem_wait(&q_empty);
		// Add the new connection to the queue
		pthread_mutex_lock(&q_mutex);
		//sem_wait(&q_mutex);
		AddNewItemToQueue(new_fd);
		//sem_post(&q_mutex);
		pthread_mutex_unlock(&q_mutex);
		printf("Added to Queue\n");
		sem_post(&q_full);
	}
}
