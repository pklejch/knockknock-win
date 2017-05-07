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

import os, string
import ConfigParser
import binascii
import stat

import CryptoEngine


class Profile:

    def __init__(self, directory, cipherKey=None, macKey=None, counter=None, knockPort=None, legacy=True):
        self.legacy = legacy
        self.counterFile  = None
        self.directory    = directory
        self.name         = directory.rstrip('/').split('/')[-1]
        if (cipherKey == None):
            self.deserialize()
        else:
            self.cipherKey = cipherKey
            self.macKey    = macKey
            self.counter   = counter
            self.knockPort = knockPort

        self.cryptoEngine = CryptoEngine.CryptoEngine(self, self.cipherKey, self.macKey, self.counter)
        self.cryptoEngine.legacy = legacy

    def deserialize(self):
        self.cipherKey    = self.loadCipherKey()
        self.macKey       = self.loadMacKey()
        self.counter      = self.loadCounter()
        if self.legacy:
            self.knockPort = self.loadConfig()
        else:
            [self.knockPort, self.source_ip] = self.loadConfig()

    def serialize(self):
        self.storeCipherKey()
        self.storeMacKey()
        self.storeCounter()
        self.storeConfig()

    # Getters And Setters

    def getIPAddrs(self):
        return self.ipAddressList

    def setIPAddrs(self, ipAddressList):
        self.ipAddressList = ipAddressList        

    def getName(self):
        return self.name

    def getDirectory(self):
        return self.directory

    def getKnockPort(self):
        return self.knockPort

    def get_source_ip(self):
        return self.source_ip

    def setCounter(self, counter):
        self.counter = counter

    # Encrypt And Decrypt

    def decrypt(self, ciphertext, windowSize):
        if self.cryptoEngine.legacy:
            return self.cryptoEngine.decrypt_legacy(ciphertext, windowSize)
        else:
            return self.cryptoEngine.decrypt(ciphertext, windowSize)

    def encrypt(self, plaintext):
        if self.cryptoEngine.legacy:
            return self.cryptoEngine.encrypt_legacy(plaintext)
        else:
            return self.cryptoEngine.encrypt(plaintext)

    # Serialization Methods

    def loadCipherKey(self):
        return self.loadKey(self.directory + "/cipher.key")

    def loadMacKey(self):
        return self.loadKey(self.directory + "/mac.key")

    def loadCounter(self):
        # Privsep bullshit...
        if (self.counterFile == None):
            self.counterFile = open(self.directory + "/counter", 'r+')

        counter = self.counterFile.readline()
        counter = counter.rstrip("\n")

        return int(counter)

    def loadConfig(self):
        config = ConfigParser.SafeConfigParser()
        config.read(self.directory + "/config")
        print(self.directory)
        knock_port = config.get('main', 'knock_port')
        if not self.legacy:
            source_ip = config.get('main', 'source_ip')
            return [knock_port, source_ip]
        else:
            return knock_port

    def loadKey(self, keyFile):
        file = open(keyFile, 'r')
        key  = binascii.a2b_base64(file.readline())        

        file.close()
        return key

    def storeCipherKey(self):        
        self.storeKey(self.cipherKey, self.directory + "/cipher.key")

    def storeMacKey(self):
        self.storeKey(self.macKey, self.directory + "/mac.key")

    def storeCounter(self):
        # Privsep bullshit...
        if (self.counterFile == None):
            self.counterFile = open(self.directory + '/counter', 'w')
            self.setPermissions(self.directory + '/counter')

        self.counterFile.seek(0)
        self.counterFile.write(str(self.counter) + "\n")
        self.counterFile.flush()

    def storeConfig(self):
        config = ConfigParser.SafeConfigParser()
        config.add_section('main')
        config.set('main', 'knock_port', str(self.knockPort))
        config.set('main', 'delay', str(10))
        config.set('main', 'source_ip', 'auto')
        config.set('main', 'auth_packet_delay', str(100))

        configFile = open(self.directory + "/config", 'w')
        config.write(configFile)
        configFile.close()

        self.setPermissions(self.directory + "/config")

    def storeKey(self, key, path):
        file = open(path, 'w')
        file.write(binascii.b2a_base64(key))
        file.close()

        self.setPermissions(path)

    # Permissions

    def setPermissions(self, path):
        os.chmod(path, stat.S_IRUSR | stat.S_IWUSR)

    # Debug

    def printHex(self, val):
        for c in val:
            print "%#x" % ord(c),
            
        print ""
