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
#include "server_client_common.h"
#include "server_client_DLL.h"

typedef struct listenThreadArgs
{
	bool	*stop;
	SOCKET	listen_sock;
	void*	instance;
	void	(*messageHandler) (void* , char*, int);
	sendThreadArgs* sendArgs;
	std::list<recvThreadArgs*> *recvArgs;
}listenThreadArgs;

//listen for clients to join
DWORD WINAPI listenForConnections(LPVOID args);

//creates a socket to listen on
bool startListenSocket(listenThreadArgs* listenArgs);

//server class
class server
{
private:

	std::string port;
	std::string address;

	//tell server to stop
	bool stop;
	bool connect;

	//windows data structure
	WSAData wsa_data;

	//dll file handle
	HINSTANCE dllHandle;

	//dll instance
	void*	dllInstance;

	//dll functions
	HASMSGFUNC	hasMessagesToSend;
	NEXTMSG		getNextSend;
	MSGHANDLER	messageHandler;
	CREATE		createInstance;
	DESTROY		destroyInstance;
	RUN			runInstance;
	STOP		stopInstance;
	HASCMDFUNC	hasCmd;
	NEXTCMD		getNextCmd;

	std::list<recvThreadArgs*>	*recvArgs;
	sendThreadArgs*			sendArgs;
	listenThreadArgs*		listenArgs;
	runDllThreadArgs*		dllArgs;

public:
	server(std::string dllName);
	~server();

	//start the dll
	void run();
	
	//tell the server to stop
	void quit();

};
//execute dll
DWORD WINAPI server_rund_dll(LPVOID args);
#endif