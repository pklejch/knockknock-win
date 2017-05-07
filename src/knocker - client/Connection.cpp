#include "stdafx.h"
#include "Connection.h"


Connection::Connection(UINT16 source_port, UINT16 dest_port, UINT32 source_ip, UINT32 dest_ip, UINT8 protocol):
	source_port(source_port), dest_port(dest_port), source_ip(source_ip), dest_ip(dest_ip), protocol(protocol)
{
}

Connection::Connection(void):source_port(0), dest_port(0), source_ip(0), dest_ip(0), protocol(0) {};

// same elements are compared using less then operator as:  !(a < b) && !(b < a)
bool Connection::operator<(const Connection & conn) const {
	if (//source_port == conn.source_port && 
		dest_port == conn.dest_port && 
		source_ip == conn.source_ip && 
		dest_ip == conn.dest_ip && 
		protocol == conn.protocol){
		return false;
	}
	return true;
}

bool Connection::operator==(const Connection & conn)const {
	if (//source_port == conn.source_port && 
		dest_port == conn.dest_port && 
		source_ip == conn.source_ip && 
		dest_ip == conn.dest_ip && 
		protocol == conn.protocol){
		return true;
	}
	return false;
}

void Connection::print() const{
	UINT8 * saddr = (UINT8*) &source_ip;
	UINT8 * daddr = (UINT8*) &dest_ip;
	std::string message;
	message += "SPORT: ";
	message += std::to_string(source_port);

	message += " DPORT: ";
	message += std::to_string(dest_port);

	message += " SADDR: ";
	message += std::to_string(saddr[0]);
	message += ".";
	message += std::to_string(saddr[1]);
	message += ".";
	message += std::to_string(saddr[2]);
	message += ".";
	message += std::to_string(saddr[3]);

	message += " DADDR: ";
	message += std::to_string(daddr[0]);
	message += ".";
	message += std::to_string(daddr[1]);
	message += ".";
	message += std::to_string(daddr[2]);
	message +=".";
	message += std::to_string(daddr[3]);

	message += " PROTO: ";
	if (protocol == IPPROTO_UDP){
		message += "UDP";
	}
	else if (protocol == IPPROTO_TCP){
		message += "TCP";
	}
	else{
		message += "UNKNOWN";
	}

	CLogger::debug(message);
}