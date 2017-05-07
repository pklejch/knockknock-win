#pragma once
struct Connection
{
	UINT16 source_port;
	UINT16 dest_port;
	UINT32 source_ip;
	UINT32 dest_ip;
	UINT8 protocol;
	Connection(UINT16 source_port, UINT16 dest_port, UINT32 source_ip, UINT32 dest_ip, UINT8 protocol);
	Connection();
	bool operator< (const Connection & conn) const;
	bool operator== (const Connection & conn) const;
	void print() const;
};

