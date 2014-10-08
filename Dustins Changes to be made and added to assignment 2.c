#define NEW_ITEM_LENGTH 256

struct CalorieEntry * calorieEntries[CALORIESENTRIES];

void InitialiseCalarieEntries(){

	int i = 0;
		for(i = 0; i < CALORIESENTRIES; i++){
			calorieEntries[i] = malloc(sizeof(struct CalorieEntry));
			calorieEntries[i] = NULL;
	}

}

void ProcessConnection(){

	int currentSocket = NULL;

	char request[2];
	int numbytes;

	char searchTerm[SEARCHTERMLENGTH];
	char newItem[NEW_ITEM_LENGTH];

	while(1){

		//throw a mutex around this

		// Grab next item in queue
		currentSocket = GrabNextItemInQueue();

		if(currentSocket !=NULL){
			
			// Check the first recieved message
			if ((numbytes=recv(currentSocket, request, 1, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			
			request[numbytes] = '\0';

			// its a search request
			if(strcmp("s", request) == 0){

				if ((numbytes=recv(currentSocket, searchTerm, SEARCHTERMLENGTH, 0)) == -1) {
					perror("recv");
					exit(1);
				}

				searchTerm[numbytes] = '\0';

				printf("Received: %s",searchTerm);

				SearchForItem(currentSocket, searchTerm);
					
			} else if(strcmp("a", request) == 0) // its a add new item request

				// grab the new item
				if ((numbytes=recv(currentSocket, newItem, NEW_ITEM_LENGTH, 0)) == -1) {
					perror("recv");
					exit(1);
				}

				newItem[numbytes] = '\0';

				printf("Received: %s",newItem);

				// create a new item
				CreateCalorieEntry(newItem);

				printf("%s has been added", calorieEntries[(entriesAdded - 1)]->name);
			}

			// Close the current connection
			close(current_fd);
		}
	}
}

// Struct for a Queue
struct Queue {

	int * socketQueue[BACKLOG];
	int frontOfQueue;
	int nextFreeSpot;

};

// Function to Grab next item from the Queue
int GrabNextItemInQueue(){

	int *nextSocket = comQueuesocketQueue[comQueue.frontOfQueue];

	free(comQueue.socketQueue[comQueue.frontOfQueue]);

	if(nextSocket != NULL){
		comQueue.frontOfQueue = (comQueue.frontOfQueue + 1) % BACKLOG;
	}

	return *nextSocket;
}

void AddNewItemToQueue(int socket){

	int * newSocket = &socket;

	comQueue.socketQueue[comQueue.nextFreeSpot] = newSocket;

	comQueue.nextFreeSpot = (comQueue.nextFreeSpot + 1) % BACKLOG;

}


// Function to intialise the Queue
void IntialiseQueue(){

	int i;
	for( i = 0; i < BACKLOG; i++){

		comQueue.socketQueue[i] = malloc(sizeof(int));
	}	

	comQueue.frontOfQueue = 0;
	comQueue.nextFreeSpot = 1;
}


// Function to Create the thread pool
void CreateThreadPool(){

	// Create PIDs for each of the threads
	pthread_t threadPool[10];

	int rc;

	int i;

	for(i = 0; i < BACKLOG; i++){
		
		rc = pthread_create(&threads[0], NULL, ProcessConnection, 0);
		
		if(rc){
			// Error has occured in creating thread
			printf("\nERROR: Return Code from pthread_create is %d\n", rc);
			exit(-1);
		}

	}
}

// Function to add the new item to the correct spot in the array (alphabetically)
void AddNewItemArray(struct CalorieEntry * newEntry, int isNewEntry){
	
	if(isNewEntry){

		int isSorted = 0;
		int currentIndex = 0;
		int comparison = 0;

		// Create a temp array and give it some memory, and copy in the old array
		struct CalorieEntry *tmpArray[entriesAdded];
		int i = 0;
		for(i = 0; i < entriesAdded; i++){
			tmpArray[i] = malloc(sizeof(struct CalorieEntry));
			tmpArray[i] = calorieEntries[i];
		}
		
		// Increment the entries to include the new addition
		entriesAdded++;
		
		// Resize calorieEntry to add in the new item
		calorieEntries = (CalorieEntry *)realloc(CalorieEntry, entriesAdded);

		// While not sorted
		while(!isSorted){

			// Compare the new entry against the current position entry
			comparison = CompareItems(newEntry, tmpArray[currentIndex]);

			// If it comes before the current item alphabetically add it in that
			// position and set the flag to exit the loop
			if(comparison == 1){
				calorieEntries[currentIndex] = newEntry;
				printf("%s\n", calorieEntries[currentIndex]->name);
				isSorted = 1;
			} else {
				calorieEntries[currentIndex] = tmpArray[currentIndex];

				// free up none used memory
				free(tmpArray[currentIndex]);
			}

			currentIndex++;
		}

		// Shift the rest of the items down into thier new positions
		for (i = currentIndex; i < entriesAdded; i++){

			calorieEntries[i] = tmpArray[(i-1)];
			free(tmpArray[(i-1)]); // free up none used memory
		}

		// not sure if I need this
		free(tmpArray);

	} else {

		// Save the current food's data into the array of entries and increment the counter for entries
		calorieEntries[entriesAdded] = newEntry;

		printf("%s\n", calorieEntries[entriesAdded]->name); // I assume this is here for debugging
		entriesAdded++;
	}
	
}

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

