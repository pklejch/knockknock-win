#include "stdafx.h"
#include "PythonCaller.h"


PyObject* PythonCaller::python_init(int argc, char * argv[], PyObject **pModule){
			PyObject *pName, *pDict, *pFunc;
			Py_NoSiteFlag = 1;
			Py_Initialize();
			CLogger::debug("Python initialized.");

			char home[MAX_PATH+1];

			//get path of this executable
			DWORD length = GetModuleFileNameA(NULL, home, MAX_PATH + 1);

			//strip name of the executable
			PathRemoveFileSpecA(home);

			if (argc != 0 || argv != NULL){
				PySys_SetArgvEx(argc, argv, 1);
			}



			std::string sHome(home);
			std::string cmd;//sys.path.append('packages');
			cmd = "import sys;  sys.path.append('.');";
			cmd += " sys.path.append('"+sHome+"'); sys.path.append('"+sHome+"\\packages'); sys.stderr = open('"+sHome+"\\python.log','w');";
			CLogger::debug(cmd);
			PyRun_SimpleString(cmd.c_str());

			CLogger::debug("Python Path updated.");

			Py_SetPythonHome((char*)(sHome+"\\packages").c_str());
			//load module get_header
			pName = PyString_FromString("get_header");

			//try to load module
			*pModule = PyImport_Import(pName);
			if (*pModule == NULL){
				CLogger::error("Cannot find python module.");
				PyErr_Print();
				exit(EXIT_FAILURE);
			}
			CLogger::debug("Module found.");

			Py_DECREF(pName);
			pDict = PyModule_GetDict(*pModule);
			pFunc = PyDict_GetItemString(pDict, "main");

			CLogger::debug("Function in the module was found.");
			return pFunc;
}

void PythonCaller::python_cleanup(PyObject* pModule, PyObject* pFunc){
			// Clean up
			Py_DECREF(pModule);
			Py_DECREF(pFunc);

			// Finish the Python Interpreter
			Py_Finalize();
}

std::string PythonCaller::get_packet_header(char* buffer, PyObject * pFunc){
			std::string packetInfo;
			PyObject *pRetValue = NULL;
			PyObject* pArgs = PyTuple_New(1);

			PyTuple_SetItem(pArgs, 0, PyString_FromString(buffer));
			

			if (PyCallable_Check(pFunc)) 
			{
				pRetValue = PyObject_CallObject(pFunc, pArgs);
			} else 
			{
				PyErr_Print();
				exit(EXIT_FAILURE);
			}
			Py_DECREF(pArgs);

			// i have successful result from Python
			if(pRetValue != NULL){
				packetInfo = PyString_AsString(pRetValue);
				Py_DECREF(pRetValue);
			}

			return packetInfo;
}