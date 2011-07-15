//***********************************
//		HEADERS
//***********************************
//-----------------------------------
//Standard Headers
//-----------------------------------
#include <iostream>

//-----------------------------------
//Project Headers
//-----------------------------------
#include "server.h"
#include "client.h"
#include "server_client_common.h"

using namespace std;

//***********************************
//		LIBRARIES INCLUDED
//***********************************
//Ws2_32.lib

//***********************************
//		FORWARD DECLARTIONS
//***********************************

server* my_server;
client* my_client;
string	dllName;

bool parseCommandLine(int argc, char* argv[])
{
	cout<<__FUNCTION__<<endl;

	if (argc != 2)
	{
		cout <<"Invalid command line options"<<endl;
		return true;
	}
	else
	{
		dllName = argv[1];
		return false;
	}
}

int main(int argc, char* argv[])
{
	my_server=NULL;
	my_client=NULL;

	if(parseCommandLine(argc, argv))
	{
		return 0;
	}

	cout<<"0 for server, >0 for client"<<endl;
	int choice=0;
	cin>>choice;

	if(choice)
	{
		my_client = new client("127.0.0.1", dllName);
		my_client->run();
	}
	else
	{
		my_server = new server(dllName);
		my_server->run();
	}
	if(my_server!=NULL)
	{
		delete my_server;
	}
	else if(my_client!=NULL)
	{
		delete my_client;
	}

	return 0;
}