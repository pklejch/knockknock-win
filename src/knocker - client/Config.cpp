#include "stdafx.h"
#include "Config.h"


Config::Config(unsigned int auth_packet_delay, unsigned int auth_packet_timeout, bool legacy):auth_packet_delay(auth_packet_delay), 
	auth_packet_timeout(auth_packet_timeout), legacy(legacy)
{
}

Config::Config(){
}

Config::~Config(){
}