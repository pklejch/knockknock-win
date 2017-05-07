#include "stdafx.h"
#include "Logger.h"


CLogger::CLogger()
{
}

void CLogger::set(bool foreground, const std::string path, unsigned int debug_level){
	m_foreground = foreground;
	if(!m_foreground){
		m_log_file = std::wofstream(path);
	}
	m_debug_level = debug_level;
}

void CLogger::set(bool foreground, const std::wstring path, unsigned int debug_level){
	m_foreground = foreground;
	if(!m_foreground){
		m_log_file = std::wofstream(path);
	}
	m_debug_level = debug_level;
}

void CLogger::debug(const std::string message){
	if (m_debug_level > 1){
		write_message("DEBUG: " + message);
	}
}

void CLogger::info(const std::string message){
	if (m_debug_level > 0){
		write_message("INFO: " + message);
	}
}

void CLogger::error(const std::string message){
	write_message("ERROR: " + message);
}

void CLogger::debug(const std::wstring message){
	if (m_debug_level > 1){
		write_message(L"DEBUG: " + message);
	}
}

void CLogger::info(const std::wstring message){
	if (m_debug_level > 0){
		write_message(L"INFO: " + message);
	}
}

void CLogger::error(const std::wstring message){
	write_message(L"ERROR: " + message);
}

void CLogger::write_message(const std::string message){
	if(m_foreground){
		std::cout << message << std::endl;
	}else{
		m_log_file << std::wstring(message.begin(), message.end()) << std::endl;
	}
}

void CLogger::write_message(const std::wstring message){
	if(m_foreground){
		std::wcout << message << std::endl;
	}else{
		m_log_file << message << std::endl;
	}
}

CLogger::~CLogger(void)
{
	if(!m_foreground){
		m_log_file.close();
	}
}
