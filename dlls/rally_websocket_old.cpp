// Implementation of the ClientSocket class

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "rally_websocket.h"

#pragma comment (lib, "wsock32.lib")

extern int CountPlayers();

char *CRallySock :: Socket_Connect(void)
{
	WORD version = MAKEWORD(1,1);
	WSADATA wsaData;
	int nRet;

	SOCKET theSocket;

	ALERT(at_console, "\n\nConnecting to HL Rally Master Servers...\n");

	// Init Winsock
	WSAStartup(version, &wsaData);


    // Get information about the server
    LPHOSTENT lpHostEntry;
 
    lpHostEntry = gethostbyname("www.hlrally.net");
    if (lpHostEntry == NULL) {
		return "Could Not Connect To http://hlrally.net";
    }

	ALERT(at_console, "Server Resolved\n");

	// Create client socket
	theSocket = socket(AF_INET,		        // Go over TCP/IP
                       SOCK_STREAM,	     	// Socket type
                       IPPROTO_TCP);	    // Protocol
	if (theSocket == INVALID_SOCKET) {
		return "Error at socket()";
	}

	
	// Fill an address struct with the server's location
	SOCKADDR_IN saServer;
	
	saServer.sin_family = AF_INET;
	saServer.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
                     // ^ Address of the server
	saServer.sin_port = htons(80);


	// Connect to the specified address and port
	nRet = connect(theSocket,
                   (LPSOCKADDR)&saServer,		// Server address
                   sizeof(struct sockaddr));	// Length of address structure
    if (nRet == SOCKET_ERROR) {
	   return "Error at connect()";
	}


	// Successfully connected!

	ALERT(at_console, "Connection Successful\n");

	char myBuf[256];

	sprintf(myBuf, "GET /servers/record.php?sn=%s&plr=%i&plrmax=%i HTTP/1.1\nHost: www.hlrally.net\nUser-Agent: HL RALLY\n\n", CVAR_GET_STRING("hostname"), CountPlayers(), gpGlobals->maxClients );
	nRet = send(theSocket,		// Pretend this is connected
			myBuf,		// Our string buffer
			strlen(myBuf), 	// Length of the data in the buffer
			0);			// Most often is 0, but see end of tutorial for options
	if (nRet == SOCKET_ERROR) {
		return "Error on send()";
	}

	for (int i = 0; i < 7; i++)
	{
		Socket_ReadLn(theSocket);
	}

	ALERT(at_console,  Socket_ReadLn(theSocket));



	
	closesocket(theSocket);

	// Shutdown Winsock
	WSACleanup();

	return "\nCONNECTION TO HALF-LIFE RALLY MASTER SERVERS SUCCESSFUL!\n\n";
}


char *CRallySock :: Socket_ReadLn(SOCKET theSocket)
{
	char result[256];
	memset (result, 0, 256);		// Empty the string

	char buffer[1];
	int bytesReceived;

	int len = 0;

	while (true) {
		bytesReceived = recv(theSocket, buffer, 1, 0);

		if (bytesReceived <= 0)
			return "ERROR IN TRANSFER\n";

		result[len] = buffer[0];
		len++;

		if (buffer[0] == '\n')
		{
		//	ALERT(at_console, result);
			return result;
		}
	}
}