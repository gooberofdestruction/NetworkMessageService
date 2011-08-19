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
#include "client.h"

//-----------------------------------
//Namespaces
//-----------------------------------
using namespace std;

//***********************************
//		CREATE CLIENT
//***********************************
client::client(string p_address, string dllName)
{
	address = p_address;

	//init thread args to NULL
	recvArgs = new recvThreadArgs();
	sendArgs = new sendThreadArgs();
	sendArgs->socketList = new vector<SOCKET>();
	dllArgs = new runDllThreadArgs();
	//load dll
	dllHandle = loadDll(dllName.c_str());
	stop = false;
	if(dllHandle == NULL)
	{
		cout<<"Unable to load DLL at "<< dllName.c_str() <<endl;
		stop = true;
		return;
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
	hasCmd = (HASCMDFUNC)loadFunction(dllHandle, "hasCmdMesssages");
	getNextCmd = (NEXTCMD)loadFunction(dllHandle, "getNextCmd");

	//create an instance
	createInstance(&dllInstance);

	dllArgs->stop = &stop;
	dllArgs->instance = dllInstance;
	dllArgs->runInstance = runInstance;
	dllArgs->stopInstance = stopInstance;

	CreateThread(NULL,NULL,run_dll, dllArgs,NULL,NULL);

}//END OF CLIENT

//***********************************
//		DELETE CLIENT
//***********************************
client::~client()
{
	quit();

	//destroy client

	//close socket
	closeSocket();

	if(dllHandle != NULL)
	{
		//dll destroy
		destroyInstance(dllInstance);

		//dll unload
		unloadDll(dllHandle);
	} 

	//delete thread args
	if(recvArgs != NULL)
	{
		delete recvArgs;
	}

	if(sendArgs != NULL)
	{
		delete sendArgs;
	}

}//END OF ~CLIENT

//***********************************
//		RUN CLIENT
//***********************************
void client::run()
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
					//connect
					stop = !connectToServer(address);
				break;
				case CMD_DISCONNECT:
					printf("DISCONNECT!\n");
				break;
				case CMD_NETWORK:
					printf("NETWORK!\n Addr: %ud\n Port: %d\n",message->data.network.address,message->data.network.port);
					//address = ""+message->data.network.address;
					//port = ""+message->data.network.port;
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

}//END OF RUN
//***********************************
//		STOP CLIENT
//***********************************
void client::quit()
{
	//dll stop
	stopInstance(dllInstance);

	closeSocket();
}//END OF STOP

//***********************************
//		CLOSE CONNECTION
//***********************************
bool client::closeSocket()
{
	int iResult=0;
	// shutdown the connection since no more data will be sent
	
	if(sendArgs->mutex != NULL)
	{
		WaitForSingleObject( sendArgs->mutex, INFINITE );
		while(sendArgs->socketList->begin() != sendArgs->socketList->end())
		{
			cout<<"Destroying "<<*sendArgs->socketList->begin()<<endl;
			iResult = shutdown(*sendArgs->socketList->begin(), SD_SEND);
			if (iResult == SOCKET_ERROR) 
			{
				cout<<"shutdown failed: "<< WSAGetLastError()<<endl;
				closesocket(*sendArgs->socketList->begin());
				return false;
			}

			// cleanup
			closesocket(*sendArgs->socketList->begin());

			sendArgs->socketList->erase(sendArgs->socketList->begin());
		}
	
		ReleaseMutex( sendArgs->mutex);
	}
	WSACleanup();
	return true;
}//END OF CLOSESOCKET

//***********************************
//		OPEN CONNECTION
//***********************************
bool client::connectToServer(string address)
{
	SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) 
	{
        cout<<"WSAStartup failed: "<< iResult<<endl;
        return false;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo((PCSTR)address.c_str(), (PCSTR)CONNECTION_PORT, &hints, &result);
    if ( iResult != 0 ) 
	{
        cout<<"getaddrinfo failed: "<< iResult<<endl;
        return false;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) 
	{

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) 
		{
            cout<<"Error at socket(): "<< WSAGetLastError()<<endl;
            freeaddrinfo(result);
            return false;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) 
		{
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;//valid connection established
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) 
	{
        cout<<"Unable to connect to server!"<<endl;
        return false;
    } 
	cout<<"Connection accepted..."<<endl;

	recvArgs->socket = ConnectSocket;
	recvArgs->messageHandler = messageHandler;
	recvArgs->instance = dllInstance;

	sendArgs->socketList = new vector<SOCKET>();
	sendArgs->socketList->push_back(ConnectSocket);
	sendArgs->instance = dllInstance;
	sendArgs->pollQueue = hasMessagesToSend;
	sendArgs->nextMessage = getNextSend;
	sendArgs->mutex = CreateMutex (NULL, FALSE, NULL);
	
	stop = false;
	
	sendArgs->stop = &stop;
	recvArgs->stop = &stop;

	CreateThread(NULL,NULL,recv_service_connection,recvArgs,NULL,NULL);
	CreateThread(NULL,NULL,send_service_connection,sendArgs,NULL,NULL);

    return true;
}// END OF CONNECTTOSERVER