#include "stdafx.h"



// 
// Settings of the service
// 

// Internal name of the service
#define SERVICE_NAME             "Knocker"

// Displayed name of the service
#define SERVICE_DISPLAY_NAME     "Auto Knocker"

// Service start options.
#define SERVICE_START_TYPE       SERVICE_DEMAND_START

// List of service dependencies - "dep1\0dep2\0\0"
#define SERVICE_DEPENDENCIES     "\0"

// The name of the account under which the service should run
#define SERVICE_ACCOUNT          ".\\LocalSystem"

// The password to the service account name
#define SERVICE_PASSWORD         NULL

HANDLE stop;

// static members of Logger class
bool CLogger::m_foreground = false;
unsigned int CLogger::m_debug_level = 0;
std::wofstream CLogger::m_log_file;

BOOL WINAPI consoleHandler(DWORD signal) {

    if (signal == CTRL_C_EVENT){
		SetEvent(stop);
	}

    return TRUE;
}

int main(int argc, char *argv[])
{
    if ((argc > 1) && ((*argv[1] == '-' || (*argv[1] == '/'))))
    {
		if (strcmp("install", argv[1] + 1) == 0)
        {
            // Install the service when the command is 
            // "-install" or "/install".
            InstallService(
                SERVICE_NAME,               // Name of service
                SERVICE_DISPLAY_NAME,       // Name to display
                SERVICE_START_TYPE,         // Service start type
                SERVICE_DEPENDENCIES,       // Dependencies
                SERVICE_ACCOUNT,            // Service running account
                SERVICE_PASSWORD            // Password of the account
                );
        }
        else if (strcmp("remove", argv[1] + 1) == 0)
        {
            // Uninstall the service when the command is 
            // "-remove" or "/remove".
            UninstallService(SERVICE_NAME);
        }

		// foreground mode
		else if(!strcmp("-f",argv[1])){

			// set Ctrl+C event to close
			stop = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (!SetConsoleCtrlHandler(consoleHandler, TRUE)) {
				fprintf(stderr,"ERROR: Could not set control handler\n"); 
				return EXIT_FAILURE;
			}
			
			CLogger::set(true, "", 3);
			//start main loop
			start(argc, argv, stop);
		}
    }
    else
    {
        printf("Parameters:\n");
        printf(" -install  to install the service.\n");
        printf(" -remove   to remove the service.\n");


		//TODO set logging file from config file
		CLogger::set(false, "C:\\log.log", 3);

        CSampleService service(SERVICE_NAME, argc, argv);
        if (!CServiceBase::Run(service))
        {
            CLogger::error("Service failed to run with error: " + GetLastError());
        }
    }

    return 0;
}