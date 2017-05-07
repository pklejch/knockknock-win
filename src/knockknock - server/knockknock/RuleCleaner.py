import threading
import time
import subprocess


class RuleCleaner(threading.Thread):
    def __init__(self, cleanup_delay, connections, lock):
        threading.Thread.__init__(self)
        self.daemon = True
        self.connections = connections
        self.cleanup_delay = cleanup_delay
        self.lock = lock

    def run(self):
        while True:
            time.sleep(self.cleanup_delay)

            #load keys
            with self.lock:
                keys = self.connections.keys()

            for key in keys:
                # load last updated time
                with self.lock:
                    clock = self.connections[key]
                clock_now = int(time.time())

                #calculate time difference
                time_diff = clock_now - clock
                # time is up
                if time_diff > self.cleanup_delay:
                    # call remove rule
                    with self.lock:
                        del self.connections[key]
                    source_ip, port = key.split(":")
                    for proto in ["TCP", "UDP"]:
                        command = 'netsh advfirewall firewall delete rule protocol=' + proto + ' ' \
                                  'name="knockknock-' + source_ip + ":" + port + \
                                  '" localport=' + port + ' remoteip=' + source_ip
                        command = command.split()
                        print(command)
                        subprocess.call(command, shell=False)

