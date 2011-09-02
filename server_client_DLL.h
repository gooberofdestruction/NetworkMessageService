#ifndef SERVER_CLIENT_DLL_01112011
#define SERVER_CLIENT_DLL_01112011

#include "ipAddress.h"

//Buffer length of socket
#define SOCK_BUFFER_LEN 512

//maximum Message length
#define MESSAGE_LEN SOCK_BUFFER_LEN-sizeof(int)

//Message header
// - Size - size of the message excluding this header
// - The message follows this header
struct GenericHeader
{
	unsigned int	size;
	//message
};
//structure to hold messages to send
struct messageQueueStruct
{
	GenericHeader	hdr;
	char			message[MESSAGE_LEN];
};

typedef enum CMD_MESSAGE
{
	CMD_STOP = 0,
	CMD_CONNECT,
	CMD_DISCONNECT,
	CMD_NETWORK
};
struct StopMessage
{
	int reserved;
};
struct ConnectMessage
{
	int connect;
};
struct DisConnectMessage
{
	int reserved;
};
struct NetworkMessage
{
	unsigned int	address;
	unsigned short	port;
	unsigned short	reserved;
};

union CmdTypes
{
	StopMessage			stop;
	ConnectMessage		connect;
	DisConnectMessage	disconnect;
	NetworkMessage		network;
};

struct CmdMessage
{
	CMD_MESSAGE msg;
	CmdTypes data;
};

//--------------------------------------------
//Functions the DLL must implement
//--------------------------------------------
//Returns bool that indicates if there is a message to send
//bool hasMessagesToSend(void* instance);
typedef bool (__cdecl *HASMSGFUNC)(void*);

//returns a pointer to a message queue structure that has the populated data to send
//server/client will reclaim memeory
//messageQueueStruct* getNextSend(void* instance);
typedef messageQueueStruct* (__cdecl *NEXTMSG)(void*);

//given a populated buffer and size
//void messageHandler(void* instance, char* buffer, int size);
typedef void (__cdecl *MSGHANDLER)(void*, char*, int);

//constructor
//void createInstance(void** instance);
typedef void (__cdecl *CREATE)(void**);

//destructor
//void destroyInstance(void* instance);
typedef void (__cdecl *DESTROY)(void*);

//main execution sequence
//void runInstance(void* instance);
typedef void (__cdecl *RUN)(void*);

//stop main execution sequence
//void stopInstance(void* instance);
typedef void (__cdecl *STOP)(void*);

//bool allowConnection(void* instance);
typedef bool (__cdecl *MAX)(void*);

//Returns bool that indicates if there is a cmd to process
//bool hasCmdMesssages(void* instance);
typedef bool (__cdecl *HASCMDFUNC)(void*);

//returns a pointer to a cmd message
//server/client will reclaim memeory
//CmdMessage* getNextCmd(void* instance);
typedef CmdMessage* (__cdecl *NEXTCMD)(void*);

#endif