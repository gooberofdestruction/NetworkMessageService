#ifndef CLIENT_04262010
#define CLIENT_04262010
//***********************************
//		HEADERS
//***********************************
//-----------------------------------
//Standard Headers
//-----------------------------------
#include <list>
//-----------------------------------
//Project Headers
//-----------------------------------
#include "service.h"

class client: public service
{
private:

	bool connectToServer(std::string address);
public:
	client(std::string dllName, std::string address);
	~client();
	void run();

};
#endif