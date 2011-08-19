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
#include "server.h"

//-----------------------------------
//Namespaces
//-----------------------------------
using namespace std;

server::server(string dllName)
{
	int error_result=0;

	//load dll
	dllHandle = loadDll(dllName.c_str());
	connect = true;
	if(dllHandle == NULL)
	{
		cout<<"Unable to load DLL at "<< dllName.c_str() <<endl;
		stop = true;
		return;
	}
	else
	{
		stop = false;
	}

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

	recvArgs	= new list<recvThreadArgs*>();
	sendArgs	= new sendThreadArgs();
	listenArgs	= new listenThreadArgs();
	dllArgs     = new runDllThreadArgs();

	sendArgs->pollQueue = hasMessagesToSend;
	sendArgs->nextMessage = getNextSend;
	sendArgs->instance = dllInstance;
	sendArgs->mutex = CreateMutex (NULL, FALSE, NULL);
	sendArgs->socketList = new vector<SOCKET>();
	sendArgs->stop = &connect;
	
	port = CONNECTION_PORT;

	listenArgs->stop = &connect;
	listenArgs->listen_sock = NULL;
	listenArgs->recvArgs = recvArgs;
	listenArgs->sendArgs = sendArgs;
	listenArgs->instance = dllInstance;
	listenArgs->messageHandler = messageHandler;

	dllArgs->stop = &stop;
	dllArgs->instance = dllInstance;
	dllArgs->runInstance = runInstance;
	dllArgs->stopInstance = stopInstance;

	cout<<"Starting WSA..."<<endl;

	//--------------------
	// Start WSA
	//--------------------
	error_result = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (error_result != 0) 
	{
        cout<<"WSAStartup failed: "<<error_result<<endl;
    }

	CreateThread(NULL,NULL,run_dll, dllArgs,NULL,NULL);
}
server::~server()
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

void server::quit()
{
	stop = true;
	connect = true;
}

void server::run()
{
	//-----------------------------------------------------------------
	//Variables
	//-----------------------------------------------------------------

	//buffer to read to
	CmdMessage* message;

	//hold boolean value of poll functon
	bool poll = true;

	int time_out=GIVE_UP_REMAINING_THREAD_TIME;

	//-----------------------------------------------------------------
	//While socket is still open, change to still have valid sockets
	//-----------------------------------------------------------------
	while( !stop )
	{
		printf("Waiting...\n");
		//poll the dll for a message
		poll = hasCmd(dllInstance);

		//-----------------------------------------------------------------
		//While socket is still open and there are messages to send. change to still have valid sockets to send on
		//-----------------------------------------------------------------
		if(!stop && poll ) 
		{
			message = getNextCmd(dllInstance);

			switch(message->msg)
			{
				case CMD_STOP:
					printf("STOP!\n");
					stop = true;
				break;
				case CMD_CONNECT:
					printf("CONNECT!\n");
					connect = false;
					//start sending on valid connections
					CreateThread(NULL,NULL,send_service_connection, sendArgs,NULL,NULL);

					//start listening
					CreateThread(NULL,NULL,listenForConnections, listenArgs,NULL,NULL); 
				break;
				case CMD_DISCONNECT:
					printf("DISCONNECT!\n");
					connect = true;
				break;
				case CMD_NETWORK:
					address = dotFormatAddress(message->data.network.address);
					port = ""+message->data.network.port;
					printf("NETWORK!\n Addr: %s\n Port: %s\n",address.c_str(),port.c_str());
				break;
				default:
					printf("STOP!\n");
					stop = true;
				break;
			};

			//clean up message
			delete message;
		}
		
		Sleep(time_out);
	}//END OF while(bytes_sent!=SOCKET_ERROR)
	
	//-----------------------------------------------------------------
	//Clean up
	//-----------------------------------------------------------------
}
//***********************************
//		RUN SERVER
//***********************************
DWORD WINAPI listenForConnections(LPVOID args)
//int server::listenForConnections(void)
{
	//--------------------
	//Variables
	//--------------------
	SOCKET connection_sock=INVALID_SOCKET;
	listenThreadArgs* listenArgs = ((listenThreadArgs*)args);

	bool hasListenSocket = false;

	int error_result=0;

	fd_set my_set;
	timeval* poll=NULL;

	hasListenSocket = startListenSocket(listenArgs);
	std::cout<<"CAN HAZ "<<hasListenSocket<<std::endl;
	std::cout<<"CAN RUN "<<!*(listenArgs->stop)<<std::endl;
	//--------------------
	//Run until told to stop
	//--------------------
	while(!*(listenArgs->stop) && hasListenSocket )
	{
		cout<<"Listening..."<<endl;
		//listen on listen socket
		error_result = listen(listenArgs->listen_sock, SOMAXCONN);
		if (error_result == SOCKET_ERROR) 
		{ 
			cout<<"listen failed: "<< WSAGetLastError()<<endl;;
			closesocket(listenArgs->listen_sock);
			hasListenSocket = startListenSocket(listenArgs);
			continue;
		}
		FD_ZERO(&my_set);
		FD_SET(listenArgs->listen_sock,&my_set);

		//poll listen socket
		select(NULL,&my_set,NULL,NULL,poll);

		//someone wants to connect
		if(FD_ISSET(listenArgs->listen_sock,&my_set)/*and I am still allowing connections*/)
		{
			//accept connection
			connection_sock = accept(listenArgs->listen_sock, NULL, NULL);

			//error check
			if (connection_sock != INVALID_SOCKET) 
			{ 
				cout<<"Connection accepted..."<<endl;

				//Get mutex to add the new connection to the list
				WaitForSingleObject( listenArgs->sendArgs->mutex, INFINITE );
				listenArgs->sendArgs->socketList->push_back(connection_sock);
				ReleaseMutex( listenArgs->sendArgs->mutex);

				listenArgs->recvArgs->push_back(new recvThreadArgs());

				listenArgs->recvArgs->back()->messageHandler = listenArgs->messageHandler;
				listenArgs->recvArgs->back()->instance = listenArgs->instance;
				listenArgs->recvArgs->back()->socket = connection_sock;
				listenArgs->recvArgs->back()->stop = listenArgs->stop;
				//if passed error check
				//start new thread and pass connected socket to thread
				CreateThread(NULL,NULL,recv_service_connection,listenArgs->recvArgs->back(),NULL,NULL); 
				
			}
			else
			{
				//reject connection
				//clean up WSA State
				cout<<"accept failed: "<<WSAGetLastError()<<endl;
			}//END OF if/else block

		}//END OF IF(FD_ISSET())					

	}//END OF while(!quit)

	//--------------------
	//	Clean Up
	//--------------------
	cout<<"Stop Listening..."<<endl;
	closesocket(listenArgs->listen_sock);
	
	return 0;
}//END OF RUN SERVER

//***********************************
//	START LISTENING SOCKET
//***********************************
bool startListenSocket(listenThreadArgs* listenArgs)
{
	struct addrinfo  listen_info;
	struct addrinfo *addr_info = NULL;

	int error_result=0;

	//--------------------
	// Setup and Start listen socket
	//--------------------
	//zero memory space
	ZeroMemory(&listen_info, sizeof(listen_info));

	//initial data
    listen_info.ai_family = AF_INET;
    listen_info.ai_socktype = SOCK_STREAM;
    listen_info.ai_protocol = IPPROTO_TCP;
    listen_info.ai_flags = AI_PASSIVE;

	cout<<"Getting address info..."<<endl;

    // getting address info
    error_result = getaddrinfo(NULL, (PCSTR)CONNECTION_PORT, &listen_info, &addr_info);
    if ( error_result != 0 ) 
	{
        cout<<"getaddrinfo failed: "<<error_result<<endl;
		return false;
    }

	cout<<"Creating listen socket..."<<endl;

    // Create a SOCKET for connecting to server
    listenArgs->listen_sock = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);

    if (listenArgs->listen_sock == INVALID_SOCKET) 
	{
        cout<<"socket failed: "<<WSAGetLastError()<<endl;;
        freeaddrinfo(addr_info);
		return false;
    }

	cout<<"Binding..."<<endl;

    // Setup the TCP listening socket
    error_result = bind( listenArgs->listen_sock, addr_info->ai_addr, (int)addr_info->ai_addrlen);

    if (error_result == SOCKET_ERROR) 
	{
        cout<<"bind failed: "<<WSAGetLastError()<<endl;
        freeaddrinfo(addr_info);
        closesocket(listenArgs->listen_sock);
		return false;
    }

    freeaddrinfo(addr_info);

	return true;
}//END OF START LISTENING