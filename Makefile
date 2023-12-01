
# Name: Vineet Agarwal
# USC ID: 6693187979
# Course Number: EE 450
# Project: Socket Programming Project, Spring 2022
 

#Using g++ to compile C++ code
CC = g++
CFLAGS = -std=c++11

#Compile all the files in the folder
all: serverA serverB serverC serverM clientA clientB

#Compile the Central Server
serverM: serverM.cpp 
	$(CC) $(CFLAGS) -o serverM serverM.cpp 
 
#Compile the Server S
serverA: serverA.cpp 
	$(CC) $(CFLAGS) -o serverA serverA.cpp 

#Compile the Server T
serverB: serverB.cpp 
	$(CC) $(CFLAGS) -o serverB serverB.cpp 

#Compile the Server P
serverC: serverC.cpp 
	$(CC) $(CFLAGS) -o serverC serverC.cpp

#Compile the Client A
clientA: clientA.cpp 
	$(CC) $(CFLAGS) -o clientA clientA.cpp 

#Compile the Client B
clientB: clientB.cpp
	$(CC) $(CFLAGS) -o clientB clientB.cpp 

