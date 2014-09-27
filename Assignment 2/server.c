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


// Struct to hold an entry from calories.csv

struct CalorieEntry {
	char name[128];
	char measure[32];
	int weight;
	int cal;
	int fat;
	int carb;
	int protein;

};

// Array to hold all the entries

struct CalorieEntry calorieEntries[CALORIESENTRIES];

// Tracks how many entries have been added so far
int entriesAdded = 0;

// Stores the minimum number of commas for a given line in the calories.csv file (i.e. the
// the number of commas if the name doesn't include any commas in it)
int minCommas = 6;


// Searches for an item and sends results to client
void SearchForItem(int fd, char searchTerm[128]){
	if (send(fd, calorieEntries[0].name, sizeof(calorieEntries[0].name), 0) == -1){
		perror("send");
	}

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
	int count = -commaCount + minCommas;


	// While there is still text in the line that hasn't been processed

	while(token != NULL){
		// Get the next token
		token = strtok(NULL, ",");

		// If the current token is still part of the name of the food

		if(count < 0){
			// Add in the comma and space as they won't be included in the token
			strcat(newEntry.name, ", ");
			// Add the token to the name of the food
			strcat(newEntry.name, token);
		}
		else{
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
	
	// Save the current food's data into the array of entries and increment the counter for entries
	calorieEntries[entriesAdded] = newEntry;
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


		if ((numbytes=recv(new_fd, searchTerm, SEARCHTERMLENGTH, 0)) == -1) {
			perror("recv");
			exit(1);
		}

		searchTerm[numbytes] = '\0';

		printf("Received: %s",searchTerm);


		SearchForItem(new_fd, searchTerm);



		printf("server: got connection from %s\n", \
			inet_ntoa(their_addr.sin_addr));
		if (!fork()) { /* this is the child process */
			//if (send(new_fd, "Test!\n", 14, 0) == -1)
			if (send(new_fd, calorieEntries[0].name, sizeof(calorieEntries[0].name), 0) == -1)
				perror("send");
			close(new_fd);
			exit(0);
		}
		close(new_fd);  /* parent doesn't need this */

		while(waitpid(-1,NULL,WNOHANG) > 0); /* clean up child processes */
	}
}
