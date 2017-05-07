// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _DEBUG
#include <vld.h>
#endif

#include <stdio.h>
#include <tchar.h>



#include <windows.h>
#include <iostream>
#include <winsock2.h>
#include <mutex>
#include <thread>
#include <iterator>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <strsafe.h>
#include <ShlObj.h>
#include <regex>
#include <strsafe.h>
#include <assert.h>
#include <memory>
#include <fstream>
#include <random>
#include <iphlpapi.h>
#include <Shlwapi.h>
// TODO: reference additional headers your program requires here

#include "Connection.h"
#include "KeyHasher.h"
#include "windivert.h"
#include "NetworkHeaders.h"
#include "PythonCaller.h"
#include "Config.h"
#include "knocker.h"
#include "Logger.h"