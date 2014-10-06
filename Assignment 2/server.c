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

#define MYPORT 12345    /* the default port users will be connecting to */
#define BACKLOG 10     /* how many pending connections queue will hold */
#define CALORIESENTRIES 959 // Number of unique entries in calories.csv, ideally would be dyanmic
#define SEARCHTERMLENGTH 128 /* max number of bytes we can get at once */
#define TRUE 1
#define FALSE 1


int keep_running = TRUE;


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

// Array to hold all the entries

struct CalorieEntry calorieEntries[CALORIESENTRIES];

// Tracks how many entries have been added so far
int entriesAdded = 0;

// Stores the minimum number of commas for a given line in the calories.csv file (i.e. the
// the number of commas if the name doesn't include any commas in it)
int minCommas = 6;


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
	
	// Cycle through all entries
	for (int i = 0; i < CALORIESENTRIES; i++){
		// Count how many spaces are in the name of the current food
		int nameSpaceCount = 1;	

		for (int j = 0; j < strlen(calorieEntries[i].name); j++){
			if(calorieEntries[i].name[j] == ' '){
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
			strcpy(originalName, calorieEntries[i].name);

			// String Token
			char *token;

			// Create the initial Token
			token = strtok(calorieEntries[i].name, " ");

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
				strcpy(calorieEntries[i].name, originalName);

				// Send the result to the client
				send(fd, &calorieEntries[i], sizeof(calorieEntries[i]), 0);
			}
			strcpy(calorieEntries[i].name, originalName);

		}
	}

	struct CalorieEntry endMessage;
	strcpy(endMessage.name, "End Message");
	send(fd, &endMessage, sizeof(endMessage), 0);


}

// Create a new CalorieEntry struct from the given line from the Calories.csv file
void CreateCalorieEntry(char line[256]){
	// This entry will contain food data for the new entry
	struct CalorieEntry newEntry;

	// Clear strings
	memset(newEntry.name, 0, sizeof(newEntry.name));
	memset(newEntry.measure, 0, sizeof(newEntry.measure));
	
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
	strcpy(newEntry.name, token);

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
			strcat(newEntry.name, ", ");
			// Add the token to the name of the food
			strcat(newEntry.name, token);
		}
		else{
			//printf("%s\n", newEntry.name);
			switch (count){
				// Once the name is completed, the next 6 values should only have a single
				// comma seperating them, so can be populated using a switch and counter
				case 0:
					strcpy(newEntry.measure, token);
					break;
				case 1:
					newEntry.weight = atoi(token);
					break;
				case 2:
					newEntry.cal = atoi(token);
					break;		
				case 3:
					newEntry.fat = atoi(token);
					break;				
				case 4:
					newEntry.carb = atoi(token);
					break;
				case 5:
					newEntry.protein = atoi(token);
					break;
			}
		}
	count++;
	} 
	

	//AddNewItemArray(newEntry, 0);

	// Save the current food's data into the array of entries and increment the counter for entries
	calorieEntries[entriesAdded] = newEntry;

	//char originalName[128];
	//strcpy(originalName, calorieEntries[entriesAdded].name);
	//printf("%s\n",strtok(calorieEntries[entriesAdded].name, ","));
	//strcpy(calorieEntries[entriesAdded].name, originalName);
	printf("%s\n", calorieEntries[entriesAdded].name);
	entriesAdded++;

	
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
				CreateCalorieEntry(line);
			}
		}		
		fclose(file);
	}

}


int main(int argc, char *argv[])
{
	int sockfd, new_fd, numbytes;  /* listen on sock_fd, new connection on new_fd */
	struct sockaddr_in my_addr;    /* my address information */
	struct sockaddr_in their_addr; /* connector's address information */
	socklen_t sin_size;
	char searchTerm[SEARCHTERMLENGTH];

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
	while(1) {  /* main accept() loop */
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, \
		&sin_size)) == -1) {
			perror("accept");
			continue;
		}

		while(keep_running){
			if ((numbytes=recv(new_fd, searchTerm, SEARCHTERMLENGTH, 0)) == -1) {
			perror("recv");
			exit(1);
			}

			searchTerm[numbytes] = '\0';

			printf("Received: %s",searchTerm);

			SearchForItem(new_fd, searchTerm);

		}


		printf("server: got connection from %s\n", \
			inet_ntoa(their_addr.sin_addr));
		if (!fork()) { /* this is the child process */
			//if (send(new_fd, "Test!""", 14, 0) == -1)
		/*
			if (send(new_fd, calorieEntries[0].name, sizeof(calorieEntries[0].name), 0) == -1)
				perror("send");
			close(new_fd); */
			exit(0);
		}
		//close(new_fd);  /* parent doesn't need this */

		while(waitpid(-1,NULL,WNOHANG) > 0); /* clean up child processes */
	}
}
