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

import subprocess
import time
import threading


class RuleTimer(threading.Thread):

    def __init__(self, open_duration, source_ip, port):
        self.open_duration = open_duration
        self.source_ip = source_ip
        self.port = port
        threading.Thread.__init__(self)
        self.daemon = True

    def run(self):
        time.sleep(self.open_duration)
        command = 'netsh advfirewall firewall delete rule protocol=TCP ' \
                  'name="knockknock-' + self.source_ip + ":" + str(self.port) + \
                  '" localport=' + str(self.port) + ' remoteip=' + self.source_ip
        command = command.split()
        print(command)

        subprocess.call(command, shell=False)
