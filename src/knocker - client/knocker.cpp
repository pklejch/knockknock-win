// knocker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// max size of TCP/IP packet = 65535 B
#define MAXBUF 0xFFFF

std::mutex g_mutex;

bool g_legacy;

std::map<std::string, UINT32> parse_result(std::string packetInfo){
	std::smatch match;
	std::map<std::string, UINT32> parsed_line;
	if(g_legacy){
		std::regex re("DPORT=(\\d+) ID=(\\d+) WINDOW=(\\d+) SEQ=(\\d+) ACK=(\\d+)");
		if (std::regex_search(packetInfo, match, re)) {
			parsed_line["DPORT"] = std::stoul(match[1]);
			parsed_line["ID"] = std::stoul(match[2]);
			parsed_line["WINDOW"] = std::stoul(match[3]);
			parsed_line["SEQ"] = std::stoul(match[4]);
			parsed_line["ACK"] = std::stoul(match[5]);
		} 
	}
	else{
		std::regex re("DPORT=(\\d+) ID=(\\d+) WINDOW=(\\d+) SEQ=(\\d+) ACK=(\\d+) URG=(\\d+)");
		if (std::regex_search(packetInfo, match, re)) {
			parsed_line["DPORT"] = std::stoul(match[1]);
			parsed_line["ID"] = std::stoul(match[2]);
			parsed_line["WINDOW"] = std::stoul(match[3]);
			parsed_line["SEQ"] = std::stoul(match[4]);
			parsed_line["ACK"] = std::stoul(match[5]);
			parsed_line["URG"] = std::stoul(match[6]);
		} 
	}



	return parsed_line;
}

void check_ip_address(){
    /* Declare and initialize variables */

    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    unsigned int i = 0;

    // Set the flags to pass to GetAdaptersAddresses
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

    // default to unspecified address family (both)
    ULONG family = AF_UNSPEC;

    LPVOID lpMsgBuf = NULL;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;

	ULONG mask;
    family = AF_INET;
	UINT32 ip =  htonl(3232249857);
    // Allocate a 15 KB buffer to start with.
    outBufLen = 15000;

    do {

        pAddresses = (IP_ADAPTER_ADDRESSES *) malloc(outBufLen);
        if (pAddresses == NULL) {
            printf
                ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
            exit(1);
        }

        dwRetVal =
            GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            free(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < 3));

    if (dwRetVal == NO_ERROR) {
        // If successful, output some information from the data we received
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            pUnicast = pCurrAddresses->FirstUnicastAddress;
			ConvertLengthToIpv4Mask(pUnicast->OnLinkPrefixLength, &mask);	
            if (pUnicast != NULL) {
                for (i = 0; pUnicast != NULL; i++){
					struct sockaddr_in *addr_in = (struct sockaddr_in *) pUnicast->Address.lpSockaddr;
					std::cout << addr_in->sin_addr.S_un.S_addr<< std::endl;
					std::cout << inet_ntoa(addr_in->sin_addr) << std::endl;
					std::cout << "mask "<< (unsigned int)pUnicast->OnLinkPrefixLength << std::cout;
					UINT32 ipa = htonl(addr_in->sin_addr.S_un.S_addr);
					UINT32 rightMask = htonl(mask);
					UINT32 a = (ipa & rightMask);
					UINT32 b =  (ip & rightMask);
					if(a == b){
						std::cout << inet_ntoa(addr_in->sin_addr) << " is in the same network as 192.168.56.4" << std::endl;
					}
                    pUnicast = pUnicast->Next;
				}

            }
            printf("\n");

            pCurrAddresses = pCurrAddresses->Next;
        }
    } 
    if (pAddresses) {
        free(pAddresses);
    }
}

bool compare_time(std::unordered_map<Connection, ULARGE_INTEGER, KeyHasher> &connections, 
				  PWINDIVERT_IPHDR ip_header, PWINDIVERT_TCPHDR tcp_header, PWINDIVERT_UDPHDR udp_header,
				  int timeout){
	FILETIME file_time;
	ULARGE_INTEGER curr_time;
	ULARGE_INTEGER last_time;
	bool send_auth_packet = false;
	Connection conn;
	//its TCP
	if(ip_header->Protocol == IPPROTO_TCP && tcp_header != NULL){
		conn.source_port = ntohs(tcp_header->SrcPort);
		conn.dest_port = ntohs(tcp_header->DstPort);
	}
	//its UDP
	
	else if( ip_header->Protocol == IPPROTO_UDP && udp_header != NULL){
		conn.source_port = ntohs(udp_header->SrcPort);
		conn.dest_port = ntohs(udp_header->DstPort);
	}
	else{
		//bad protocol in IPv4 header
		return false;
	}

	//set headers from IP header
	conn.source_ip = ip_header->SrcAddr;
	conn.dest_ip = ip_header->DstAddr;
	conn.protocol = ip_header->Protocol;

	//get local time
	GetSystemTimeAsFileTime(&file_time);	
	curr_time.LowPart = file_time.dwLowDateTime;
	curr_time.HighPart = file_time.dwHighDateTime;

	//debug
	CLogger::debug("List of stored connections: ");
	for(auto const &it: connections){
		it.first.print();
	}
	
	// try to find connection
	if(connections.count(conn)){
		g_mutex.lock();
		last_time = connections[conn];
		g_mutex.unlock();
		CLogger::debug("Found old connection.");
		conn.print();

		// if delay between last send packet and current packet is less then 10 seconds, 
		// skip sending auth packet
		// TODO: load value from config
		if ((curr_time.QuadPart - last_time.QuadPart) < timeout * 10000000){
			send_auth_packet = false;
		}else{
			g_mutex.lock();
			connections[conn] = curr_time;
			g_mutex.unlock();
			send_auth_packet = true;

		}
	}else{
		// if its a new connection, store current time
		CLogger::debug("NEW CONNECTION");
		// store time
		g_mutex.lock();
		connections[conn] = curr_time;
		g_mutex.unlock();
		send_auth_packet = true;
	}
	return send_auth_packet;
}
bool test_permissions(std::wstring configDir){
	if(_waccess_s((configDir + L"\\counter").c_str(), 6) || _waccess_s((configDir + L"\\config").c_str(), 6)||
		_waccess_s((configDir + L"\\cipher.key").c_str(), 6)|| _waccess_s((configDir + L"\\mac.key").c_str(), 6)){
		return false;
	}
	return true;
}
// this function will search all directories in ~/.knockknock/ folder
// all directories in this folder are used as stored profiles
std::vector<std::wstring> list_directories(){

	std::vector<std::wstring> directories;
	WIN32_FIND_DATAW ffd;
	wchar_t home[MAX_PATH+1];

	//get path of this executable
	DWORD length = GetModuleFileNameW(NULL, home, MAX_PATH + 1);

	//strip name of the executable
    PathRemoveFileSpecW(home);

	//append configuration directory
	StringCchCatW(home, MAX_PATH+1, L"\\conf\\");

	std::wstring sHome(home);

	HANDLE hFind = FindFirstFileW((sHome + L"*").c_str(), &ffd);

	if (hFind == INVALID_HANDLE_VALUE){
		CLogger::error("Error while opening user config directories.");
		return directories;
	}

	// store all directories, except . and ..
	do {
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
			wcscmp(ffd.cFileName, L".") && wcscmp(ffd.cFileName, L".."))
		{
			if(!test_permissions(sHome + ffd.cFileName)){
				//unreadable, unwritable or nonexisting files
				CLogger::error("Exisitng configuration directory but files in it are missing, unreadable or unwritable.");
				continue;
			}
			directories.push_back(ffd.cFileName);
			std::wstring message(L"Found configuration directory: ");
			message += ffd.cFileName;
			CLogger::debug(message);
		}
	} while (FindNextFileW(hFind, &ffd) != 0);
	return directories;
}


std::string construct_filter(std::vector<std::wstring> directories, std::map<std::string, std::string> &ip_or_hostname){
	if (!directories.size()){
		CLogger::error("No directories with configuration were found.");
		exit(EXIT_FAILURE);
	}
	UINT32 addr;
	struct hostent *remoteHost;
	WSADATA wsaData;
	std::vector<std::string> ips;
    int error = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (error != 0)
    {
		CLogger::error("Error while initializing WinSockets. Maybe unsopported version of Windows.");
		exit(EXIT_FAILURE);
    }
	//for all configuration directories which defines IP or hostname
	for(unsigned int i = 0; i < directories.size(); i++){

		//retrieve widechar IP or hostname
		std::wstring ip = directories.at(i);

		//buffer for IP or hostname in ASCII
		char asciiIP[257];
		size_t charsConverted = 0;

		//try to convert widechar IP or hostname to ASCII
		if(wcstombs_s(&charsConverted, asciiIP, 256, ip.c_str(), ip.length())){

			// it failed, maybe it is hostname in unicode
			// try to convert it to punny code, which is made up from ASCII chars
			wchar_t decodedDomainName[257];
			int charsConverted2 = IdnToAscii(0, ip.c_str(), ip.length(), decodedDomainName, 256); 
			if(!charsConverted2){
				//it failed to convert to puny code, probably its some trash
				continue;
			}
			//append trailing null character
			decodedDomainName[charsConverted2]='\0';

			//try to convert hostname in puny code from widechar to ASCII chars
			if(wcstombs_s(&charsConverted, asciiIP, 256, decodedDomainName, 256)){
				continue;
			}
		}

		// try to convert IPv4 address
		if(!WinDivertHelperParseIPv4Address(asciiIP, &addr)){

			//if it isnt IPv4, try to resolve hostname
			remoteHost = gethostbyname(asciiIP);
			CLogger::debug(L"Detected possible hostname: " + ip);

			// hostname cant be translated, skip it
			if(remoteHost == NULL){
				CLogger::debug("Can't translate hostname to IPv4, skipping...");
				continue;
			}else{
				// get list of ipv4 addresses from hostname
				struct in_addr **addr_list;
				addr_list = (struct in_addr **) remoteHost->h_addr_list;
				std::string resolvedIP;
				//get ip as char *
				for(int i = 0; addr_list[i] != NULL; i++){
					resolvedIP = inet_ntoa(*addr_list[i]);
					//we have IP with hostname
					ip_or_hostname[resolvedIP] = remoteHost->h_name;
					ips.push_back(resolvedIP);
				}
			}
		}else{
			// just placeholder (we have IP not hostname)
			ip_or_hostname[std::string(asciiIP)] = "NULL";
			ips.push_back(std::string(asciiIP));
		}
	}


	std::string filter = "ip and outbound and (tcp or udp) and (";
	// construct filter from stored ips
	for(unsigned int i = 0; i < ips.size(); i++)
	{
		CLogger::debug("Resolved IPv4: " + ips[i]);
		if(i == (ips.size() - 1)){
			filter += "ip.DstAddr == ";
			filter += ips[i];
		}else{
			filter += "ip.DstAddr == ";
			filter += ips[i];;
			filter += " or ";
		}
	}
	filter += ")";
	WSACleanup();
	return filter;
}

std::map<std::string, Config> read_configs(std::vector<std::wstring> directories){

	wchar_t home[MAX_PATH+1];

	//get path of this executable
	DWORD length = GetModuleFileNameW(NULL, home, MAX_PATH + 1);

	//strip name of the executable
    PathRemoveFileSpecW(home);


	std::wstring root_config_dir(home);
	root_config_dir += L"\\conf\\";
	std::map<std::string, Config> configs;
	for(unsigned int i = 0; i < directories.size(); i++){
		//retrieve widechar IP or hostname
		std::wstring config_dir = root_config_dir + directories.at(i);
		config_dir += L"\\config";

		int auth_packet_delay = GetPrivateProfileIntW(L"main", L"auth_packet_delay", 100,  config_dir.c_str());
		int auth_packet_timeout = GetPrivateProfileIntW(L"main", L"delay", 10,  config_dir.c_str());
		int legacy = GetPrivateProfileIntW(L"main", L"moxie", 0,  config_dir.c_str());

		bool bLegacy;
		if (legacy){
			bLegacy=true;
			CLogger::debug("Parsed legacy value: true");
		}else
		{
			bLegacy=false;
			CLogger::debug("Parsed legacy value: false");
		}

		if (auth_packet_delay <= 0 || auth_packet_timeout <= 0 ){
			CLogger::error("Bad config values, setting defaults.");
			auth_packet_delay = 100;
			auth_packet_timeout = 10;
		}

		char asciiIP[257];
		size_t charsConverted = 0;

		//try to convert widechar IP or hostname to ASCII
		if(wcstombs_s(&charsConverted, asciiIP, 256, directories.at(i).c_str(), directories.at(i).length())){

			// it failed, maybe it is hostname in unicode
			// try to convert it to punny code, which is made up from ASCII chars
			wchar_t decodedDomainName[257];
			int charsConverted2 = IdnToAscii(0, directories.at(i).c_str(), directories.at(i).length(), decodedDomainName, 256); 
			if(!charsConverted2){
				//it failed to convert to puny code, probably its some trash
				continue;
			}
			//append trailing null character
			decodedDomainName[charsConverted2]='\0';

			//try to convert hostname in puny code from widechar to ASCII chars
			if(wcstombs_s(&charsConverted, asciiIP, 256, decodedDomainName, 256)){
				continue;
			}
		}
		configs[std::string(asciiIP)] = Config(auth_packet_delay, auth_packet_timeout, bLegacy);
	}

	return configs;
}

// thread will iterate through connection map and delete old connection
// thread will sleep for 60s and then deletes connections older than 120s
void clear_connections(std::unordered_map<Connection, ULARGE_INTEGER, KeyHasher> *connections){
	FILETIME file_time;
	ULARGE_INTEGER curr_time;
	ULARGE_INTEGER last_time;
	
	// timeout of 120s
	const int timeout = 1200000000;
	while (true){
		// sleep for 60s
		Sleep(60000);
		for(auto it = connections->cbegin(); it != connections->cend();){	
			//get local time
			GetSystemTimeAsFileTime(&file_time);	
			curr_time.LowPart = file_time.dwLowDateTime;
			curr_time.HighPart = file_time.dwHighDateTime;
			last_time.QuadPart = it->second.QuadPart;

			//if connection is old, delete it
			if((curr_time.QuadPart - last_time.QuadPart) >= timeout){
				g_mutex.lock();
				it = connections->erase(it);
				g_mutex.unlock();
				CLogger::debug("Clearing connection...");
			}else{
				it++;
			}
		}
	}
}

int start(int argc, char* argv[], HANDLE stopService)
{
	// handle for network device to open, read and send packets
	HANDLE handle;

	// buffer for incoming packet, size is 65535B
	unsigned char packet[MAXBUF];
    UINT packet_len;

	// structures for packet header
    WINDIVERT_ADDRESS addr;
    PWINDIVERT_IPHDR ip_header;
    PWINDIVERT_TCPHDR tcp_header;
    PWINDIVERT_UDPHDR udp_header;


	// structure for authentication packet
	TCPPACKET auth_packet0;
    PTCPPACKET auth_packet = &auth_packet0;

	// helper buffers
	char buffer[256];
	char test_buffer[256];

	DWORD bytes_read = 0;

	// init pseudorandom generator, only used for source port of authentication packets
	std::default_random_engine generator(time(0));
	std::uniform_int_distribution<UINT16> distribution(49152, 65535);

	// create overlapped structure for Async read of packets
	// handleArray has two handles - one will signal incoming packets
	//							   - the other will signal Stop command from service
	OVERLAPPED op;
	ZeroMemory(&op, sizeof(op));
	HANDLE handleArray[2];
	handleArray[0] = op.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	handleArray[1] = stopService;
	DWORD which_event;

	// map with pairs [ip, hostname]
	// if ip doesnt have hostname, values is "NULL"
	std::map<std::string, std::string> ip_or_hostname;

	// list of directories with user defined hosts
	std::vector <std::wstring> directories = list_directories();

	// construct filter for capturing outgoing trafic
	std::string filter = construct_filter(directories, ip_or_hostname);

	// read configs
	std::map<std::string, Config> configs = read_configs(directories);

	// map with pairs [connection, timestamp]
	std::unordered_map<Connection, ULARGE_INTEGER, KeyHasher> connections;

	// create and start cleaner thread
	std::thread cleaner(clear_connections, &connections);
	cleaner.detach();

	PyObject * pModule;

	// this value decides if authentication packet will be send or not
	bool send_auth_packet = false;

	CLogger::debug("Constructed filter for detecting outgoing traffic: " + filter);

    // Divert traffic matching the filter:
	handle = WinDivertOpen(filter.c_str(), WINDIVERT_LAYER_NETWORK, 0, 0);
    if (handle == INVALID_HANDLE_VALUE)
    {
        CLogger::error("error: failed to open the WinDivert device " + GetLastError());
        exit(EXIT_FAILURE);
    }

    // Max-out the packet queue:
    if (!WinDivertSetParam(handle, WINDIVERT_PARAM_QUEUE_LEN, 8192))
    {
        CLogger::error("error: failed to set packet queue length " + GetLastError());
        exit(EXIT_FAILURE);
    }
	// max-ou queue time
    if (!WinDivertSetParam(handle, WINDIVERT_PARAM_QUEUE_TIME, 2048))
    {
        CLogger::error("error: failed to set packet queue time " + GetLastError());
        exit(EXIT_FAILURE);
    }

	// reset authentication packet and set default values (TCP SYN, Dont Fragment, ...)
	PacketIpTcpInit(auth_packet);

	// init python module
	CLogger::info("Initializing Python module.");	
	PyObject * pFunc =  PythonCaller::python_init(argc, argv, &pModule);


	CLogger::info("Starting to catch packets...");

	int cnt=0;

	//check_ip_address();
	while(true){
		send_auth_packet = false;

		// Read a matching packet.
		if (!WinDivertRecvEx(handle, packet, sizeof(packet), 0 ,&addr, &packet_len, &op))
		{
			DWORD err = GetLastError();
			if(ERROR_IO_PENDING != err){
				CLogger::error("warning: failed to read packet " + err);
				continue;
			}
		}

		// wait for event either for incoming packet or stop event
		which_event = WaitForMultipleObjects(2, handleArray, FALSE, INFINITE);

		//incoming packet
		if(which_event == WAIT_OBJECT_0){
			//reset event 
			ResetEvent(op.hEvent);

			if (!GetOverlappedResult(handle, &op, &bytes_read, TRUE)){
				continue;
			}
			packet_len = (UINT)bytes_read;
       
			// Parse packet
			WinDivertHelperParsePacket(packet, packet_len, &ip_header,
				NULL, NULL, NULL, &tcp_header,
				&udp_header, NULL, NULL);

			// received packet is TCP/IP (or UDP/IP)
			if(ip_header != NULL && (tcp_header != NULL || udp_header != NULL)){

				// get destination address
				UINT8 *dst_addr = (UINT8 *)&ip_header->DstAddr;

				//convert UINT32 IP to char[]
				_snprintf_s(test_buffer, sizeof(test_buffer), sizeof(test_buffer) - 1, "%hhu.%hhu.%hhu.%hhu\0", dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3]);

				bool hostname = false;
				Config config;

				// decide if hostname or not
				// load config
				if (ip_or_hostname[test_buffer] != "NULL"){
					hostname = true;
					config = configs[ip_or_hostname[test_buffer]];
				}else{
					config = configs[test_buffer];
				}

				g_legacy = config.legacy;

				// decide if auth packet will be send
				send_auth_packet = compare_time(connections, ip_header, tcp_header, udp_header, config.auth_packet_timeout);

				if(send_auth_packet){
					// this value controls if auth packet shouldnt be sent
					bool dont_send = false;

					if(g_legacy){
						CLogger::debug("Legacy enabled.");
					}else{
						CLogger::debug("Legacy disabled.");
					}

					// detect if profile is set using ip or hostname
					if(hostname){
						// create string for python module in format: [port i want to open, hostname, legacy mode]
						if(tcp_header != NULL){
							_snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, "%hu %s %d\0",
								ntohs(tcp_header->DstPort), ip_or_hostname[test_buffer].c_str(), config.legacy);
						}
						else if (udp_header != NULL){
							_snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1,"%hu %s %d\0",
								ntohs(udp_header->DstPort), ip_or_hostname[test_buffer].c_str(), config.legacy);
						}
					}
					else{
						// create string for python module in format: [port i want to open, ip, legacy mode]
						if(tcp_header != NULL){
						_snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, "%hu %hhu.%hhu.%hhu.%hhu %d\0",
							ntohs(tcp_header->DstPort), dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3], config.legacy);
						}
						else if(udp_header != NULL){
						_snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, "%hu %hhu.%hhu.%hhu.%hhu %d\0",
							ntohs(udp_header->DstPort), dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3], config.legacy);
						}
					}

					// call python module and get result (encrypted header field)
					std::string packetInfo = PythonCaller::get_packet_header(buffer, pFunc);

					//python module failed to get us result, skip this
					if (packetInfo == "" || packetInfo.empty()){
						dont_send = true;
					}

					// parse result from python module
					std::map <std::string, UINT32> parsed_line = parse_result(packetInfo);

					// fill authentication packet with right values
					fill_packet(auth_packet, ip_header, parsed_line, config.legacy, std::ref(generator), distribution);
			
					CLogger::info("sending auth packet:" + packetInfo);

					//send auth packet
					if(!dont_send){
						if (!WinDivertSend(handle, (PVOID)auth_packet, sizeof(TCPPACKET),
								&addr, NULL))
						{
							CLogger::error("warning: failed to send auth packet: " + GetLastError());
						}
					}
					Sleep(config.auth_packet_delay);
				}

				// send original packet
				CLogger::info("sending original packet...");
				if (!WinDivertSend(handle, (PVOID)packet, packet_len,
						&addr, NULL))
				{
					CLogger::error("warning: failed to send original packet: " + GetLastError());
				}
			}
#ifdef _DEBUG
			cnt++;	
			if(cnt>100){
				break;
			}
#endif
		}

		// i received closing signal
		else if(which_event == (WAIT_OBJECT_0 + 1)){
			// jump out of while cycle
			break;
		}
		else if (which_event == WAIT_TIMEOUT){
			continue;
		}
		else{
			//something went wrong
			continue;
		}

	}

	//cleanup
	WinDivertClose(handle);
	PythonCaller::python_cleanup(pModule, pFunc);
	CLogger::info("Gracefully closing...");
	_CrtDumpMemoryLeaks();  
	return 0;
}

