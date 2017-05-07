import os
import sys

import knockknock.Profiles
import knockknock.PortOpener
import knockknock.DaemonConfiguration
import knockknock.KnockWatcher
import logging
import knockknock_wrapper_service

def getDir():
    if getattr(sys, 'frozen', False):
        # If the application is run as a bundle, the pyInstaller bootloader
        # extends the sys module by a flag frozen=True and sets the app
        # path into variable _MEIPASS'.
        application_path = sys._MEIPASS
    else:
        application_path = os.path.dirname(os.path.abspath(__file__))
    return application_path

DIRECTORY = getDir()


def checkConfiguration():
    if not os.path.isdir(os.path.join(DIRECTORY, "conf")):
        logging.info("profiles directory does not exist.  You need to setup your profiles first..")
        sys.exit(3)


def main(stop_event):
    checkConfiguration()

    profiles_dir = os.path.join(DIRECTORY, "conf", "profiles")
    config_file = os.path.join(DIRECTORY, "conf", "config")

    logging.debug("Loading daemon config...")
    config = knockknock.DaemonConfiguration.DaemonConfiguration(config_file)

    legacy = config.legacy

    if legacy:
        logging.debug("Moxie compatibility mode is ON")
    else:
        logging.debug("Moxie compatibility mode is OFF")
    logging.debug("Loading profiles...")
    profiles = knockknock.Profiles.Profiles(profiles_dir, legacy)



    if profiles.isEmpty():
        logging.warning('WARNING: Running knockknock-daemon without any active profiles.')

    port_opener = knockknock.PortOpener.PortOpener(config.getDelay())

    knock_watcher = knockknock.KnockWatcher.KnockWatcher(config, profiles,
                                                         port_opener, stop_event)
    port_opener.start()
    knock_watcher.tailAndProcess()

if __name__ == '__main__':
    main(None)