#pragma once
class CLogger
{
public:
	CLogger();
	~CLogger(void);
	static void debug(const std::string message);
	static void info(const std::string message);
	static void error(const std::string message);
	static void debug(const std::wstring message);
	static void info(const std::wstring message);
	static void error(const std::wstring message);
	static void set(bool foreground, const std::string path, unsigned int debug_level);
	static void set(bool foreground, const std::wstring path, unsigned int debug_level);
private:
	static void write_message(const std::string message);
	static void write_message(const std::wstring message);
	static bool m_foreground;
	static unsigned int m_debug_level;
	static std::wofstream m_log_file;
};

