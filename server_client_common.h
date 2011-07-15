#ifndef SERVER_CLIENT_COMMON_04252010
#define SERVER_CLIENT_COMMON_04252010
//***********************************
//		HEADERS
//***********************************
//-----------------------------------
//Standard Headers
//-----------------------------------
#include <vector>
#include <stdlib.h>
#include <WinSock2.h>
#include <Windows.h>
#include <ws2tcpip.h>

//-----------------------------------
//Project Headers
//-----------------------------------
#include "server_client_DLL.h"

//-----------------------------------
//Definitions
//-----------------------------------
#define CONNECTION_PORT	"1337"

//Buffer length of socket
#define SOCK_BUFFER_LEN 512

//windows recv() return to close socket
#define CLOSE_SOCKET 0

//Buffer length was not enough to contain message
#define INVALID_BUFFER_LENGTH SOCK_BUFFER_LEN+1

//
#define REMOTE_DLL_STOP SOCK_BUFFER_LEN+1

//
#define GIVE_UP_REMAINING_THREAD_TIME 1000

//argument structure to pass arguments to 
//the recv_service_connection thread function
struct recvThreadArgs
{
	bool	*stop;
	SOCKET	socket;
	void*	instance;
	void	(*messageHandler) (void* , char*, int);
};

//argument structure to pass arguments to 
//the send_service_connection thread function
struct sendThreadArgs
{
	bool	*stop;
	std::vector<SOCKET>	*socketList; 
	HANDLE	mutex; 
	void*	instance;
	HASMSGFUNC pollQueue;
	NEXTMSG nextMessage;
};

struct runDllThreadArgs
{
		bool	*stop;
		void*	instance;
		RUN		runInstance;
		STOP	stopInstance;
};

//-----------------------------------
//Common Functions
//-----------------------------------
DWORD WINAPI run_dll(LPVOID args);

//-----------------------------------------------------------------
//recv_service_connection
//
//Description:
//	Thread function that services recv's on a connected socket.
//	Receives a connected socket and a function pointer to handle 
//	messags from the args structure.  It then starts a loop that 
//	blocks on a select() call to receive messages from the socket.  
//	This loop continues until the socket is closed or an error is 
//	encountered. It closes the socket just before the function returns.
//
//Inputs:	args - pointer to a populated recvThreadArgs structure.
//
//Outputs:	NONE
//
//Returns:	0
//-----------------------------------------------------------------
DWORD WINAPI recv_service_connection(LPVOID args);

//-----------------------------------------------------------------
//send_service_connection
//
//Description:
//	Thread function that services sends on a connected socket.
//	Receives a connected socket, a function to poll the class 
//	if it has messages, and a function to get the next message 
//	to send.  Starts a loop that polls the class for messages.
//	If messages are waiting to be sent then all messages are sent.  If no messages
//	are waiting or all have been sent then the thread releases its execution time
//	for other processes.  The loop ends when an error occurs with the socket.
//	The function closes the connection just before it returns.
//
//Inputs:	args - pointer to a populated sendThreadArgs structure.
//
//Outputs:	NONE
//
//Returns:	0
//-----------------------------------------------------------------
DWORD WINAPI send_service_connection(LPVOID args);

//-----------------------------------------------------------------
//packMessage
//
//Description:
//	Packs a message into a buffer defined by length and message and
//	tries to send the message on an open socket.  The function keeps
//	sending on the socket until the entire message has been sent.
//
//Inputs:	sock	-	a connected socket
//			message	-	the message to send
//			buf		-	a buffer to pack the message and header in
//			size	-	size of the buffer
//			flags	-	any flags to pass to underlying send()
//Outputs:	NONE
//
//Returns:	INVALID_BUFFER_LENGTH	- the buffer was not large enough to hold message and header
//			SOCKET_ERROR			- Error on socket
//			Total bytes sent		- On success the number of bytes written
//-----------------------------------------------------------------
int packMessage(SOCKET sock, messageQueueStruct* message, char* buf, int size, int flags);

//-----------------------------------------------------------------
//recvCompleteMessage
//
//Description:
//	Receives a message from an open socket and populates the buffer
//	with the message.  First the header is read to determine the length
//	of the remaining message.  Then rest of the message is then read.
//
//Inputs:	sock	-	a connected socket
//			buffer	-	a buffer to populate the message in
//			length	-	length of buffer
//			flags	-	any flags to pass to underlying recv()
//
//Outputs:	buffer	-	holds a message to process
//
//Returns:	INVALID_BUFFER_LENGTH	- Message from socket was too large for buffer
//			CLOSE_SOCKET			- Close socket signal was encounterd
//			SOCKET_ERROR			- Error on socket
//			Total bytes recv'ed		- On success the number of bytes written to buffer
//-----------------------------------------------------------------
int recvCompleteMessage(SOCKET sock, char* buffer, int length, int flags);

//going into a debug dll
void debugMessageParser(char* buffer, int size);
bool debugPollQueue();
void* debugNextMessage();

typedef void (__cdecl *funcPtr)(void); 

HINSTANCE loadDll(const char* dllName);
FARPROC WINAPI loadFunction(HINSTANCE dllHandle, const char* functionName);
void unloadDll(HINSTANCE dllHandle);

#endif