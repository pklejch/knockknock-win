#pragma once
#include <cstdlib>
#include "Connection.h"

// struct for hashing TCPConnection struct in unordered map
struct KeyHasher{
	std::size_t operator()(const Connection& conn) const;
};