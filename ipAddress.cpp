#include "ipAddress.h"
#include <sstream>

using namespace std;

IpAddress::IpAddress(string ip, string port)
{
	stringToRawIpConversion(ip);
	stringToRawPortConversion(port);
}
IpAddress::IpAddress(string ip, RAW_PORT port)
{
	stringToRawIpConversion(ip);

	raw_port = port;
}
IpAddress::IpAddress(RAW_IP ip,	RAW_PORT port)
{
	raw_ip = ip;
	raw_port = port;
}

IpAddress::~IpAddress()
{
}

string	IpAddress::getDotFormattedIp()
{
	 char* octets = (char*)&raw_ip;
	 std::stringstream str("");
	 str<<(int)octets[3]<<"."<<(int)octets[2]<<"."<<(int)octets[1]<<"."<<(int)octets[0];
	 return str.str();
}

RAW_IP	IpAddress::getRawIp()
{
	return raw_ip;
}

string	IpAddress::getStringPort()
{
	std::stringstream str("");
	str<<raw_port;
	return str.str();
}

RAW_PORT IpAddress::getRawPort()
{
	return raw_port;
}

void IpAddress::setAddress(RAW_IP ip)
{
	raw_ip = ip;
}

void IpAddress::setAddress(std::string ip)
{
	stringToRawIpConversion(ip);
}

void IpAddress::setPort(RAW_PORT port)
{
	raw_port = port;
}

void IpAddress::setPort(std::string port)
{
	stringToRawPortConversion(port);
}

void IpAddress::stringToRawPortConversion(std::string port)
{
	sscanf(port.c_str(), "%d", &raw_port);
}

void IpAddress::stringToRawIpConversion(std::string ip)
{
	int	  temp[4]={0};
	OCTET octets[4]={0};
	
	sscanf(ip.c_str(),"%d.%d.%d.%d\n", &temp[0], &temp[1], &temp[2], &temp[3]);//convert to string stream
	for(int i=0; i<4; i++)
	{
		octets[i]=temp[i];
		raw_ip = raw_ip<<8;
		raw_ip += octets[i];
	}
}