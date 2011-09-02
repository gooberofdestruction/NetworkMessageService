//***********************************
//		HEADERS
//***********************************
//-----------------------------------
//Standard Headers
//-----------------------------------
#include <iostream>
#include <sstream>

//-----------------------------------
//Project Headers
//-----------------------------------
#include "server_client_common.h"
#include "server_client_DLL.h"

//***********************************
//	Run DLL
//***********************************
DWORD WINAPI run_dll(LPVOID args)
{
	//-----------------------------------------------------------------
	//Variables
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	//Setting variables to the arguments passed in.
	//-----------------------------------------------------------------
	void* instance = ((runDllThreadArgs*)args)->instance;
	RUN runInstance = ((runDllThreadArgs*)args)->runInstance;		
	STOP stopInstance = ((runDllThreadArgs*)args)->stopInstance;	
	bool* stop = ((runDllThreadArgs*)args)->stop;

	//-----------------------------------------------------------------
	//While socket is still open, change to still have valid sockets
	//-----------------------------------------------------------------
	if( !*stop )
	{
		runInstance(instance);
	}//END OF while(bytes_sent!=SOCKET_ERROR)
	
	//-----------------------------------------------------------------
	//Clean up
	//-----------------------------------------------------------------
	stopInstance(instance);

	return 0;
}

//***********************************
//	SERVICE RECV's ON CONNECTION
//***********************************
DWORD WINAPI recv_service_connection(LPVOID args)
{
	//-----------------------------------------------------------------
	//Variables
	//-----------------------------------------------------------------

	//Number of bytes recvd from the socket,
	//close socket, or error. Initial value to allow loop to run.
	int bytes_recvd=!CLOSE_SOCKET;

	//buffer to read to
	char buffer[SOCK_BUFFER_LEN];
	
	//buffer to copy too
	char* send_buffer;

	//flags for reading from socket
	int flags = 0;	//None

	//fd_set for select call
	fd_set my_set;

	//-----------------------------------------------------------------
	//Setting variables to the arguments passed in.
	//-----------------------------------------------------------------
	void* instance = ((recvThreadArgs*)args)->instance;
	SOCKET my_sock = ((recvThreadArgs*)args)->socket;						//connected socket
	MSGHANDLER messageHandler = ((recvThreadArgs*)args)->messageHandler;	//dll function to handle incoming messages
	bool* stop = ((recvThreadArgs*)args)->stop;

	//-----------------------------------------------------------------
	//While socket is still open process incoming messages.
	//-----------------------------------------------------------------
	while( !*stop && bytes_recvd!=CLOSE_SOCKET )
	{
		//Create FD Set
		FD_ZERO(&my_set);
		FD_SET(my_sock,&my_set);

		//select on FD_SET
		select(NULL,&my_set,NULL,NULL,NULL);
		
		//if I have something
		if(FD_ISSET(my_sock,&my_set))
		{

			//recv the from socket
			bytes_recvd = recvCompleteMessage(my_sock, buffer, SOCK_BUFFER_LEN, flags);

			//is it a close connection?
			if (bytes_recvd == CLOSE_SOCKET /*|| bytes_recvd == REMOTE_DLL_STOP*/)
			{
				//socket is closed. Stop processing.
			}
			//is it a buffer error?
			else if (bytes_recvd == INVALID_BUFFER_LENGTH)
			{
				//message overflowed buffer.  Something is wrong, close socket.
				bytes_recvd = CLOSE_SOCKET;
			}
			//is it a socket error?
			else if(bytes_recvd == SOCKET_ERROR)
			{
				//socket error.  Clean up the WSA state to clear error, close socket.
				bytes_recvd=CLOSE_SOCKET;
			}
			//I have data
			else 
			{
				send_buffer = new char[SOCK_BUFFER_LEN];
				memcpy(send_buffer, buffer, SOCK_BUFFER_LEN);
				messageHandler(instance, send_buffer, bytes_recvd);

			}//END OF RECV BLOCK
		}
		else
		{
			//select stopped for an FD I did not specify. Ignore.
		}//END OF if/else block

	}//END OF while(bytes_recvd!=CLOSE_SOCKET)

	//-----------------------------------------------------------------
	//Clean up
	//-----------------------------------------------------------------

	return 0;
}//END OF recv_service_connection

//***********************************
//	SERVICE SENDS ON CONNECTION
//***********************************
DWORD WINAPI send_service_connection(LPVOID args)
{
	//-----------------------------------------------------------------
	//Variables
	//-----------------------------------------------------------------

	//Number of bytes sent from the socket,
	//or error
	int bytes_sent=!SOCKET_ERROR;

	//buffer to read to
	char buffer[SOCK_BUFFER_LEN];
	messageQueueStruct* message;

	//sleep time
	int time_out=GIVE_UP_REMAINING_THREAD_TIME;

	//hold boolean value of poll functon
	bool poll = true;

	std::vector<SOCKET>::iterator currSock;
	std::vector<SOCKET>::iterator eraseSock;

	//-----------------------------------------------------------------
	//Setting variables to the arguments passed in.
	//-----------------------------------------------------------------
	void* instance = ((sendThreadArgs*)args)->instance;
	std::vector<SOCKET> *sockList = ((sendThreadArgs*)args)->socketList;				//connected socket list
	bool (*pollQueue) (void* ) = ((sendThreadArgs*)args)->pollQueue;					//polling the dll to see if it has messages to send
	messageQueueStruct* (*nextMessage) (void* ) = ((sendThreadArgs*)args)->nextMessage;	//get the next message from the dll
	HANDLE mutex = ((sendThreadArgs*)args)->mutex;
	bool* stop = ((sendThreadArgs*)args)->stop;

	//-----------------------------------------------------------------
	//While do not stop and no errors have occured
	//-----------------------------------------------------------------
	while( !*stop && bytes_sent!=SOCKET_ERROR )
	{
		//poll the dll for a message
		poll = pollQueue(instance);

		//-----------------------------------------------------------------
		//While do not stop and still have valid sockets to send on
		//-----------------------------------------------------------------
		WaitForSingleObject( mutex, INFINITE );

		while(!*stop && poll && sockList->size() != 0 ) 
		{
			message = nextMessage(instance);
			
			//-----------------------------------------------------------------
			//While do no stop and send on all my connections.
			//-----------------------------------------------------------------
			currSock = sockList->begin();
			while(!*stop && sockList->size() != 0 && currSock != sockList->end())
			{
				bytes_sent = packMessage(*currSock, message, buffer, SOCK_BUFFER_LEN, 0);//current socket looking at
				
				if(bytes_sent == SOCKET_ERROR)
				{
					//Error on socket.  Stop processing, close socket and update list
					closesocket(*currSock);
					eraseSock = currSock;

					currSock = sockList->erase(eraseSock);

					continue;
				}
				//else if(bytes_sent == INVALID_BUFFER_LENGTH)
				//{
					//Message being sent is too large for buffer.  Stop processing. change to ignore the message
					//bytes_sent = SOCKET_ERROR;
				//}
				else if(bytes_sent == CLOSE_SOCKET)
				{
					//DLL is trying to stop
					*stop= true;
				}

				if( !*stop && sockList->size() != 0 && currSock != sockList->end())
				{
					currSock++;
				}
			}
			
			//clean up message
			delete message;

			//poll the dll again for any more messages
			poll = pollQueue(instance);

		}//END OF while(poll && bytes_sent!=SOCKET_ERROR) 
		ReleaseMutex( mutex );
	Sleep(time_out);
	}//END OF while(bytes_sent!=SOCKET_ERROR)
	
	//-----------------------------------------------------------------
	//Clean up
	//-----------------------------------------------------------------

	return 0;
}// END OF send_service_connection

//***********************************
//	RECEIVE A COMPLETE MESSAGE
//***********************************
int recvCompleteMessage(SOCKET sock, char* buffer, int length, int flags)
{
	int bytes_recvd = 0;
	int message_size = 0;
	int current_recv = 0;
	
	//error state, not enough room on buffer to read the generic header
	if(length < sizeof(GenericHeader))
	{
		return INVALID_BUFFER_LENGTH;
	}
	
	//-----------------------------------------------------------------
	//read the header to be able to determine size of message
	//-----------------------------------------------------------------
	while(bytes_recvd < sizeof(GenericHeader))
	{
		bytes_recvd = recv(sock, buffer+bytes_recvd, sizeof(GenericHeader)-bytes_recvd, 0);
		
		if(bytes_recvd == CLOSE_SOCKET)
		{
			return CLOSE_SOCKET;
		}
		else if (bytes_recvd == SOCKET_ERROR)
		{
			return SOCKET_ERROR;
		}
	}// END OF while(bytes_recvd < sizeof(GenericHeader))
	
	//-----------------------------------------------------------------
	//found size. read rest of message
	//-----------------------------------------------------------------
	message_size = ((GenericHeader*)buffer)->size;

	bytes_recvd = 0;

	while(bytes_recvd < message_size)
	{
		current_recv = recv(sock, buffer+bytes_recvd, message_size-bytes_recvd, 0);

		if(current_recv == CLOSE_SOCKET)
		{
			return CLOSE_SOCKET;
		}
		else if (bytes_recvd == SOCKET_ERROR)
		{
			return SOCKET_ERROR;
		}

		bytes_recvd += current_recv;
	}// END OF 	while(bytes_recvd < message_size)

	return bytes_recvd;
}// END OF recvCompleteMessage

//***********************************
//	SEND A COMPLETE MESSAGE
//***********************************
int packMessage(SOCKET sock, messageQueueStruct* message, char* buf, int size, int flags)
{
	int bytes_sent = 0;
	int send_return = !SOCKET_ERROR;

	//length of message is greater than buffer.  Return error
	if(message->hdr.size+sizeof(message->hdr) > (unsigned int)size)
	{
		return INVALID_BUFFER_LENGTH;
	}

	//copy message and header to buffer
	memcpy(buf, message, message->hdr.size+sizeof(message->hdr));

	//-----------------------------------------------------------------
	//Keep sending chunks of the message until the entire message is sent or an error is found.
	//-----------------------------------------------------------------
	while ((unsigned int)bytes_sent < message->hdr.size+sizeof(message->hdr) && send_return != SOCKET_ERROR)
	{
		send_return = send(sock, buf+bytes_sent, (message->hdr.size+sizeof(message->hdr))-bytes_sent, flags);

		if(send_return == SOCKET_ERROR)
		{
			bytes_sent = send_return;
		}
		else
		{
			bytes_sent += send_return;
		}

	}//END OF while (bytes_sent < length+sizeof(length) && send_return != SOCKET_ERROR)

	return bytes_sent;

}//END OF packMessage

 
HINSTANCE loadDll(const char* dllName)
{ 
    // Get a handle to the DLL module.
    return LoadLibrary(TEXT(dllName)); 
}
FARPROC WINAPI loadFunction(HINSTANCE dllHandle, const char* functionName)
{
	// If the handle is valid, try to get the function address.
    if (dllHandle != NULL) 
    { 
        return GetProcAddress(dllHandle, functionName); 
    }
	return NULL;
}

void unloadDll(HINSTANCE dllHandle)
{
	//free instance
	FreeLibrary(dllHandle);
}