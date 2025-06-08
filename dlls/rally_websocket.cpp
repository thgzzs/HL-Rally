// Implementation of the ClientSocket class

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "rally_websocket.h"

// hero
#ifdef _WIN32
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "wsock32.lib")
#pragma warning (disable:4172)
#endif

bool Connected;

// hero
#ifdef _WIN32
SOCKET theSocket;
#else
int theSocket;
// for WORD
//#include "sqltypes.h"
typedef unsigned short WORD;
// for hostent
#include "netdb.h"
// for INVALID_SOCKET
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SOCKADDR_IN sockaddr_in
#define MAKEWORD(a, b) ((WORD) (((BYTE) (a) | ((WORD) (b))) << 8))
#define LPHOSTENT hostent*
#endif

std::string CRallySock :: Socket_Connect(const char *myBuf)
{
        if(strstr(myBuf, "wonid=-1") || strstr(myBuf, "loopback") || strstr(myBuf, "127.0.0.1"))
                return std::string("\n\nStats not recorded for local players...\n\n");
	WORD version = MAKEWORD(1,1);
	int nRet;


	ALERT(at_console, "\n\nCommunicating with HL Rally Master Servers...\n");


    // Get information about the server
    LPHOSTENT lpHostEntry;
 
    lpHostEntry = gethostbyname("www.hlrally.net");
    if (lpHostEntry == NULL) {
		return std::string("Could Not Connect To http://hlrally.net");
    }

	while(Connected) // Wait
	{
	}

	// Create client socket
	theSocket = socket(AF_INET,		        // Go over TCP/IP
                       SOCK_STREAM,	     	// Socket type
                       IPPROTO_TCP);	    // Protocol
	if (theSocket == INVALID_SOCKET) {
		return std::string("Error at socket()");
	}

	Connected = true;

	
	// Fill an address struct with the server's location
	SOCKADDR_IN saServer;
	
	saServer.sin_family = AF_INET;
// hero
#ifdef _WIN32
	saServer.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
                     // ^ Address of the server
#else
	saServer.sin_addr = *((struct in_addr *)lpHostEntry->h_addr);
#endif

	saServer.sin_port = htons(80);


	// Connect to the specified address and port
	nRet = connect(theSocket,
// hero
#ifdef _WIN32
                   (LPSOCKADDR)&saServer,		// Server address
#else
                   (sockaddr*)&saServer,		// Server address
#endif
                   sizeof(struct sockaddr));	// Length of address structure
    if (nRet == SOCKET_ERROR) {
	   return std::string("Error at connect()");
	}



	// Successfully connected!
	nRet = send(theSocket,		// Pretend this is connected
			myBuf,		// Our string buffer
			strlen(myBuf), 	// Length of the data in the buffer
			0);			// Most often is 0, but see end of tutorial for options
	if (nRet == SOCKET_ERROR) {
		return std::string("Error on send()");
	}

	// Strip HTTP Headers
	for (int i = 0; i < 6; i++)
	{
		Socket_ReadLn();
	}

	ALERT(at_console, Socket_ReadLn().c_str());
	ALERT(at_console, Socket_ReadLn().c_str());


	return std::string(); // No error
}




std::string CRallySock :: Socket_ReadLn()
{
    std::string result;
    char buffer[1];
    while (true) {
        int bytesReceived = recv(theSocket, buffer, 1, 0);
        if (bytesReceived <= 0)
            return std::string();
        result.push_back(buffer[0]);
        if (buffer[0] == '\n')
            return result;
    }
    return std::string();
}


void CRallySock :: SocketClose()
{
//hero
#ifdef _WIN32
	closesocket(theSocket);
#else
	close(theSocket);
#endif
	Connected = false;
	ALERT(at_console, "\n");
}

