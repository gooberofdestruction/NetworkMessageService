#ifndef IPADDRESS_08222011
#define IPADDRESS_08222011
#include <string>

typedef unsigned int RAW_IP;
typedef unsigned short RAW_PORT;
typedef unsigned char OCTET;

class IpAddress
{
private:
	RAW_IP		raw_ip;
	RAW_PORT	raw_port;

	void stringToRawIpConversion(std::string ip);
	void stringToRawPortConversion(std::string port);

public:
	IpAddress(std::string ip,	std::string port);
	IpAddress(std::string ip,	RAW_PORT port);
	IpAddress(RAW_IP ip=0,		RAW_PORT port=0);
	
	~IpAddress();

	std::string getDotFormattedIp();
	RAW_IP		getRawIp();
	std::string getStringPort();
	RAW_PORT	getRawPort();

	void setAddress(RAW_IP ip);
	void setAddress(std::string ip);
	void setPort(RAW_PORT port);
	void setPort(std::string port);
};

#endif