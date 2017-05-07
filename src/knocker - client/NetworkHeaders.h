#pragma once

typedef struct{
    WINDIVERT_IPHDR ip;
    WINDIVERT_TCPHDR tcp;
} TCPPACKET, *PTCPPACKET;

void PacketIpInit(PWINDIVERT_IPHDR packet);
void PacketIpTcpInit(PTCPPACKET packet);
void fill_packet(PTCPPACKET auth_packet, PWINDIVERT_IPHDR ip_header, 
				 std::map<std::string, UINT32> parsed_line, bool legacy,
				 std::default_random_engine &generator, 
				 std::uniform_int_distribution<UINT16> distribution);