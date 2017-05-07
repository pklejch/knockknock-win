#include "stdafx.h"
std::map<std::string, UINT32> parse_result(std::string packetInfo);
bool compare_time(std::unordered_map<Connection, ULARGE_INTEGER, KeyHasher> &connections, 
				  PWINDIVERT_IPHDR ip_header, PWINDIVERT_TCPHDR tcp_header, PWINDIVERT_UDPHDR udp_header, int timeout);
std::vector<std::wstring> list_directories();
std::string construct_filter(std::vector<std::wstring> directories, std::map<std::string, std::string> &ip_or_hostname);
void clear_connections(std::unordered_map<Connection, ULARGE_INTEGER, KeyHasher> *connections);
bool parse_arguments(int argc, char ** argv);
int start(int argc, char* argv[], HANDLE stopService);
bool test_permissions(std::wstring configDir);
std::map<std::string, Config> read_configs(std::vector<std::wstring> directories);