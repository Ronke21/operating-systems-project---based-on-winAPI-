/*
Author: Ron Keinan
ID: 203735857

This code is the client.
it opens a socket, get an input ommand from user, sends to server and print the answer.

*/

#define WIN32_LEAN_AND_MEAN //prevents the Winsock.h from being included by the Windows.h header
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>  //winAPI funcs
#include <winsock2.h> //contains most of the Winsock functions, structures, and definitions.
#include <ws2tcpip.h> //contains definitions introduced in the WinSock 2 Protocol-Specific Annex document for TCP/IP that includes newer functions and structures used to retrieve IP addresses
#include <stdio.h>    //standard input and output
#include <iostream>   //input and output
#include <fstream>    //for files

using namespace std;


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib") //Winsock Library file
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512  //size of buffer for sending and receiving
#define DEFAULT_PORT "2000" //server and client must connect to the same port number
#define UPDATED_VERSION_PATH "C:\\Users\\ronke\\source\\repos\\new_server.exe" //path to updted version - send to server
#define LOOPBACK_ADDRESS "127.0.0.1" //inner computer IP address


/// <summary>
/// this function handels an UPDATE request.
/// takes the new version of server app from a defined path and sends through socket.
/// </summary>
/// <param name="fp">pointer to file of new version</param>
/// <param name="sockfd">socket identifier</param>
void send_file(FILE* fp, int sockfd)
{
	int size;
	char* rbuffer;          //will save file content here

	ifstream file;
	file.open(UPDATED_VERSION_PATH, ios::in | ios::binary | ios::ate);      //open file

	if (file.is_open())
	{
		file.seekg(0, ios::end);
		size = file.tellg();    //file size!

		file.seekg(0, ios::beg);        //sets location back to beginning of file

		rbuffer = new char[size];
		file.read(rbuffer, size);       //write file to buffer


		int check = send(sockfd, rbuffer, size, NULL);    //send file to server

		if (check == SOCKET_ERROR)
		{
			printf("Error uploading file to server");
		}
	}
}


/// <summary>
/// This function establishes the connection to the server socket
/// </summary>
/// <param name="iResult">counter for checking</param>
/// <param name="ConnectSocket">socket to be connected to</param>
void connectionEstablishment(SOCKET& ConnectSocket)
{
	int iResult;

	struct addrinfo* result = NULL, //addrinfo struct hold host address information
		* ptr = NULL,
		hints;

	//set the socket type to be a stream socket for the TCP protocol
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;     //specify the IPv4 or IPv6 address family
	hints.ai_socktype = SOCK_STREAM; //specify a stream socket
	hints.ai_protocol = IPPROTO_TCP; //specify the TCP protocol

	// Resolve the server address and port. argv[1] is server name
	iResult = getaddrinfo(LOOPBACK_ADDRESS, DEFAULT_PORT, &hints, &result); //func that get IP according to server name
	if (iResult != 0) //check for errors
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup(); //terminates use of the Winsock 2 DLL (Ws2_32.dll)
		return;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		//Check for errors to ensure that the socket is a valid socket:
		if (ConnectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup(); //terminate the use of the WS2_32 DLL.
			return;
		}

		// Connect to server, passing the created socket and the sockaddr structure 
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) //Check for general errors
		{
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}  //try the next address returned by getaddrinfo if the connect call failed


	//free the resources returned by getaddrinfo
	freeaddrinfo(result);

}


/// <summary>
/// This function get an input from user and sends to the server, then receive an answer and prints it
/// </summary>
/// <param name="iResult">counter for checking</param>
/// <param name="ConnectSocket">socket to be connected to</param>
/// <returns>true if sending was good, else false</returns>
bool sendAndReceiveData(SOCKET& ConnectSocket)
{
	int iResult;

	//printf("\033[34m"); //text color blue

	//orders to user:
	printf("Welcome dear client! \nyou can choose one of the commands to the server:\n");
	printf("\tPING - for echo request\n");
	printf("\tRUN pathOfApplication - for activating an app in the server on a new process\n");
	printf("\tUPDATE - to send new version to server app\n");
	printf("\tVERSION - to get current version in the server app\n");
	printf("\tEXIT - for finish\n");

	CHAR cmd[DEFAULT_BUFLEN];
	printf("Please enter your command:\n");
	fgets(cmd, DEFAULT_BUFLEN, stdin); //get text including spaces

	//printf("\033[0m"); //text color default

	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN];

	// Send an initial buffer
	iResult = send(ConnectSocket, cmd, (int)strlen(cmd), 0); //return number of bytes sent
	if (iResult == SOCKET_ERROR) //check for errors in sending
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();    //terminates use of the Winsock 2 DLL (Ws2_32.dll)
		return false;
	}

	if (strcmp((char*)strtok((char*)cmd, "\n"), "UPDATE") == 0) //if input is UPDATE - go to func that sends a file
	{
		FILE* fp = fopen(UPDATED_VERSION_PATH, "r");
		send_file(fp, ConnectSocket);
	}

	if (strcmp((char*)strtok((char*)cmd, "\n"), "EXIT") == 0) //if input is EXIT - return false to finish receving
	{
		return false;
	}

	// Receive data
	iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0); //return number of bytes received
	if (iResult > 0)
	{
		//printf("\033[31m"); //red
		printf("------------------------------String received: %.*s------------------------------\n\n", iResult, recvbuf);
		//printf("\033[0m"); //default
	}
	else
	{
		printf("recv failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}


/// <summary>
/// function that close the connection to the server
/// </summary>
/// <param name="iResult">counter for checking</param>
/// <param name="ConnectSocket">socket to be connected to</param>
void closeConnection(int& iResult, SOCKET& ConnectSocket)
{
	// shutdown the connection for sending since no more data will be sent, allows the server to release some of the resources for this socket
	// the client can still use the ConnectSocket for receiving data
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) //check for errors in closing
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();    //terminates use of the Winsock 2 DLL (Ws2_32.dll)
		return;
	}

	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN];

	// Receive data until the server closes the connection
	do
	{
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0); //return number of bytes received
		if (iResult > 0)
		{
			//printf("\033[31m"); //red
			printf("------------------------------String received: %.*s------------------------------\n\n", iResult, recvbuf);
			//printf("\033[0m"); //default
		}
		else if (iResult == 0)
		{
			//printf("\033[33m"); //yellow
			printf("Connection closed\n");
			//printf("\033[0m"); //default
			return;
		}
		else
		{
			printf("recv failed with error: %d\n", WSAGetLastError());
			return;
		}

	} while (iResult > 0);
}

int main()
{
	WSADATA wsaData; //structure contains information about the Windows Sockets implementation.
	int iResult;

	//------------------------------CONNECTION ESTABLISHMENT------------------------------------------------
	// Initialize Winsock = initialize the use of the Windows Sockets DLL before making other Winsock functions calls
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); //called to initiate use of WS2_32.dll, version 2.2.
	if (iResult != 0)  //check success of loading winsock DLL
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	//Create a SOCKET object:
	SOCKET ConnectSocket = INVALID_SOCKET;
	connectionEstablishment(ConnectSocket);
	if (ConnectSocket == INVALID_SOCKET) //establishment failed - print an error message 
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		system("pause");
		return 0;
	}


	//------------------------------SENDING AND RECEIVING DATA------------------------------------------------
	bool cmd = false;
	do
	{
		cmd = sendAndReceiveData(ConnectSocket);

	} while (cmd == true);


	//------------------------------CLOSE CONNECTION------------------------------------------------
	closeConnection(iResult, ConnectSocket);
	// cleanup
	closesocket(ConnectSocket); //close the socket
	WSACleanup(); //release DLL resources


	system("pause");
	return 0;
}