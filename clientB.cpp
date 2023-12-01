/**
 * Name: Vineet Agarwal
 * USC ID: 6693187979
 * Course Number: EE-450
 * Project: Socket Programming Project, Spring 2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>
#include <vector>

// UDP port numbers for connections between server M and server A, B and C
#define SERVER_A_UDP_PORT 21979
#define SERVER_B_UDP_PORT 22979
#define SERVER_C_UDP_PORT 23979
#define SERVER_M_UDP_PORT 24979 

// Port numbers for TCP connections
#define CLIENT_A_TCP_PORT 25979 
#define CLIENT_B_TCP_PORT 26979

#define LOCAL_HOST "127.0.0.1"

#define MAX_DATA_SIZE 1024

#define CLIENT_A "Client A"
#define CLIENT_B "Client B"
#define SERVER_A "Server A"
#define SERVER_B "Server B"
#define SERVER_C "Server C"
#define SERVER_M "Server M"

using namespace std;

void error(const char* message);

int createSocketTCP(string device);

sockaddr_in setSockAddr(string whosePort);

void connectSocket(int socket_descr, sockaddr_in clientA_Addr);

void checkWallet(int socket_descr, char* user1, bool transaction_flag=false);

void txCoins(int socket_descr, char* user1, char* user2, char* amount);

void txList(int socket_descr);

void userStats(int socket_descr, char* user1);

int main(int argc, char* argv[])
{	
	// Create socket descriptor for Client B
	int socket_descr = createSocketTCP(CLIENT_B);

	// Create socket variables for Client B
	struct sockaddr_in clientA_Addr = setSockAddr(CLIENT_B);
	
	// Print client status on boot
	cout << "The Client B is up and running" << endl;

	connectSocket(socket_descr, clientA_Addr);

	if (argc == 2)
	{	
		if (strcmp(argv[1], "TXLIST") == 0)
		{
			txList(socket_descr);
		}
		else
		{
			checkWallet(socket_descr, argv[1]);
		}
	}
	else if (argc == 3)
	{
		userStats(socket_descr, argv[1]);
	}
	else if (argc == 4)
	{
		txCoins(socket_descr, argv[1], argv[2], argv[3]);
	}

	close(socket_descr);
}

void error(const char* message)
{
    perror(message);
    exit(1);
}

int createSocketTCP(string device)
{
	// Create a socket for Client <--> Main Server commmunications
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == -1) 
	{
		string errorString = "Error opening socket for " + device + "...";
		error(errorString.c_str());
	}

	// Set socket options for Client socket
	int sockopt_val = 1;
	if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &sockopt_val, sizeof(sockopt_val)) == -1) 
	{
		string errorString = "Error setting socket options for " + device + "...";
		error(errorString.c_str());
	}

	return clientSocket;
}

sockaddr_in setSockAddr(string whosePort)
{	
	struct sockaddr_in sockAddr;

	memset(&sockAddr, '0', sizeof(sockAddr)); 
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
	if (strcmp(whosePort.c_str(), "Client A") == 0)
	{
		sockAddr.sin_port = htons(CLIENT_A_TCP_PORT);	
	}
	else if (strcmp(whosePort.c_str(), "Client B") == 0)
	{
		sockAddr.sin_port = htons(CLIENT_B_TCP_PORT);	
	}
	else if (strcmp(whosePort.c_str(), "Server A") == 0)
	{
		sockAddr.sin_port = htons(SERVER_A_UDP_PORT);	
	}
	else if (strcmp(whosePort.c_str(), "Server B") == 0)
	{
		sockAddr.sin_port = htons(SERVER_B_UDP_PORT);	
	}
	else if (strcmp(whosePort.c_str(), "Server C") == 0)
	{
		sockAddr.sin_port = htons(SERVER_C_UDP_PORT);	
	}

	return sockAddr;
}


void connectSocket(int socket_descr, sockaddr_in clientA_Addr)
{
	if (connect(socket_descr, (struct sockaddr * ) &clientA_Addr, sizeof(clientA_Addr)) == -1){
		error("Error making a connetion to the remote socket!");
	}
}

void checkWallet(int socket_descr, char* user1, bool transaction_flag)
{	
	
	char message[MAX_DATA_SIZE];
	strcpy(message, "1 ");
	strcat(message, user1);
	
    if (send(socket_descr, message, strlen(message), 0) == -1)
    {
    	error("Error sending balance check request from Client B.");
    }

    if (!transaction_flag) 
    {
    	cout << "\"" << user1 << "\" sent a balance enquiry request to the main server." << endl;
    }

    char* messageTo_Client = new char[MAX_DATA_SIZE];
    recv(socket_descr, messageTo_Client, MAX_DATA_SIZE, 0);
    
    if (!(strcmp(messageTo_Client, user1) == 0))
    {
    	cout << "The current balance of \"" << user1 << "\" is : " << messageTo_Client << " alicoins." << endl;
    }
    else
    {
    	cout << "\"" << user1 << "\" is not part of the network." << endl;
    }
    delete[](messageTo_Client);
}

void txCoins(int socket_descr, char* user1, char* user2, char* amount)
{
	char message[MAX_DATA_SIZE];
	strcpy(message, "2 ");
	strcat(message, user1);
	strcat(message, " ");
	strcat(message, user2);
	strcat(message, " ");
	strcat(message, amount);

	// Check the balance of the user
    if (send(socket_descr, message, strlen(message), 0) == -1)
    {
    	error("Error sending transaction request from Client B.");
    }

    cout << "\"" << user1 << "\" has requested to transfer " << amount << " coins to \""<< user2 << "\"." << endl;

	char* messageFromServer = new char[MAX_DATA_SIZE];
    int recvStatus = recv(socket_descr, messageFromServer, MAX_DATA_SIZE, 0);
    if (recvStatus==-1)
    {
    	error("Error receiving data from main server");
    }
    
    string tempString = string(messageFromServer);
	int justBeforeMessage = tempString.find("|");
	string transaction_status = tempString.substr(0,justBeforeMessage);


    cout << "transaction status: " << transaction_status << endl;
    if (strcmp(transaction_status.c_str(), "SUCCESS")==0)
    {
    	cout << "\"" << user1 << "\" successfully transferred " << amount << " alicoins to \"" << user2 << "\"." << endl;

    	string balance = tempString.substr(justBeforeMessage + 1, tempString.size()-justBeforeMessage-1);
    	
    	cout << "The current balance of \"" << user1 << "\" is : " << balance << " alicoins." << endl;
    }
    else if (strcmp(transaction_status.c_str(), "INSUFFICIENT_FUNDS")==0)
    {
    	cout << "\"" << user1 << "\" was unable to transfer " << amount << " alicoins to \"" << user2 << "\" because of insufficient funds." << endl;
    	
    	string balance = tempString.substr(justBeforeMessage + 1, tempString.size()-justBeforeMessage-1);
    	
    	cout << "The current balance of \"" << user1 << "\" is : " << balance << " alicoins." << endl;	
    }
    else if (strcmp(transaction_status.c_str(), user1)==0)
    {
    	cout << "Unable to proceed with the transaction as \"" << user1 << "\" is not part of the network." << endl;
    }
    else if (strcmp(transaction_status.c_str(), user2)==0)
    {
    	cout << "Unable to proceed with the transaction as \"" << user2 << "\" is not part of the network." << endl;
    }
    else if (strcmp(transaction_status.c_str(), "NO_USERS")==0)
    {
    	cout << "Unable to proceed with the transaction as \"" << user1 << "\" and \"" << user2 << "\" are not part of the network." << endl;
    }
    // cout << transaction_status << endl;
    delete[](messageFromServer);
}

void txList(int socket_descr)
{	
	char* message = new char[MAX_DATA_SIZE];
	strcat(message, "3 ");
	strcat(message, "TXLIST");

    if (send(socket_descr, message, strlen(message), 0) == -1)
    {
    	error("Error sending transaction list request from Client B.");
    }

    cout << "Client B sent a sorted list request to the main server." << endl;

}

void userStats(int socket_descr, char* user1)
{	
	char message[MAX_DATA_SIZE];
	strcat(message, "4 ");
	strcat(message, user1);

    if (send(socket_descr, message, strlen(message), 0) == -1)
    {
    	error("Error sending user stats request from Client B.");
    }
    
    cout << "\"" << user1 << " sent a balance inquiry request to the main server.\"" << endl;

    char* stats;
    recv(socket_descr, &stats, sizeof(stats), 0);

    cout << "\"" << user1 << "\" statistics are the following :" << endl;
}