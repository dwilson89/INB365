/*
*  Client-side code for the INB365 Assignment 2
*	Authors:
*		Chris Rhodan n6862624
*		Dustin Wilson n6325157
*
*	Due Date: 21st October 2014
*/


#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>

#define PORT 12345    /* the port client will be connecting to */
#define MAXDATASIZE 100 /* max number of bytes we can get at once */
#define SEARCHTERMLENGTH 128 // The maximum length of the search term
#define TRUE 1
#define FALSE 0 

int keep_running = TRUE;

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct hostent *he;
	struct sockaddr_in their_addr; /* connector's address information */

	if (argc != 3) {
		fprintf(stderr,"usage: <client hostname> <client port>\n");
		exit(1);
	}

	if ((he=gethostbyname(argv[1])) == NULL) {  /* get the host info */
		herror("gethostbyname");
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	their_addr.sin_family = AF_INET;      /* host byte order */

	// Set the Port to the user defined port
	their_addr.sin_port = htons(atoi(argv[2]));     /* short, network byte order */

	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */


	// Connect to the Server

	if (connect(sockfd, (struct sockaddr *)&their_addr, \
	sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}


	// TODO: Finish Main Loop

	while(keep_running){

		printf("Enter the food name to search for, or 'q' to quit:\n");
		while(1){
			char searchTerm[SEARCHTERMLENGTH];  // Buffer to store the search term
			
			if(fgets(searchTerm, sizeof(searchTerm), stdin)){
				// TODO: Send Request to Server

				// TODO: Receive Response from Server

				// TODO: Output Response to user


				break;

			}
			else{
				printf("An Error has occured in your search term, please try again\n\n");
				break;
			}

		}
		
	}

	


	

	if ((numbytes=recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	buf[numbytes] = '\0';

	printf("Received: %s",buf);

	close(sockfd);

	return 0;
}
