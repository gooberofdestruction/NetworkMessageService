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
client::client(std::string dllName, std::string p_address)
{
	setAddress(p_address, CONNECTION_PORT);

	if(setupDll(dllName) && setupConnections())
	{
		startServices();
	}
	else
	{
		stopServices();
	}

}//END OF CLIENT

//***********************************
//		DELETE CLIENT
//***********************************
client::~client()
{
	destroyData();
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
					//connect
					connectServices();
					if (!connectToServer(getAddress()))
					{
						stopServices();
						disconnectServices();
					}
					else
					{
						startSendService();
					}
				break;
				case CMD_DISCONNECT:
					printf("DISCONNECT!\n");
					disconnectServices();
				break;
				case CMD_NETWORK:
					printf("NETWORK!\n Addr: %ud\n Port: %d\n",message->data.network.address,message->data.network.port);
					setAddress(message->data.network.address,message->data.network.port);
				break;
				default:
					printf("STOP!\n");
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

}//END OF RUN

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
    iResult = WSAStartup(MAKEWORD(2,2), getWsa());
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

	addRecvConnection(&ConnectSocket);
	addSendConnection(&ConnectSocket);

    return true;
}// END OF CONNECTTOSERVER