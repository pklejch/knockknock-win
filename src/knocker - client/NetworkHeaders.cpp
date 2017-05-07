#include "stdafx.h"
#include "NetworkHeaders.h"

void PacketIpInit(PWINDIVERT_IPHDR packet){
    memset(packet, 0, sizeof(WINDIVERT_IPHDR));
    packet->Version = 4;
    packet->HdrLength = sizeof(WINDIVERT_IPHDR) / sizeof(UINT32);
    packet->TTL = 64;
	WINDIVERT_IPHDR_SET_DF(packet, 1);
}


void PacketIpTcpInit(PTCPPACKET packet){
    memset(packet, 0, sizeof(TCPPACKET));
    PacketIpInit(&packet->ip);
    packet->ip.Length = htons(sizeof(TCPPACKET));
    packet->ip.Protocol = IPPROTO_TCP;
    packet->tcp.HdrLength = sizeof(WINDIVERT_TCPHDR) / sizeof(UINT32);
	packet->tcp.Syn = 1;
}

void fill_packet(PTCPPACKET auth_packet, PWINDIVERT_IPHDR ip_header, 
				 std::map<std::string, UINT32> parsed_line, bool legacy,
				 std::default_random_engine &generator, 
				 std::uniform_int_distribution<UINT16> distribution){


	//copy src and dest IPv4 from original packet
	auth_packet->ip.DstAddr = ip_header->DstAddr;
	auth_packet->ip.SrcAddr = ip_header->SrcAddr;

	// create "random" source port
	// it will be from range <49152, 65535> as recommended by IANA
	UINT16 random_src_port = distribution(generator);
	auth_packet->tcp.SrcPort = htons(random_src_port);


	// in non legacy mode we have 2B more data in Urgent pointer field
	if(!legacy){
		auth_packet->tcp.UrgPtr = htons((UINT16)parsed_line["URG"]);
	}

	//fill right values into headers field
	auth_packet->ip.Id = htons((UINT16)parsed_line["ID"]);
	auth_packet->tcp.SeqNum = htonl((UINT32)parsed_line["SEQ"]);
	auth_packet->tcp.AckNum = htonl((UINT32)parsed_line["ACK"]);
	auth_packet->tcp.Window = htons((UINT16)parsed_line["WINDOW"]);
	auth_packet->tcp.DstPort = htons((UINT16)parsed_line["DPORT"]);

	//recalculate checksums
	WinDivertHelperCalcChecksums((PVOID)auth_packet, sizeof(TCPPACKET), 0);
}