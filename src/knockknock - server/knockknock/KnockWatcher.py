# Copyright (c) 2009 Moxie Marlinspike
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
#

import LogEntry
import MacFailedException
import win32file
import win32event
import win32pipe
import pywintypes
import sys
import socket
import logging
import win32api


class KnockWatcher:

    def __init__(self, config, profiles, port_opener, stop_event):
        self.config = config
        self.profiles = profiles
        self.port_opener = port_opener
        if stop_event is None:
            #dummy event
            self.stop_event = win32event.CreateEvent(None, 0, 0, None)
        else:
            self.stop_event = stop_event

    def tailAndProcess(self):
        overlapped = pywintypes.OVERLAPPED()
        overlapped.hEvent = win32event.CreateEvent(None, 1, 0, None)

        pipe = None
        try:
            buffer2 = win32file.AllocateReadBuffer(4096)
            # only read from pipe
            pipe = win32file.CreateFile(r'\\.\pipe\log', win32file.GENERIC_READ,
                                        0, None, win32file.OPEN_EXISTING, win32file.FILE_FLAG_OVERLAPPED, None)

            if pipe == win32file.INVALID_HANDLE_VALUE:
                logging.error("Cannot read from pipe")
                sys.exit(1)

            logging.debug("Entering main loop...")
            lines = ""
            while True:
                res, buffer2 = win32file.ReadFile(pipe, win32file.AllocateReadBuffer(4096), overlapped)

                # error occured
                if res != win32file.WSA_IO_PENDING and res != 0:
                    continue

                # wait for some signal, either data from pipe or stop signal from service manager
                which_event = win32event.WaitForMultipleObjects([overlapped.hEvent, self.stop_event],
                                                                False, win32event.INFINITE)

                # i received packet
                if which_event == win32event.WAIT_OBJECT_0:
                    win32event.ResetEvent(overlapped.hEvent)
                    logging.debug("Got packet...")

                    # read some bytes
                    num_bytes = win32file.GetOverlappedResult(pipe, overlapped, False)
                    logging.debug("Received: " + str(num_bytes) + " bytes")
                    if not num_bytes:
                        # i have read nothing, skip it
                        continue
                    # store number of read bytes into lines variable
                    lines += str(buffer2[:num_bytes])

                    # if it ends with "\n" i have received whole message
                    if not lines.endswith("\n"):
                        continue

                    # copy message
                    linesFinal = lines

                    # reset message
                    lines = ""

                    # for all lines
                    for line in linesFinal.split('\n'):
                        if line == '':
                            continue
                        logging.debug("Received line from pipe.")
                        logging.debug(line)
                        try:
                            log_entry = LogEntry.LogEntry(line)
                            profile = self.profiles.getProfileForPort(log_entry.getDestinationPort())
                            if profile is not None:
                                try:
                                    if profile.cryptoEngine.legacy:
                                        ciphertext = log_entry.getEncryptedData()
                                        port = profile.decrypt(ciphertext, self.config.getWindow())
                                        source_ip = log_entry.getSourceIP()
                                    else:
                                        ciphertext = log_entry.getEncryptedDataNonLegacy()
                                        source_ip = log_entry.getSourceIP()
                                        port = profile.decrypt(ciphertext + socket.inet_aton(source_ip), self.config.getWindow())

                                    self.port_opener.open_port(source_ip, port)
                                    logging.info("Received authenticated port-knock for port " + str(port) + " from " + source_ip)
                                except MacFailedException.MacFailedException:
                                    pass
                        except MacFailedException.MacFailedException:
                            logging.info("knocknock skipping unrecognized line.")

                # i received stop signal
                elif which_event == win32event.WAIT_OBJECT_0 + 1:
                    logging.debug("Received stop signal.")
                    break
        except pywintypes.error:
            logging.error("Error while reading from pipe, perhaps logger is not running.")

        except Exception as ex:
            logging.critical("Something went wrong")
            logging.critical(ex)
        finally:
            # get PID of logger.exe (other end of pipe)
            pid = win32pipe.GetNamedPipeServerProcessId(pipe)
            logging.debug("PID of logger.exe: " + str(pid))
            if pid is not None:
                PROCESS_TERMINATE = 1
                handle = win32api.OpenProcess(PROCESS_TERMINATE, False, pid)

                # kill it :(
                win32api.TerminateProcess(handle, -1)
                logging.info("Killing logger.exe")
                win32api.CloseHandle(handle)
            if pipe is not None:
                win32file.CloseHandle(pipe)

