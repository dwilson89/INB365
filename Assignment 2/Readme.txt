To compile the server program, please use the command;

gcc -o server server.c -std=gnu99 -lpthread

where "server" can be changed to whatever name you prefer for the program.

To run the server, it's usage is;

./server <port>	Note: <port> is optional, if no port specified it uses 12345

For example, to use the server on locahost port 12345 use the command

./server 12345




To compile the client program, please use the command;

gcc -o client client.c

where "client" can be changed to whatever name you prefer for the program.

To run the client, usage is;

./client <hostname> <port>

For example, to use the client on locahost port 12345 use the command

./client localhost 12345
