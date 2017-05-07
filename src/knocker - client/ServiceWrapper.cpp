#include "stdafx.h"

#define SERVICE_NAME "Auto Knocker"

HANDLE stop;
// static members of Logger class
bool CLogger::m_foreground = false;
unsigned int CLogger::m_debug_level = 0;
std::wofstream CLogger::m_log_file;
SERVICE_STATUS        g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

BOOL WINAPI consoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT){
		SetEvent(stop);
	}
    return TRUE;
}

VOID WINAPI ServiceCtrlHandler (DWORD CtrlCode)
{
	// i received signal to stop
    if (CtrlCode == SERVICE_CONTROL_STOP) 
	{

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING){
           return;
		}

        //set service to stopping
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        SetServiceStatus (g_StatusHandle, &g_ServiceStatus);

        //send signal to main loop to end
        SetEvent (g_ServiceStopEvent);
    }

}

void control_service(int argc,char ** argv){

    g_StatusHandle = RegisterServiceCtrlHandler (SERVICE_NAME, ServiceCtrlHandler);

    if (g_StatusHandle == NULL) 
    {
		CLogger::error("Unable to register service control handler.");
        return;
    }

    ZeroMemory (&g_ServiceStatus, sizeof (g_ServiceStatus));

	//set service to starting
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus (g_StatusHandle, &g_ServiceStatus) == FALSE) 
    {
		CLogger::error("Cannot set service to running.");
		return;
    }

    // signal which controls when STOP command occurs
    g_ServiceStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL);

    if (g_ServiceStopEvent == NULL) 
    {

		//set service to stopped
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        SetServiceStatus (g_StatusHandle, &g_ServiceStatus);

        return;
    }    

    // set service to running
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    SetServiceStatus (g_StatusHandle, &g_ServiceStatus);


	//start main loop
	start(argc, argv, g_ServiceStopEvent);

	// service stopped
    CloseHandle (g_ServiceStopEvent);

	//change status to stopped
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    SetServiceStatus (g_StatusHandle, &g_ServiceStatus);

    return;
}

int init_service(int argc, char ** argv){

	//create service table
    SERVICE_TABLE_ENTRY ServiceTable[] = 
    {
        {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION) control_service},
        {NULL, NULL}
    };

    return (!StartServiceCtrlDispatcher (ServiceTable));
}

int main(int argc, char ** argv){

	//start in foreground mode
	if (argc >= 2 && !strcmp(argv[1],"-f")){

		// set Ctrl+C event to close
		stop = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
			fprintf(stderr,"ERROR: Could not set control handler\n"); 
			return EXIT_FAILURE;
		}
			
		CLogger::set(true, "", 3);
		start(argc, argv, stop);
	}else{
		//get path of the executable
		wchar_t home[MAX_PATH+1];
		GetModuleFileNameW(NULL, home, MAX_PATH + 1);

		//strip name of the executable
		PathRemoveFileSpecW(home);

		//set logging file into directory with executable
		CLogger::set(false, std::wstring(home) + L"\\log.log", 3);
		//start in service mode
		if(init_service(argc, argv)){
			CLogger::error("Error occured while starting service.");
		}
	}
}