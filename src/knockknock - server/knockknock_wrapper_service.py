import win32serviceutil
import win32service
import win32event
import servicemanager
import socket
import sys
import knockknock_wrapper
import logging
import os


def getDir():
    if getattr(sys, 'frozen', False):
        # If the application is run as a bundle, the pyInstaller bootloader
        # extends the sys module by a flag frozen=True and sets the app
        # path into variable _MEIPASS'.
        application_path = sys._MEIPASS
    else:
        application_path = os.path.dirname(os.path.abspath(__file__))
    return application_path

class KnockknockService(win32serviceutil.ServiceFramework):
    _svc_name_ = "KnockknockDaemon"
    _svc_display_name_ = "Knockknock Daemon"

    def __init__(self, args):
        win32serviceutil.ServiceFramework.__init__(self, args)
        self.stop_event = win32event.CreateEvent(None, 0, 0, None)
        socket.setdefaulttimeout(60)

    def SvcStop(self):
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)
        win32event.SetEvent(self.stop_event)

    def SvcDoRun(self):
        servicemanager.LogMsg(servicemanager.EVENTLOG_INFORMATION_TYPE,
                              servicemanager.PYS_SERVICE_STARTED,
                              (self._svc_name_, ''))

        knockknock_wrapper.main(self.stop_event)

if __name__ == '__main__':
    if len(sys.argv) == 1:
        directory = getDir()
        logging.basicConfig(filename=os.path.join(directory, 'server.log'), level=logging.DEBUG)
        logging.info("Starting daemon mode.")
        servicemanager.Initialize()
        servicemanager.PrepareToHostSingle(KnockknockService)
        servicemanager.StartServiceCtrlDispatcher()
    elif len(sys.argv) == 2 and sys.argv[1] == "-f":
        knockknock_wrapper.main(None)
    else:
        win32serviceutil.HandleCommandLine(KnockknockService)