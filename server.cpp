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

server::server(string dllName, std::string address)
{
	
	if(setupDll(dllName) && setupConnections())
	{
		startServices();
	}
	else
	{
		stopServices();
	}
	listenArgs = new listenThreadArgs();
	
	listenArgs->instance = this;
	listenArgs->listen_sock = NULL;

}
server::~server()
{
	destroyData();
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
	while( isRunning() )
	{
		printf("Waiting...\n");
		//poll the dll for a message
		poll = dllHasCmd();

		//-----------------------------------------------------------------
		//While socket is still open and there are messages to send. change to still have valid sockets to send on
		//-----------------------------------------------------------------
		if( isRunning() && poll ) 
		{
			message = dllGetNextCmd();

			switch(message->msg)
			{
				case CMD_STOP:
					printf("STOP!\n");
					stopServices();
				break;
				case CMD_CONNECT:
					printf("CONNECT!\n");
					connectServices();
					//start sending on valid connections
					startSendService();

					//start listening
					CreateThread(NULL,NULL,listenForConnections, listenArgs,NULL,NULL); 
				break;
				case CMD_DISCONNECT:
					printf("DISCONNECT!\n");
					disconnectServices();
				break;
				case CMD_NETWORK:
					printf("NETWORK!\n Addr: %d\n Port: %d\n", message->data.network.address, message->data.network.port);
					setAddress(message->data.network.address,message->data.network.port);
				break;
				default:
					printf("UNRECOGNIZED COMMAND - STOP!\n");
					stopServices();
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
	//--------------------
	//Run until told to stop
	//--------------------
	while( listenArgs->instance->isRunning() && hasListenSocket )
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

				listenArgs->instance->addSendConnection(&connection_sock);
				listenArgs->instance->addRecvConnection(&connection_sock); 				
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

