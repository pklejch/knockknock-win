#pragma once
struct Config
{
	Config(unsigned int auth_packet_delay, unsigned int auth_packet_timeout, bool legacy);
	Config();
	~Config(void);
	unsigned int auth_packet_delay;
	unsigned int auth_packet_timeout;
	bool legacy;
};

