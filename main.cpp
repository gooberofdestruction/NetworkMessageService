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

service* g_service;
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
	g_service=NULL;

	if(parseCommandLine(argc, argv))
	{
		return 0;
	}

	cout<<"0 for server, >0 for client"<<endl;
	int choice=0;
	cin>>choice;

	if(choice)
	{
		g_service = new client(dllName, "127.0.0.1");
	}
	else
	{
		g_service = new server(dllName, "");
	}

	g_service->run();

	if(g_service!=NULL)
	{
		delete g_service;
	}

	return 0;
}