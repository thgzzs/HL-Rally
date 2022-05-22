// hero
#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

class CRallySock {
public:
	char *Socket_Connect(char myBuf[256]);
	char *Socket_ReadLn();
	char *Socket_SendMessage(char myBuf[256]);
	void SocketClose(void);
private:
	int nRet;
};
