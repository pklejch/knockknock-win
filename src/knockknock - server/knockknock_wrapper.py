import sys
import os
import knockknock_daemon
import time
import subprocess
import knockknock.Profile
import logging


def get_dir():
    if getattr(sys, 'frozen', False):
        # If the application is run as a bundle, the pyInstaller bootloader
        # extends the sys module by a flag frozen=True and sets the app
        # path into variable _MEIPASS'.
        application_path = sys._MEIPASS
        frozen = True
    else:
        application_path = os.path.dirname(os.path.abspath(__file__))
        frozen = False
    return [application_path, frozen]

def test_config_directory(config_dir):
    if os.access(os.path.join(config_dir, "config"), 6) \
        and os.access(os.path.join(config_dir, "counter"), 6) \
        and os.access(os.path.join(config_dir, "mac.key"), 6) \
        and os.access(os.path.join(config_dir, "cipher.key"), 6):
        return True
    else:
        return False

def get_knockports_from_profiles(conf_dir):
    ports = list()
    directory = os.path.join(conf_dir, "conf", "profiles")

    for item in os.listdir(directory):
        config_dir = os.path.join(directory, item)
        if os.path.isdir(config_dir) and test_config_directory(config_dir):
            ports.append(knockknock.Profile.Profile(os.path.join(directory, item)).getKnockPort())

    return ports


def main(stop_event):
    DIRECTORY, isFrozen = get_dir()

    logging.debug("Running from "+DIRECTORY)
    ports = get_knockports_from_profiles(DIRECTORY)

    # run logger
    logging.info("Running logger...")
    logger_exe = [os.path.join(DIRECTORY, "logger", "logger.exe")]
    subprocess.Popen(logger_exe + ports)


    # give logger some time to initialize
    time.sleep(0.2)

    logging.info("Running knockknock daemon...")
    # run port opener daemon
    knockknock_daemon.main(stop_event)

if __name__ == '__main__':
    main(None)
