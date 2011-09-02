//***********************************
//		HEADERS
//***********************************
//-----------------------------------
//Standard Headers
//-----------------------------------
#include <iostream>
#include <conio.h>

//-----------------------------------
//Project Headers
//-----------------------------------
#include "service.h"
//-----------------------------------
//Namespaces
//-----------------------------------
using namespace std;

bool service::setupDll(string dllName)
{
	//load dll
	dllHandle = loadDll(dllName.c_str());
	disconnectServices();
	if(dllHandle == NULL)
	{
		cout<<"Unable to load DLL at "<< dllName.c_str() <<endl;
		return false;
	}
	startServices();//any place better?

	//dll functions
	hasMessagesToSend = (HASMSGFUNC)loadFunction(dllHandle, "hasMessagesToSend");
	getNextSend = (NEXTMSG)loadFunction(dllHandle, "getNextSend");
	messageHandler = (MSGHANDLER)loadFunction(dllHandle, "messageHandler");
	createInstance = (CREATE)loadFunction(dllHandle, "createInstance");
	destroyInstance = (DESTROY)loadFunction(dllHandle, "destroyInstance");
	runInstance = (RUN)loadFunction(dllHandle, "runInstance");
	stopInstance = (STOP)loadFunction(dllHandle, "stopInstance");
	hasCmd = (HASCMDFUNC)loadFunction(dllHandle, "hasCmdMesssages");
	getNextCmd = (NEXTCMD)loadFunction(dllHandle, "getNextCmd");
	
	//create an instance
	createInstance(&dllInstance);

	dllArgs     = new runDllThreadArgs();
	
	dllArgs->stop = &stop;
	dllArgs->instance = dllInstance;
	dllArgs->runInstance = runInstance;
	dllArgs->stopInstance = stopInstance;

	CreateThread(NULL,NULL,run_dll, dllArgs,NULL,NULL);
	
	return true;
}

bool service::setupConnections()
{
	recvArgs	= new list<recvThreadArgs*>();
	sendArgs	= new sendThreadArgs();

	sendArgs->pollQueue = hasMessagesToSend;
	sendArgs->nextMessage = getNextSend;
	sendArgs->instance = dllInstance;
	sendArgs->mutex = CreateMutex (NULL, FALSE, NULL);
	sendArgs->socketList = new vector<SOCKET>();
	sendArgs->stop = &network_connect;
	
	address.setPort(CONNECTION_PORT);

	cout<<"Starting WSA..."<<endl;

	//--------------------
	// Start WSA
	//--------------------
	int error_result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (error_result != 0) 
	{
        cout<<"WSAStartup failed: "<<error_result<<endl;
		return false;
    }

	return true;
}

bool service::addRecvConnection(SOCKET* connection)
{
	recvArgs->push_back(new recvThreadArgs());

	recvArgs->back()->messageHandler = messageHandler;
	recvArgs->back()->instance = dllInstance;
	recvArgs->back()->socket = *connection;
	recvArgs->back()->stop = &stop;

	CreateThread(NULL,NULL,recv_service_connection,recvArgs->back(),NULL,NULL);
	return true;
}

void service::quit()
{
	stopServices();
	disconnectServices();
}

void service::destroyData()
{
	int iResult=0;

	quit();

	// shutdown the connection since no more data will be sent
	WaitForSingleObject( sendArgs->mutex, INFINITE );
	while(sendArgs->socketList->begin() != sendArgs->socketList->end())
	{
		iResult = shutdown(*sendArgs->socketList->begin(), SD_SEND);
		if (iResult == SOCKET_ERROR) 
		{
			cout<<"shutdown failed: "<< WSAGetLastError()<<endl;
			closesocket(*sendArgs->socketList->begin());
		}

		// cleanup
		closesocket(*sendArgs->socketList->begin());

		sendArgs->socketList->erase(sendArgs->socketList->begin());
	}
	
	ReleaseMutex( sendArgs->mutex);
	delete sendArgs;

	//
	while(recvArgs->begin() != recvArgs->end())
	{
		delete *recvArgs->begin();
		recvArgs->erase(recvArgs->begin());
	}
	delete recvArgs;

	//
	if(dllHandle != NULL)
	{
		//
		destroyInstance(dllInstance);

		//destroy the server
		unloadDll(dllHandle);
	}
	delete dllInstance;

	WSACleanup();
}

bool service::addSendConnection(SOCKET* connection)
{
	WaitForSingleObject( sendArgs->mutex, INFINITE );
	sendArgs->socketList->push_back(*connection);
	ReleaseMutex( sendArgs->mutex);
	return true;
}

void service::startSendService()
{
	CreateThread(NULL,NULL,send_service_connection, sendArgs,NULL,NULL);
}

void service::stopServices()
{
	stop = true;
}

void service::startServices()
{
	stop = false;
}

void service::connectServices()
{
	network_connect = false;
}

void service::disconnectServices()
{
	network_connect = true;
}

bool service::isRunning()
{
	return stop == false;
}
bool service::isConnected()
{
	return connect == false;
}

bool service::dllHasCmd()
{
	return hasCmd(dllInstance);
}

CmdMessage* service::dllGetNextCmd()
{
	return getNextCmd(dllInstance);
}

void service::setAddress(string ip, string port)
{
	address = IpAddress(ip, port);
}

void service::setAddress(RAW_IP ip, RAW_PORT port)
{
	address = IpAddress(ip, port);
}

string service::getAddress()
{
	return address.getDotFormattedIp();
}
string service::getPort()
{
	return address.getStringPort();
}

WSAData* service::getWsa()
{
	return &wsa_data;
}