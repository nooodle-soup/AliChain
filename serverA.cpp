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
#include <fstream>
#include <unordered_map>
#include <vector>
#include <iterator>
#include <utility>
#include <sstream>

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

#define LOG_FILE "block1.txt"

using namespace std;

unordered_map<string, string> coinsFrom, coinsTo;
unordered_map<string, int> coinCount;

/*Sends logs to destination socket with the destination address*/
void sendLogs(int destination_socket, sockaddr_in destination_addr);

/*Checks if the user is in the logs or not*/
bool checkUserInLogs(char* user);

/*Checks if the users are present in the log file in this server. Returns status messages for different conditions*/
void checkUserRequest(char* user1, char* user2, int destination_socket, sockaddr_in destination_addr);

/*Sends the logs pertaining to a particular user to the main server if any*/
void checkWallet_onBackend(char* user, int destination_socket, sockaddr_in destination_addr);

/*Gets logs for the specified user if forUser is true else returns all logs in the log file*/
vector<string> getLogs(bool forUser=false, char* user=NULL);

/*Writes the log to the log file on the backend server*/
void logTransactionToFile(char* singleLog, int destination_socket, sockaddr_in destination_addr);

/*Sends an integer data value to the destination socket with the destination address*/
void sendInt(int num, int destination_socket, sockaddr_in destination_addr);

/*Sends an character* data to the destination socket with the destination address*/
void sendChar(char* message, int destination_socket, sockaddr_in destination_addr);

/*Prints out the error and terminates the server*/
void error(const char* message);

/*Creates a UDP socket for the mentioned device*/
int createSocketUDP(string device);

/*Sets the socket address for the mentioned port*/
sockaddr_in setSockAddr(string whosePort);

/*Binds the socket with the clientSocket descriptor and socketAddr address */
void bindSocket(int clientSocket, sockaddr_in sockAddr, string device);

int main() {

    // open file
    ifstream myfile(LOG_FILE);
    

    if(!myfile.is_open()){
    	cout << "Unable to open file on backend server A" << endl;
    }   
    string singleLog;    
        
    while (getline(myfile,singleLog)){

        vector<string> transactionData;

        // split lines by " "
        char* token = strtok(const_cast<char*>(singleLog.c_str()), " ");
        while(token != NULL){
        	// cout << token << endl;
            transactionData.push_back(string(token));
            token = strtok(NULL, " ");
        }

        // add transaction to maps
        pair<string, string> from = make_pair(transactionData[0],transactionData[1]);
  		pair<string, string> to = make_pair(transactionData[0],transactionData[2]);
  		pair<string, int> amt = make_pair(transactionData[0], stoi(transactionData[3]));

        coinsFrom.insert(from);
        coinsTo.insert(to);
        coinCount.insert(amt);
    }
	myfile.close();

    // create a server SOCKET for server A and server M
    int serverA_socket = createSocketUDP(SERVER_A);    
	int serverM_socket = createSocketUDP(SERVER_M);  

    // set server A and M addresses
    struct sockaddr_in serverA_addr = setSockAddr(SERVER_A);
    struct sockaddr_in serverM_addr = setSockAddr(SERVER_M);

   	bindSocket(serverA_socket, serverA_addr, SERVER_A);
   	// bindSocket(serverM_socket, serverM_addr, "Server M");

    // bootup message for server A
    cout<<"The server A is up and running."<<endl;

    //Get the length of the socket addresses for the Central Server and Server P
    socklen_t serverA_len = sizeof(serverA_addr);
    socklen_t serverM_len = sizeof(serverM_addr);

    while(1)
    {

    	char* sendMessage;
        // get data query from main server
        char command[MAX_DATA_SIZE];
        int recv_status = recvfrom(serverA_socket, &command, sizeof(command), 0, (struct sockaddr *) &serverM_addr, &serverM_len);
        // cout << command << endl;
        if(recv_status == -1)
        {
            error("Error receiving data from main server!");
        }

        string messageFromMainServer = command;
        cout << "The ServerA received a request from the Main Server." << endl;
        // cout<<command<<endl;
        char* splitCommand[5];
	    splitCommand[0] = strtok(command, " "); // Splits spaces between words in str
	    splitCommand[1] = strtok (NULL, " ");
	    splitCommand[2] = strtok (NULL, " ");
	    splitCommand[3] = strtok (NULL, " ");
	    // cout << "DEBUG-outside if "  << endl;//<< splitCommand[1] << splitCommand[2] << splitCommand[3]
        // process query
        
        // CHECK WALLET REQUEST
        if(strcmp(splitCommand[0],"1")==0)
        {

        	// cout << "DEBUG-inside checkWallet_onBackend" << endl;
	        
	        checkWallet_onBackend(splitCommand[1], serverM_socket, serverM_addr);
        }
        // LOG TRANSACTION TO FILE REQUEST
        else if(strcmp(splitCommand[0],"2")==0) 
        {	
        	// cout << "DEBUG-inside txCoins_onBackend" << endl;
        	int justBeforeCommand = messageFromMainServer.find(" ");
        	string logData = messageFromMainServer.substr(justBeforeCommand+1, messageFromMainServer.size()-justBeforeCommand-1);

        	char* log = new char[MAX_DATA_SIZE]; 
        	strcpy(log, logData.c_str());
        	logTransactionToFile(log, serverM_socket, serverM_addr);
	    }
	    // SEND LOGS REQUEST
	    else if(strcmp(splitCommand[0],"3")==0)
	    {
	    	sendLogs(serverM_socket, serverM_addr);
	    }  
	    // CHECK USER EXIST REQUEST
	    else if(strcmp(splitCommand[0],"5")==0)
	    {	
	    	// cout << "DEBUG-inside checkUser_onBackend" << endl;
	    	int firstSpaceIndex = messageFromMainServer.find(" ");
	    	string data = messageFromMainServer.substr(firstSpaceIndex + 1, messageFromMainServer.size()-firstSpaceIndex-1);
            int spaceIndex = data.find(" ");
            char* userFrom = new char[MAX_DATA_SIZE];
            char* userTo = new char[MAX_DATA_SIZE];
            
            if(spaceIndex!=-1){
                strcpy(userFrom, data.substr(0, spaceIndex).c_str());
                strcpy(userTo, data.substr(spaceIndex+1, data.size()-spaceIndex-1).c_str());
            }
            else{
                strcpy(userFrom, data.c_str());
                strcpy(userTo, "");
            }
	    	checkUserRequest(userFrom, userTo, serverM_socket, serverM_addr);
	    }

        cout << "The ServerA finished sending the response to the the Main Server." << endl;

    }

    return 0;

}


void sendLogs(int destination_socket, sockaddr_in destination_addr)
{
	// cout << "in check wallet request" << endl;
	// get all logs
    vector<string> allLogs = getLogs();    

    // find the size of logs and send it to the server
    int numLogs = allLogs.size();
    sendInt(numLogs, destination_socket, destination_addr);

    // send each log to the main server one by one
    for(int i=0; i < numLogs; i++){

        char* message = new char[MAX_DATA_SIZE];
        strcpy(message, allLogs[i].c_str());
        sendChar(message, destination_socket, destination_addr);
        delete[](message);
    }
}

bool checkUserInLogs(char* user)
{	
	// cout << "checking user: " << user << "." << endl;
	// if case of check wallet, user can be "", return true
	if (strcmp(user, "") == 0)
	{
		return true;
	}

	// create iterators to iterate over the global maps
	unordered_map<string,string>::iterator iteratorFrom = coinsFrom.begin();
    unordered_map<string,string>::iterator iteratorTo = coinsTo.begin();

    // if user is in the from map, return true
	while (iteratorFrom != coinsFrom.end())
	{
		// cout << "in while from" << endl;
		if (strcmp(iteratorFrom->second.c_str(), user)==0)
		{
			// cout << "user found in from" << endl;
			return true;
		}

		iteratorFrom++;
	}

	// if user is in the to map, return true
	while (iteratorTo != coinsTo.end())
	{
		// cout << "in while to" << endl;
		if (strcmp(iteratorTo->second.c_str(), user)==0)
		{
			// cout << "user found in true" << endl;
			return true;
		}
		iteratorTo++;
	}

	// if user does not exist in either map, it does not exist in this server, return false
	return false;
}

void checkUserRequest(char* user1, char* user2, int destination_socket, sockaddr_in destination_addr)
{   
    // cout << "in check user request" << endl;
    
    // check if users exist in the logs
    // values will be true if they exist
    bool user1Exists = checkUserInLogs(user1);
    bool user2Exists = checkUserInLogs(user2);

    // cout << user1Exists << " " << user2Exists << endl;
    
    char* message = new char[MAX_DATA_SIZE];
    // if both users exist
    if (user1Exists && user2Exists) 
    {
        strcpy(message, "BOTH_USERS_EXIST");
    }
    // if only user2 exists
    else if (!user1Exists && user2Exists)
    {
        strcpy(message, "USER_1_MISSING");
    }
    // if only user1 exists
    else if (!user2Exists && user1Exists)
    {
        strcpy(message, "USER_2_MISSING");
    }   
    // if neither user exists
    else
    {
        strcpy(message, "NO_USERS");
    }
    // cout << "user missing" << message << endl;

    // send message to main server
    sendChar(message, destination_socket, destination_addr);

    // delete message
    delete[](message);
}

void checkWallet_onBackend(char* user, int destination_socket, sockaddr_in destination_addr)
{
	// cout << "in check wallet request" << endl;
	
	// get all logs
    vector<string> userLogs = getLogs(true, user);    

    // cout << "received logs" << endl;

    // find the size of logs and send it to the server
    int numLogs = userLogs.size();
    sendInt(numLogs, destination_socket, destination_addr);

    // send each log to the main server one by one
    for(int i=0; i < numLogs; i++){
    
        char* message = new char[MAX_DATA_SIZE];
        strcpy(message, userLogs[i].c_str());
        sendChar(message, destination_socket, destination_addr);
        delete[](message);
    }
}

vector<string> getLogs(bool forUser, char* user)
{
	// cout << "in get logs" << endl;

	// create a vector to store transactions
	vector<string> transactions;

	// create ifstream object to read from file
	ifstream logFile(LOG_FILE);
    
    if(!logFile.is_open()){
    	cout << "Unable to open file on backend server A" << endl;
    }  
    else
    {	
    	// do nothing
    	// cout<< "opened file" << endl;
    } 
    string singleLog;
     
    // read from file and process data   
    while(getline(logFile,singleLog))
    {	
    	// if logs are to be retrieved for check wallet operation for a single user
        if (forUser)
        {
        	vector<string> splitLog;

	        stringstream str_stream(singleLog);
	        string word;
	        
	        // get all the words in the line (TX_ID, FROM, TO, TX_AMT)
	        while(getline(str_stream, word, ' '))
	        {
	            splitLog.push_back(word);
	        }
	        
	        // check transaction involves user
	        if(strcmp(splitLog[1].c_str(),user)==0 || strcmp(splitLog[2].c_str(),user)==0)
	        {
	            transactions.push_back(singleLog);
	        }
        }
        // if all logs are to be retrieved
        else
        {
        	transactions.push_back(singleLog);
        }
    }
    // close file
    logFile.close();

    // return transactions
    return transactions;
}

void logTransactionToFile(char* singleLog, int destination_socket, sockaddr_in destination_addr)
{
    // open logfile in append mode
    ofstream logFile(LOG_FILE, ios::app);
    logFile << singleLog << endl;
    logFile.close();

    // create vector to store transaction data (ID, user1, user2, txAmount)
    vector<string> transactionData;

    // split lines by " " and store (ID, user1, user2, txAmount)
    char* token = strtok(singleLog, " ");
    while(token != NULL){
        transactionData.push_back(string(token));
        token = strtok(NULL, " ");
    }

    // add transaction data to global maps
    pair<string, string> from = make_pair(transactionData[0],transactionData[1]);
    pair<string, string> to = make_pair(transactionData[0],transactionData[2]);
    pair<string, int> amt = make_pair(transactionData[0], stoi(transactionData[3]));

    coinsFrom.insert(from);
    coinsTo.insert(to);
    coinCount.insert(amt);

    // result message to be sent back to main server
    char *message = new char[MAX_DATA_SIZE];
    strcpy(message, "SUCCESS");

    // send transaction logging status to main server
    sendChar(message, destination_socket, destination_addr);

    // display message on sending data to the main server
    cout<<"The Server A finished sending the response to the Main Server."<<endl;

    delete[](message);
}

void sendInt(int num, int destination_socket, sockaddr_in destination_addr)
{
	int sentBytes = sendto(destination_socket, &num, sizeof(num) + 1, 0, (struct sockaddr *) &destination_addr, (socklen_t) sizeof(destination_addr));
	if(sentBytes == -1)
	{
        error("Error sending data to main server!");
    }
}

void sendChar(char* message, int destination_socket, sockaddr_in destination_addr)
{
	// cout << "Sending char message" << endl;
	int sentBytes = sendto(destination_socket, message, MAX_DATA_SIZE + 1, 0, (struct sockaddr *) &destination_addr, (socklen_t) sizeof(destination_addr));
	if(sentBytes == -1)
	{
    	error("Error sending data to main server!");
    }
}

void error(const char* message)
{
    perror(message);
    exit(1);
}

int createSocketUDP(string device)
{
	// Create a socket for Client <--> Main Server commmunications
	int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
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
	else if (strcmp(whosePort.c_str(), "Server M") == 0)
	{
		sockAddr.sin_port = htons(SERVER_M_UDP_PORT);	
	}

	return sockAddr;
}

void bindSocket(int clientSocket, sockaddr_in sockAddr, string device)
{
	if (bind(clientSocket, (struct sockaddr*) &sockAddr, sizeof(sockAddr)) == -1)
	{
		string errorString = "Error binding socket for " + device + "...";
		error(errorString.c_str());
	}
}