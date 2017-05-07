#include "stdafx.h"
#include "KeyHasher.h"


// hashing function used for TCPConnection struct in unordered map
std::size_t KeyHasher::operator()(const Connection& conn) const  {
	using std::size_t;
	using std::hash;
	using std::string;
	size_t seed = 0;
	
	//seed ^= hash<UINT16>()(conn.source_port) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	seed ^= hash<UINT16>()(conn.dest_port) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	seed ^= hash<UINT32>()(conn.source_ip) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	seed ^= hash<UINT32>()(conn.dest_ip) + 0x9e3779b9 + (seed<<6) + (seed>>2);
	seed ^= hash<UINT8>()(conn.protocol) + 0x9e3779b9 + (seed<<6) + (seed>>2);

	//std::cout << seed << std::endl; 
	return seed;
}