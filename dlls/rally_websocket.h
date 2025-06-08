// hero
#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif
#include <string>

class CRallySock {
public:
        std::string Socket_Connect(const char *myBuf);
        std::string Socket_ReadLn();
        std::string Socket_SendMessage(const char *myBuf);
        void SocketClose(void);
private:
	int nRet;
};
