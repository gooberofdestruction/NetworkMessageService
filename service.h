#ifndef SERVICE_08222011
#define SERVICE_08222011
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
#include "ipAddress.h"
#include "server_client_common.h"
#include "server_client_DLL.h"

//server class
class service
{
private:

	IpAddress address;

	//tell server to stop
	bool stop;
	bool network_connect;

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
	runDllThreadArgs*		dllArgs;

public:
	service(){};
	~service(){};//make virtual

	bool setupDll(std::string dllName);

	bool addRecvConnection(SOCKET* connection);
	bool addSendConnection(SOCKET* connection);
	void startSendService();
	bool setupConnections();

	void destroyData();
	void quit();

	void stopServices();
	void startServices();
	void connectServices();
	void disconnectServices();
	
	bool isRunning();
	bool isConnected();
	
	bool dllHasCmd();
	CmdMessage* dllGetNextCmd();

	void setAddress(std::string ip, std::string port);
	void setAddress(RAW_IP ip, RAW_PORT port);
	std::string getAddress();
	std::string getPort();

	WSAData* getWsa();

	virtual void run() = 0;
};

#endif