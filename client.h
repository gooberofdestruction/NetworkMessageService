#ifndef CLIENT_04262010
#define CLIENT_04262010
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

class client
{
private:
	bool stop;

	std::string port;
	std::string address;

	WSADATA wsaData;

	recvThreadArgs*		recvArgs;
	sendThreadArgs*		sendArgs;
	runDllThreadArgs*	dllArgs;

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

	bool closeSocket();
	bool connectToServer(std::string address);
public:
	client(std::string address, std::string dllName);
	~client();
	void run();
	void quit();
};
#endif