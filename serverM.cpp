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
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iomanip>

// Set the localhost to 127.0.0.1
#define LOCAL_HOST "127.0.0.1"

// UDP port numbers for connections between server M and server A, B and C
#define SERVER_A_UDP_PORT 21979
#define SERVER_B_UDP_PORT 22979
#define SERVER_C_UDP_PORT 23979
#define SERVER_M_UDP_PORT 24979 

// Port numbers for TCP connections
#define CLIENT_A_TCP_PORT 25979 
#define CLIENT_B_TCP_PORT 26979

// Initial balance for all people in Alichain
#define INITIAL_BALANCE 1000

#define MAX_DATA_SIZE 1024
#define STATUS_MESSAGE_LENGTH 30

#define CLIENT_A "Client A"
#define CLIENT_B "Client B"
#define SERVER_A "Server A"
#define SERVER_B "Server B"
#define SERVER_C "Server C"
#define SERVER_M "Server M"

#define CHECK_WALLET_REQUEST "1"
#define LOG_TRANSACTION_REQUEST "2"
#define GET_LOGS_REQUEST "3"
#define USER_STATS_REQUEST "4"
#define CHECK_USER_EXISTS_REQUEST "5"

#define CHECK_WALLET_REQUEST_FLAG 1
#define LOG_TRANSACTION_REQUEST_FLAG 2
#define GET_LOGS_REQUEST_FLAG 3
#define USER_STATS_REQUEST_FLAG 4
#define CHECK_USER_EXISTS_REQUEST_FLAG 5


#define MAIN_LOG_FILE "alichain.txt"

using namespace std;

int txID = 0;
bool transactionFlag = true;

/*Returns the total number of logs on all servers*/
int getCountofAllLogs();

/*Returns the balance of the user*/
char* checkWallet_onMain(char* user, bool userExistCheck=false);

/*Checks if the users exist or not*/
char* checkUsersExist(char* user1, char* user2);

/*Handles the transaction between two users and the logging of said transaction*/
char* txCoins_onMain(char* user1, char* user2, char* txAmt);

/*Creates a sorted transaction list on the main server*/
char* txList_onMain();

/*Comparator to sort vector of logs*/
bool transactionComparator(string logA, string logB);

/*Creates and returns a command*/
char* createCommand(int request, char* user1=NULL, char* user2=NULL, char* txAmt=NULL, int txNumber=0);

/*Connects to the given backend server and communicates with it and returns the message received from the server*/
char* connectTo_ServerBackend(char* command, string device, int request);

/*Prints out the error and terminates the server*/
void error(const char* message);

/*Creates a UDP socket for given device*/
int createSocketTCP(string device);

/*Creates a UDP socket for given device*/
int createSocketUDP(string device);

/*Sends an integer data value to the destination socket with the destination address*/
void sendInt(int num, int destination_socket, sockaddr_in destination_addr, string device);

/*Sends an character* data to the destination socket with the destination address*/
void sendChar(char* message, int destination_socket, sockaddr_in destination_addr, string device);

/*Binds the socket with the clientSocket descriptor and socketAddr address */
void bindSocket(int clientSocket, sockaddr_in sockAddr, string device);

/*Accepts client sockets
* Referred from - https://stackoverflow.com/questions/15560336/listen-to-multiple-ports-from-one-server
*/
int accept_client_sockets(int list_desc[], unsigned int clientCount, struct sockaddr *address, socklen_t *address_length);

/*Finds and returns port for the device*/
int findPortForDevice(string device);

/*Finds and returns the device for a given socket_address*/
string findDeviceForAddress(sockaddr_in socket_address);

/*Chooses and returns a random server from the backend servers A, B and C*/
string chooseDevice();

/*Sets the socket address for the mentioned port*/
sockaddr_in setSockAddr(string whosePort);

int main(int argc, char const *argv[])
{
	int socket_ClientA = createSocketTCP(CLIENT_A);
	int socket_ClientB = createSocketTCP(CLIENT_B);

	struct sockaddr_in sockaddr_ClientA = setSockAddr(CLIENT_A);
	struct sockaddr_in sockaddr_ClientB = setSockAddr(CLIENT_B);	
	
	bindSocket(socket_ClientA, sockaddr_ClientA, CLIENT_A);
	bindSocket(socket_ClientB, sockaddr_ClientB, CLIENT_B);

	listen(socket_ClientA, 1);
	listen(socket_ClientB, 1);

	cout << "The main server is up and running." << endl;

    //Socket descriptors for Client A and Client B and the central server sockets
    int socket_Client;

    // Create a socket address and socket address length holder for 
    struct sockaddr_in sockaddr_Client;
    socklen_t addr_length = sizeof(sockaddr_Client);

    //Create an array of socket descriptors of both Client A and Client B
    int socket_Desc[2];
    socket_Desc[0] = socket_ClientA;
    socket_Desc[1] = socket_ClientB;

    //Char arrays to store the names entered by Client A and Client B
    char clientCommand[MAX_DATA_SIZE];	

    //Keep the Central Server permanently active, until the user terminates the server
    while(1)
    {

        //Accept the connection of either Client A or Client B (whichever client connects to the central server first)
        socket_Client = accept_client_sockets(socket_Desc, 2, (struct sockaddr *) &sockaddr_Client, &addr_length);
        if(socket_Client == -1){
            error("Error accepting socket from client...");
        }

        //Receive the name sent by either client A or client B
        int recvData = recv(socket_Client, &clientCommand, MAX_DATA_SIZE + 1, 0);
        if(recvData == -1){
            error("Error receiving username data from client...");
        }
        clientCommand[recvData] = '\0';

        //Get the locally bound name of the either Client A or Client B socket
        int check_getsockname_client = getsockname(socket_Client, (struct sockaddr *) &sockaddr_Client, (socklen_t *)&addr_length);
        if(check_getsockname_client < 0)
        {
        	error("getsockname for Client A or Client B socket failed...");
        }

		// Process the received data from the client	
		// cout << clientCommand << endl;
		
	 	char* command = new char[MAX_DATA_SIZE];
		strcpy(command, clientCommand);

	    char* splitCommand[5];
	    splitCommand[0] = strtok(clientCommand, " "); // Splits spaces between words in str
	    splitCommand[1] = strtok (NULL, " ");
	    splitCommand[2] = strtok (NULL, " ");
	    splitCommand[3] = strtok (NULL, " ");
	    // cout << (string)input[0] + " " + (string)input[1] << endl;// << input[2] << input[3] << endl;

	    if(strcmp(splitCommand[0],"1")==0)
	    {
			
			// Display message on Main Server for the balance check request
	    	cout << "The main server received input=\"" << splitCommand[1] << "\" from the" << findDeviceForAddress(sockaddr_Client) << " using TCP over port " 
	    		 <<ntohs(sockaddr_Client.sin_port)<<endl; 

			char* clientBalance = checkWallet_onMain(splitCommand[1]);
			if (send(socket_Client, clientBalance, sizeof(clientBalance), 0) == -1)
			{	
				string errorString = "Error sending balance to " + findDeviceForAddress(sockaddr_Client);
				error(errorString.c_str());
			}
			delete[](clientBalance);

			cout << "The main server sent the current balance to " << findDeviceForAddress(sockaddr_Client) << endl;

		}
		else if(strcmp(splitCommand[0],"2")==0)
		{

			// Display message on Main Server for the transaction request
	    	cout << "The main server received from \"" << splitCommand[1] << "\" to transfer " << splitCommand[3] << " coins to \""
	    		 << splitCommand[2] << "\" using TCP over port " << ntohs(sockaddr_Client.sin_port) << endl; 

	    	// send the transaction to backend servers and resulting status to client
			char* transactionStatus = txCoins_onMain(splitCommand[1], splitCommand[2], splitCommand[3]);

			if(send(socket_Client, transactionStatus, strlen(transactionStatus), 0) == -1)
			{
				string errorString = "Error sending message to " + findDeviceForAddress(sockaddr_Client);
				error(errorString.c_str());
			}
			delete[](transactionStatus);

			cout << "The main server sent the result of the transaction to " << findDeviceForAddress(sockaddr_Client) << endl;

		}
		else if(strcmp(splitCommand[0],"3")==0)
		{
			// Display message on Main Server for transaction list request
			cout << "The main server received input=\"" << splitCommand[1] << "\" from the client using TCP over port " 
	    		 << ntohs(sockaddr_Client.sin_port) <<endl; 

			char* transactionListStatus = txList_onMain();
			if (send(socket_Client, transactionListStatus, strlen(transactionListStatus), 0) == -1)
			{	
				string errorString = "Error sending transaction list creation status to " + findDeviceForAddress(sockaddr_Client);
				error(errorString.c_str());
			}

		}

	}
	return 0;	
}


int getCountofAllLogs()
{
	char* commandToBackendServer = createCommand(GET_LOGS_REQUEST_FLAG);

	char* data_ServerA = connectTo_ServerBackend(commandToBackendServer, SERVER_A, GET_LOGS_REQUEST_FLAG);
	char* data_ServerB = connectTo_ServerBackend(commandToBackendServer, SERVER_B, GET_LOGS_REQUEST_FLAG);
	char* data_ServerC = connectTo_ServerBackend(commandToBackendServer, SERVER_C, GET_LOGS_REQUEST_FLAG);	

	stringstream allLogs((string)data_ServerA + (string)data_ServerB + (string)data_ServerC);
	string singleLog;

	vector<string> logToFile;

	while(getline(allLogs, singleLog, '\n'))
	{
		if(singleLog.compare("")!=0)
		{
			logToFile.push_back(singleLog);
		}
	}

	return logToFile.size();
}

char* checkWallet_onMain(char* user, bool userExistCheck)
{	
	//check if user exists if not already checked
	if (!userExistCheck)
	{
		
		char* userStatus = checkUsersExist(user, NULL);

		if (strcmp(userStatus, user)==0)
		{
			delete[](userStatus);
			char* sendUserMissingMessage = new char[MAX_DATA_SIZE];
			strcpy(sendUserMissingMessage, user);
			return sendUserMissingMessage;
		}
	}

	char* commandToBackendServer = createCommand(CHECK_WALLET_REQUEST_FLAG, user);

	char* data_ServerA = connectTo_ServerBackend(commandToBackendServer, SERVER_A, CHECK_WALLET_REQUEST_FLAG);
	char* data_ServerB = connectTo_ServerBackend(commandToBackendServer, SERVER_B, CHECK_WALLET_REQUEST_FLAG);
	char* data_ServerC = connectTo_ServerBackend(commandToBackendServer, SERVER_C, CHECK_WALLET_REQUEST_FLAG);

	int balance = INITIAL_BALANCE;

	stringstream allLogsOfUser((string)data_ServerA + (string)data_ServerB + (string)data_ServerC);
	string singleLog;

	// For each log
	while(getline(allLogsOfUser, singleLog, '\n'))
	{
		vector<string> logData;
		stringstream splitLog(singleLog);
		string word;
		while(getline(splitLog, word, ' '))
			logData.push_back(word);

		stringstream txAmount(logData[3]);
		int transferAmount;
		txAmount >> transferAmount;
		
		// Check if the user received or sent Alicoins
		if(strcmp(logData[1].c_str(),user)==0)
		{	
			// If sent, subtract from the initial balance
			balance -= transferAmount;
		}
		else if(strcmp(logData[2].c_str(),user)==0)
		{
			// If received, add to the initial balance	
			balance += transferAmount;
		}
	}

	char* balanceToReturn = new char[MAX_DATA_SIZE];
	stringstream ss;
	ss << balance;
	ss >> balanceToReturn;
	return balanceToReturn;
}

char* checkUsersExist(char* user1, char* user2)
{
	char* commandToCheckUser = createCommand(CHECK_USER_EXISTS_REQUEST_FLAG, user1, user2);

	char* userStatus_ServerA = connectTo_ServerBackend(commandToCheckUser, SERVER_A, CHECK_USER_EXISTS_REQUEST_FLAG);
	char* userStatus_ServerB = connectTo_ServerBackend(commandToCheckUser, SERVER_B, CHECK_USER_EXISTS_REQUEST_FLAG);
	char* userStatus_ServerC = connectTo_ServerBackend(commandToCheckUser, SERVER_C, CHECK_USER_EXISTS_REQUEST_FLAG);

	bool user1MissingOnA = strcmp(userStatus_ServerA, "USER_1_MISSING")==0 || strcmp(userStatus_ServerA, "NO_USERS")==0;
	bool user1MissingOnB = strcmp(userStatus_ServerB, "USER_1_MISSING")==0 || strcmp(userStatus_ServerB, "NO_USERS")==0;
	bool user1MissingOnC = strcmp(userStatus_ServerC, "USER_1_MISSING")==0 || strcmp(userStatus_ServerC, "NO_USERS")==0;

	bool user2MissingOnA = strcmp(userStatus_ServerA, "USER_2_MISSING")==0 || strcmp(userStatus_ServerA, "NO_USERS")==0;
	bool user2MissingOnB = strcmp(userStatus_ServerB, "USER_2_MISSING")==0 || strcmp(userStatus_ServerB, "NO_USERS")==0;
	bool user2MissingOnC = strcmp(userStatus_ServerC, "USER_2_MISSING")==0 || strcmp(userStatus_ServerC, "NO_USERS")==0;

	bool user1DoesNotExist = (user1MissingOnA && user1MissingOnB && user1MissingOnC);
	bool user2DoesNotExist = (user2MissingOnA && user2MissingOnB && user2MissingOnC);
	bool bothUsersDoNotExist = (user1DoesNotExist && user2DoesNotExist);

	char* status = new char[MAX_DATA_SIZE];
	// cout << "user1: " << user1DoesNotExist << " user2: " << user2DoesNotExist << endl;
	if (bothUsersDoNotExist)
	{
		strcpy(status, "NO_USERS");
	}
	else if (user2DoesNotExist)
	{
		strcpy(status, user2);
	}
	else if (user1DoesNotExist)
	{
		strcpy(status, user1);
	}
	else 
	{
		strcpy(status, "BOTH_USERS_EXIST");
	}

	delete[](userStatus_ServerA);
	delete[](userStatus_ServerB);
	delete[](userStatus_ServerC);

	return status;
}

char* txCoins_onMain(char* user1, char* user2, char* txAmount)
{
	
	if (transactionFlag)
	{
		txID = getCountofAllLogs() + 1;
		transactionFlag = false;
	}

	// check user availability
	char* userStatus = checkUsersExist(user1, user2);

	char* transactionMessage = new char[MAX_DATA_SIZE];
	// handling if either of the user is not in the databases
	if(strcmp(userStatus,"BOTH_USERS_EXIST")!=0)
	{
		strcpy(transactionMessage, userStatus);
	}
	else
	{
		delete[](userStatus);
		// calculating the userFrom current balance
		char* user1Balance = checkWallet_onMain(user1, true);

		// handling the case where the userFrom has insufficient balance
		if(atoi(user1Balance) < atoi(txAmount)){

			strcpy(transactionMessage, "INSUFFICIENT_FUNDS");
			strcat(transactionMessage, "|");
			strcat(transactionMessage, user1Balance);

		}
		// handling the logging of transaction at the backend servers
		else{

			// log the transaction data to one of the servers A,B,C at random
			
			string device = chooseDevice();
			
			char* command = createCommand(LOG_TRANSACTION_REQUEST_FLAG, user1, user2, txAmount, txID);
			
			strcpy(transactionMessage, connectTo_ServerBackend(command, device, LOG_TRANSACTION_REQUEST_FLAG)); 

			// check updated balance of the user
			strcat(transactionMessage, "|");
			strcat(transactionMessage, checkWallet_onMain(user1, true));

			txID++;
		}
	}
	return transactionMessage;	
}

char* txList_onMain()
{
	char* commandToBackendServer = createCommand(GET_LOGS_REQUEST_FLAG);

	char* data_ServerA = connectTo_ServerBackend(commandToBackendServer, SERVER_A, GET_LOGS_REQUEST_FLAG);
	char* data_ServerB = connectTo_ServerBackend(commandToBackendServer, SERVER_B, GET_LOGS_REQUEST_FLAG);
	char* data_ServerC = connectTo_ServerBackend(commandToBackendServer, SERVER_C, GET_LOGS_REQUEST_FLAG);

	stringstream allLogs((string)data_ServerA + (string)data_ServerB + (string)data_ServerC);
	string singleLog;

	vector<string> logToFile;

	while(getline(allLogs, singleLog, '\n'))
	{
		if(singleLog.compare("")!=0)
		{
			logToFile.push_back(singleLog);
		}
	}

	sort(logToFile.begin(), logToFile.end(), transactionComparator);

	ofstream logFile(MAIN_LOG_FILE);

	for (int i = 0; i < logToFile.size(); i++)
	{
		// cout << logToFile[i] << endl;
		logFile << logToFile[i] << endl;
	}

	logFile.close();

	char* messageToClient = new char[MAX_DATA_SIZE];
	strcpy(messageToClient, "SUCCESS");

	return messageToClient;
}

bool transactionComparator(string logA, string logB)
{

	stringstream log1(logA.substr(0,logA.find(" ")));
	stringstream log2(logB.substr(0,logB.find(" ")));
	
	int id_logA, id_logB;
	
	log1 >> id_logA;
	log2 >> id_logB;

	return id_logA < id_logB;
}

char* createCommand(int request, char* user1, char* user2, char* txAmt, int txNumber)
{
	if(request == CHECK_WALLET_REQUEST_FLAG){

		// data to send to the backend server
		char* command = new char[MAX_DATA_SIZE];
		strcpy(command, CHECK_WALLET_REQUEST);
		strcat(command, " ");
		strcat(command, user1);

		return command;

	}
	// log transaction on the backend server A
	else if(request == LOG_TRANSACTION_REQUEST_FLAG){

		// data to send to the backend server
		char* command = new char[MAX_DATA_SIZE];
		stringstream txId;
		txId << txNumber;
		strcpy(command, LOG_TRANSACTION_REQUEST);
		strcat(command, " ");
		strcat(command, txId.str().c_str());
		strcat(command, " ");
		strcat(command, user1);
		strcat(command, " ");
		strcat(command, user2);
		strcat(command, " ");
		strcat(command, txAmt);

		return command;

	}
	// check the usernames exist in database of server A
	else if(request == CHECK_USER_EXISTS_REQUEST_FLAG){

		// data to send to the backend server
		char* command = new char[MAX_DATA_SIZE];
		strcpy(command, CHECK_USER_EXISTS_REQUEST);
		strcat(command, " ");
		strcat(command, user1);

		if(user2!=NULL)
		{
			strcat(command, " ");
			strcat(command, user2);
		}

		return command;

	}
	// get log of transactions from server A
	else if(request == GET_LOGS_REQUEST_FLAG){

		// data to send to the backend server
		char* command = new char[MAX_DATA_SIZE];
		strcpy(command, GET_LOGS_REQUEST);

		return command;

	}

	return NULL;
}

char* connectTo_ServerBackend(char* command, string device, int request)
{
	// cout << "DEBUG-in connect " << command << " sizeof: " << sizeof(command) << " MAX_DATA_SIZE: " << MAX_DATA_SIZE<< endl;

  	// create a main server socket 	
    int mainTo_ServerBackend = createSocketUDP(device);

    // set the server address for UDP communication
	struct sockaddr_in sockaddr_ServerBackend = setSockAddr(device);
	struct sockaddr_in sockaddr_ServerM = setSockAddr(SERVER_M);

	// bind the socket to server A address
	bindSocket(mainTo_ServerBackend, sockaddr_ServerM, SERVER_M);

	// send the client command to the server A
    
    sendChar(command, mainTo_ServerBackend, sockaddr_ServerBackend, device);

    if (request == CHECK_WALLET_REQUEST_FLAG)
	{
	   	cout<<"The main server sent a request to " << device << "." <<endl;
	}

    char* messageFrom_Backend = new char[MAX_DATA_SIZE];
    socklen_t sockaddr_length = sizeof(sockaddr_ServerBackend);

    if ((request == GET_LOGS_REQUEST_FLAG) || (request == CHECK_WALLET_REQUEST_FLAG))
    {
    	int numLogs;
    	
    	int receivedBytesNumLogs = recvfrom(mainTo_ServerBackend, &numLogs, MAX_DATA_SIZE, 0, (struct sockaddr *) &sockaddr_ServerBackend, &sockaddr_length);
	    if(receivedBytesNumLogs == -1)
	    {	
	    	string errorString = "Error in receiving data from " + device + "...";
	    	error(errorString.c_str());
	    }

	    strcpy(messageFrom_Backend, "");
	    char singleLog[MAX_DATA_SIZE];
	    for(int i = 0; i < numLogs; i++)
	    {

	    	memset(singleLog, '0', sizeof(singleLog));
	    	int receivedBytesSingleLog = recvfrom(mainTo_ServerBackend, &singleLog, sizeof(singleLog), 0, (struct sockaddr *) &sockaddr_ServerBackend, &sockaddr_length);
	    	if(receivedBytesSingleLog == -1)
	    	{
	    		string errorString = "Error in receiving data from " + device + "...";
	    		error(errorString.c_str());
	    	}
	    	singleLog[receivedBytesSingleLog] = '\0';

	    	// append the received message to the final message
	    	strcat(messageFrom_Backend, singleLog);
	    	strcat(messageFrom_Backend, "\n");
	    }
    }
    else
    {
    	int receivedBytes = recvfrom(mainTo_ServerBackend, messageFrom_Backend, MAX_DATA_SIZE, 0, (struct sockaddr *) &sockaddr_ServerBackend, &sockaddr_length);
	    if(receivedBytes == -1)
	    {
	    	error(string("Error in receiving data from " + device + "...").c_str());
	    }

	    messageFrom_Backend[receivedBytes] = '\0';
    }

    if(request == CHECK_WALLET_REQUEST_FLAG)
    {
    	cout<<"The main server received transactions from server " + device + " using UDP over port " << findPortForDevice(device) << "." << endl;
    }
	else if(request == LOG_TRANSACTION_REQUEST_FLAG)
	{
	    cout<<"The main server received the feedback from server " + device + " using UDP over port " << findPortForDevice(device) << "." << endl;
	}
	
    close(mainTo_ServerBackend);

    return messageFrom_Backend;
	
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
		string errorString = "Error opening socket for " + device + " on Main Server...";
		error(errorString.c_str());
	}

	// Set socket options for Client socket
	int sockopt_val = 1;
	if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &sockopt_val, sizeof(sockopt_val)) == -1) 
	{
		string errorString = "Error setting socket options for " + device + " on Main Server...";
		error(errorString.c_str());
	}

	return clientSocket;
}

int createSocketUDP(string device)
{
	// Create a socket for Client <--> Main Server commmunications
	int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocket == -1) 
	{
		string errorString = "Error opening socket for " + device + " on Main Server...";
		error(errorString.c_str());
	}

	// Set socket options for Client socket
	int sockopt_val = 1;
	if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &sockopt_val, sizeof(sockopt_val)) == -1) 
	{
		string errorString = "Error setting socket options for " + device + " on Main Server...";
		error(errorString.c_str());
	}

	return clientSocket;
}

void sendInt(int num, int destination_socket, sockaddr_in destination_addr, string device)
{
	int sentBytes = sendto(destination_socket, &num, sizeof(num) + 1, 0, (struct sockaddr *) &destination_addr, (socklen_t) sizeof(destination_addr));
	if(sentBytes == -1)
	{
        error(string("Error sending data from main server to " + device + "...").c_str());
    }
}

void sendChar(char* message, int destination_socket, sockaddr_in destination_addr, string device)
{
	int sentBytes = sendto(destination_socket, message, MAX_DATA_SIZE + 1, 0, (struct sockaddr *) &destination_addr, (socklen_t) sizeof(destination_addr));
	if(sentBytes == -1)
	{	
		string errorString = "Error sending data from main server to " + device + "...";
    	error(errorString.c_str());
    }
}

sockaddr_in setSockAddr(string whosePort)
{	
	struct sockaddr_in sockAddr;

	memset(&sockAddr, '0', sizeof(sockAddr)); 
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
	if (strcmp(whosePort.c_str(), CLIENT_A) == 0)
	{
		sockAddr.sin_port = htons(CLIENT_A_TCP_PORT);	
	}
	else if (strcmp(whosePort.c_str(), CLIENT_B) == 0)
	{
		sockAddr.sin_port = htons(CLIENT_B_TCP_PORT);	
	}
	else if (strcmp(whosePort.c_str(), SERVER_A) == 0)
	{
		sockAddr.sin_port = htons(SERVER_A_UDP_PORT);	
	}
	else if (strcmp(whosePort.c_str(), SERVER_B) == 0)
	{
		sockAddr.sin_port = htons(SERVER_B_UDP_PORT);	
	}
	else if (strcmp(whosePort.c_str(), SERVER_C) == 0)
	{
		sockAddr.sin_port = htons(SERVER_C_UDP_PORT);	
	}

	return sockAddr;
}

void bindSocket(int clientSocket, sockaddr_in sockAddr, string device)
{
	if (bind(clientSocket, (struct sockaddr*) &sockAddr, sizeof(sockAddr)) == -1)
	{
		string errorString = "Error binding socket for " + device + " on Main Server...";
		error(errorString.c_str());
	}
}

int accept_client_sockets(int list_desc[], unsigned int clientCount, struct sockaddr *address, socklen_t *address_length)
{
    fd_set read_filedesc;

    //Set the value of the maximum file descriptor to -1 initially 
    int maximum_fd = -1;
    int socket_desc;
    int selectStatus;
    //Initialize the file descriptor set to zero
    FD_ZERO(&read_filedesc);


    //For all the socket descriptors which are currently bound and listening on their respective ports
    for (int i = 0; i < clientCount; i++) 
    {    
        //Set the bit of the file descriptor list_desc in the file descriptor set read_filedesc
        FD_SET(list_desc[i], &read_filedesc);

        //Set the value of the maximum_fd
        if (list_desc[i] > maximum_fd)
        {
            maximum_fd = list_desc[i];
        }
    }

    //Get the file descriptor which is ready to send the data amongst the two clients
    selectStatus = select(maximum_fd + 1, &read_filedesc, NULL, NULL, NULL);

    //If neither clients are ready to send data, return -1
    if (selectStatus < 0)
    {
        return -1;
    }

    socket_desc = -1;
    for (int i = 0; i < clientCount; i++)
    {
        //Get the value of the socket descriptor which is ready to send data 
        if (FD_ISSET(list_desc[i], &read_filedesc))
        {
            socket_desc = list_desc[i];

            break;
        }
    }

    //If the socket descriptor has a value of -1, return -1
    if (socket_desc == -1)
    {
        return -1;
    }
    else
    {
        /**
         * Return the value of the socket descriptor by accepting the connect request of the client 
         * which is ready to connect with the Central Server
         */
        return accept(socket_desc, address, address_length);
    }
}

int findPortForDevice(string device)
{
	if (strcmp(device.c_str(), CLIENT_A) == 0)
	{
		return CLIENT_A_TCP_PORT;	
	}
	else if (strcmp(device.c_str(), CLIENT_B) == 0)
	{
		return CLIENT_B_TCP_PORT;	
	}
	else if (strcmp(device.c_str(), SERVER_A) == 0)
	{
		return SERVER_A_UDP_PORT;	
	}
	else if (strcmp(device.c_str(), SERVER_B) == 0)
	{
		return SERVER_B_UDP_PORT;	
	}
	else if (strcmp(device.c_str(), SERVER_C) == 0)
	{
		return SERVER_C_UDP_PORT;	
	}
	else if (strcmp(device.c_str(), SERVER_M) == 0)
	{
		return SERVER_M_UDP_PORT;	
	}
	else
	{
		return -1;
	}
}

string findDeviceForAddress(sockaddr_in socket_address)
{
	if (ntohs(socket_address.sin_port)==CLIENT_A_TCP_PORT)
	{
		return CLIENT_A;
	}
	else if (ntohs(socket_address.sin_port)==CLIENT_B_TCP_PORT)
	{
		return CLIENT_B;
	}
}

string chooseDevice()
{
	int pick = rand() % 3 + 1;
	// cout << "pick: " << pick << endl;
	if (pick==1)
	{
		return SERVER_A;
	}
	else if (pick==2)
	{
		return SERVER_B;
	}
	else if (pick==3)
	{
		return SERVER_C;
	}
}