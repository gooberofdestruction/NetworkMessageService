#ifndef SERVER_04262010
#define SERVER_04262010
//***********************************
//		HEADERS
//***********************************
//-----------------------------------
//Standard Headers
//-----------------------------------
#include <list>

//-----------------------------------
//Project Headers
//-----------------------------------
#include "service.h"

class server;
typedef struct listenThreadArgs
{
	server* instance;
	SOCKET listen_sock;
}listenThreadArgs;

//listen for clients to join
DWORD WINAPI listenForConnections(LPVOID args);

//creates a socket to listen on
bool startListenSocket(listenThreadArgs* listenArgs);

//server class
class server : public service
{
private:
	listenThreadArgs* listenArgs;
public:
	server(std::string dllName, std::string address);
	~server();

	//start the dll
	void run();
	
	//tell the server to stop
};
//execute dll

#endif