#!/usr/bin/env python
__author__ = "Moxie Marlinspike"
__email__ = "moxie@thoughtcrime.org"
__license__ = """
Copyright (c) 2009 Moxie Marlinspike <moxie@thoughtcrime.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA

"""

import os
import sys
import socket

from struct import pack, unpack
from knockknock.Profile import Profile



def getProfile(host, legacy):
    homedir = os.path.dirname(os.path.abspath(__file__))
    if not os.path.isdir(homedir + '\\conf\\'):
        sys.stderr.write("Error: you need to setup your profiles in " + homedir + '\\conf\\')
        return None

    if not os.path.isdir(homedir + '\\conf\\' + host):
        sys.stderr.write('Error: profile for host ' + host + ' not found at ' + homedir + '\\conf\\' + host)
        return None

    return Profile(homedir + '\\conf\\' + host, legacy=legacy)


def get_source_ip():
    import urllib2
    import json
    req = urllib2.urlopen("http://httpbin.org/ip")
    content = req.read()
    json_content = json.loads(content)
    ip = json_content['origin']
    return ip

def addressInNetwork(ip, ip2, mask):
   "Is an address in a network"
   mask = unpack('I', socket.inet_aton(mask))[0]
   ipaddr = unpack('I',socket.inet_aton(ip))[0] & mask
   ipaddr2 = unpack('I',socket.inet_aton(ip2))[0] & mask
   return ipaddr == ipaddr2

def check_nat(source_ip):
    import netifaces
    from netifaces import AF_INET
    interfaces = netifaces.interfaces()
    for interface in interfaces:
        try:
            addresses = netifaces.ifaddresses(interface)[AF_INET]
        except KeyError:
            #interface doesnt have addr
            continue
        for addr in addresses:
            try:
                ipv4 = addr['addr']
                if source_ip == ipv4:
                    return False
            except KeyError:
                continue

    return True

def get_ip_from_same_network(host):
    dest_ip = socket.gethostbyname(host)
    import netifaces
    from netifaces import AF_INET
    interfaces = netifaces.interfaces()
    for interface in interfaces:
        try:
            addresses = netifaces.ifaddresses(interface)[AF_INET]
        except KeyError:
            continue
        for addr in addresses:
            try:
                ipv4 = addr['addr']
            except KeyError:
                continue
            # there's a bug in netifaces module, which always returns 255.255.255.255 mask on windows
            # so hardcode 255.255.255.0 for now
            if addressInNetwork(dest_ip, ipv4, "255.255.255.0"):
                return addr['addr']
    return None


def main(port_host):
    (port, host, legacy) = port_host.split(" ")

    legacy = bool(int(legacy))
    profile = getProfile(host, legacy)


    if profile is None:
        return ""
    profile.cryptoEngine.legacy = legacy


    port = pack('!H', int(port))

    if not legacy:
        ip = profile.get_source_ip()
        if ip.lower() == "auto":
            # get public source ip
            ip = get_source_ip()
            # check if you have this public ip assigned to your NIC
            nat = check_nat(ip)

            # if you are behind NAT, check which of your assigned IP is in same network as target IP
            if nat:
                local_ip = get_ip_from_same_network(host)

                # target ip is not in same network as your computer
                if local_ip != None:
                    ip = local_ip
        try:
            ip_int = socket.inet_aton(ip)
            port += ip_int
        except socket.error:
            return ""



    packetData = profile.encrypt(port)

    knockPort = profile.getKnockPort()

    if legacy:
        (idField, seqField, ackField, winField) = unpack('!HIIH', packetData)
    else:
        (idField, seqField, ackField, winField, urgPointerField) = unpack('!HIIHH', packetData)


    if legacy:
        retValue = "DPORT=" + str(knockPort) + " ID=" + str(idField) + \
                   " WINDOW=" + str(winField) + " SEQ=" + str(seqField) + \
                   " ACK=" + str(ackField)
    else:
        retValue = "DPORT=" + str(knockPort) + " ID=" + str(idField) + \
                   " WINDOW=" + str(winField) + " SEQ=" + str(seqField) + \
                   " ACK=" + str(ackField) + " URG=" + str(urgPointerField)
    return retValue

if __name__ == '__main__':
    print(main(sys.argv[1]))