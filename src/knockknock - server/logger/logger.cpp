/*
 * logger.cpp
 * (C) 2016, all rights reserved,
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#include "stdafx.h"

// maximum size of IPv4 header (60B) + maximum size of TCP header (60B) = 120B 
// but most of the time 40B should be fine
#define MAXBUF  120

// maximum size of message send to the pipe
#define MAXMSG 256


bool check_packet(UINT16 source_port, UINT16 dest_port, UINT32 seq_number, UINT32 ack_number, UINT16 window, UINT16 id, UINT8 * ip){
	//TODO
	return true;
}

std::string construct_filter(int ports_num, char ** ports){
	std::string filter;
	if (ports_num < 1){
		fprintf(stderr, "No valid ports from config files\n");
		exit(EXIT_FAILURE);
	}
	filter = "ip and inbound and tcp.Syn and (";
	for(int i = 0; i < ports_num; i++){
		if(i == (ports_num - 1)){
			filter += "tcp.DstPort == ";
			filter += ports[i];
		}else{
			filter += "tcp.DstPort == ";
			filter += ports[i];
			filter += " or ";
		}
	}
	filter += ")";
	return filter;
}

int __cdecl main(int argc, char **argv)
{
#ifdef _DEBUG
	std::ofstream log("c:\\log.log");
#endif // DEBUG



	//handle for network device
    HANDLE handle;

	//buffer for TCP + IPv4 header
    unsigned char packet[MAXBUF];
    UINT packet_len = 0;

	// structures for parsing packet
    WINDIVERT_ADDRESS addr;
    PWINDIVERT_IPHDR ip_header;
    PWINDIVERT_TCPHDR tcp_header;

	OVERLAPPED op;
	ZeroMemory(&op, sizeof(op));
	op.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// catch only incoming IPv4 and TCP SYN packets
	// with destPort from config files
	std::string filter = construct_filter(argc - 1, argv + 1);

	// buffer for sending messages
	char buffer[MAXMSG];
	DWORD bytes_written = 0;

	// data will go from logger -> knockknock daemon
	// second direction is used for detecting dead client

	HANDLE pipe = CreateNamedPipe("\\\\.\\pipe\\log", PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 
		PIPE_TYPE_MESSAGE | PIPE_WAIT | PIPE_READMODE_MESSAGE, 1, MAXMSG, MAXMSG, 0, NULL);

	if(pipe == INVALID_HANDLE_VALUE){
		fprintf(stderr, "Error while creating pipe, error code: %d \n", GetLastError());
		exit(EXIT_FAILURE);
	}


    // Divert traffic matching the filter
	// Use sniff mode (copy & divert)
	handle = WinDivertOpen(filter.c_str(), WINDIVERT_LAYER_NETWORK, 0, WINDIVERT_FLAG_SNIFF);
    if (handle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "error: failed to open the WinDivert device (%d)\n",
            GetLastError());
        exit(EXIT_FAILURE);
    }

    // Max-out the packet queue:
    if (!WinDivertSetParam(handle, WINDIVERT_PARAM_QUEUE_LEN, 8192))
    {
        fprintf(stderr, "error: failed to set packet queue length (%d)\n",
            GetLastError());
        exit(EXIT_FAILURE);
    }
    if (!WinDivertSetParam(handle, WINDIVERT_PARAM_QUEUE_TIME, 2048))
    {
        fprintf(stderr, "error: failed to set packet queue time (%d)\n",
            GetLastError());
        exit(EXIT_FAILURE);
    }

	if(!ConnectNamedPipe(pipe, NULL)){
		fprintf(stderr, "error while connecting to pipe\n");
		exit(EXIT_FAILURE);
	}
#ifdef _DEBUG
	log << "starting to catching packets"<< std::endl;
#endif // _DEBUG


    // Main loop:
    while (true)
    {
		// detect dead client (only a test read)
		BOOL success = PeekNamedPipe(pipe, NULL, NULL, NULL, NULL, NULL);

		if(!success && GetLastError() == ERROR_BROKEN_PIPE){
			fprintf(stderr, "Client closed pipe, closing it too.");
			DisconnectNamedPipe(pipe);
			exit(EXIT_FAILURE);
		}

		// Read a matching packet.
		if (!WinDivertRecv(handle, packet, sizeof(packet), &addr, &packet_len))
		{
			fprintf(stderr, "warning: failed to read packet (%d)\n",
				GetLastError());
			continue;
		}

		// Calculate checksums, b
		//WinDivertHelperCalcChecksums(packet, packet_len, WINDIVERT_HELPER_NO_REPLACE);
       
		// parse only TCP and IPv4 header
		if(!WinDivertHelperParsePacket(packet, packet_len, &ip_header,
			NULL, NULL, NULL, &tcp_header,
			NULL, NULL, NULL)){
				continue;
		}

		if (ip_header != NULL && tcp_header != NULL)
		{
			UINT8 *src_addr = (UINT8 *)&ip_header->SrcAddr;
			UINT8 *dst_addr = (UINT8 *)&ip_header->DstAddr;

			_snprintf_s(buffer, MAXMSG, MAXMSG-1 ,"SPT=%hu DPT=%hu SEQ=%u ACK=%u WINDOW=%hu ID=%hu SRC=%hhu.%hhu.%hhu.%hhu URG=%hu\n\0",
				ntohs(tcp_header->SrcPort), ntohs(tcp_header->DstPort),
				ntohl(tcp_header->SeqNum), ntohl(tcp_header->AckNum), ntohs(tcp_header->Window),
				ntohs(ip_header->Id), src_addr[0], src_addr[1], src_addr[2], src_addr[3], ntohs(tcp_header->UrgPtr));
				
			printf("Sending into pipe: %s", buffer);
#ifdef _DEBUG
			log << buffer << std::endl;
#endif // _DEBUG


			if(!WriteFile(pipe, buffer, (DWORD)strlen(buffer), &bytes_written, &op)){
				DWORD err = GetLastError();
				if(ERROR_IO_PENDING != err){
					continue;
				}
			}
			WaitForSingleObject(op.hEvent, INFINITE);
		}
	}
	exit(EXIT_SUCCESS);
}
