#pragma once
#ifdef _DEBUG
  #undef _DEBUG
  #include <Python.h>
  #define _DEBUG
#else
  #include <Python.h>
#endif
class PythonCaller
{
public:
	static PyObject * python_init(int argc, char * argv[], PyObject **pModule);
	static void python_cleanup(PyObject* pModule, PyObject* pFunc);
	static std::string get_packet_header(char* buffer, PyObject * pFunc);
	static std::map<std::string, UINT32> parse_result(std::string packetInfo);
private:
	PythonCaller(void) {};
};

