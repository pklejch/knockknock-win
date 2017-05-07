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

import os, hmac, hashlib,sys
import MacFailedException
from Crypto.Cipher import AES
from struct import *
import binascii

class CryptoEngine:

    def __init__(self, profile, cipherKey, macKey, counter):
        self.profile = profile
        self.counter = counter
        self.macKey = macKey
        self.cipherKey = cipherKey
        self.cipher = AES.new(self.cipherKey, AES.MODE_ECB)
        self.legacy = True

    def calculateMac(self, counter, message):
        counterBytes = pack('!I', counter)
        hmacSha = hmac.new(self.macKey, counterBytes + message, hashlib.sha256)
        mac = hmacSha.digest()
        return mac[:12]

    def calculateMac_legacy(self, port):
        hmacSha = hmac.new(self.macKey, port, hashlib.sha1)
        mac = hmacSha.digest()
        return mac[:10]

    def verifyMac(self, counter, port, remoteMac):
        localMac = self.calculateMac(counter, port)
        if localMac != remoteMac:
            raise MacFailedException.MacFailedException, "MAC Doesn't Match!"

    def verifyMac_legacy(self, port, remoteMac):
        localMac = self.calculateMac_legacy(port)
        if localMac != remoteMac:
            raise MacFailedException.MacFailedException, "MAC Doesn't Match!"

    def encryptCounter(self, counter):
        counterBytes = pack('!IIII', 0, 0, 0, counter)
        return self.cipher.encrypt(counterBytes)

    def encrypt(self, plaintextData):
        # plaintextData = port (2B) + IP (4B)

        # encrypt 16B
        counterCrypt = self.encryptCounter(self.counter)

        encrypted = str()

        # encrypt only port (2B)
        for i in range(len(plaintextData[:2])):
            encrypted += chr(ord(plaintextData[i]) ^ ord(counterCrypt[i]))

        # then authenticate encrypted port + plaintext IP
        # result is 2B port + 12B MAC
        encrypted += self.calculateMac(self.counter, encrypted + plaintextData[2:])

        self.counter += 1
        self.profile.setCounter(self.counter)
        self.profile.storeCounter()

        # output 14B
        return encrypted

    def encrypt_legacy(self, plaintextData):
        plaintextData += self.calculateMac_legacy(plaintextData)
        counterCrypt   = self.encryptCounter(self.counter)
        self.counter   = self.counter + 1
        encrypted      = str()

        for i in range((len(plaintextData))):
            encrypted += chr(ord(plaintextData[i]) ^ ord(counterCrypt[i]))

        self.profile.setCounter(self.counter)
        self.profile.storeCounter()

        return encrypted

    def decrypt_legacy(self, encryptedData, windowSize):
        for x in range(windowSize):
            try:
                counterCrypt = self.encryptCounter(self.counter + x)
                decrypted = str()

                for i in range((len(encryptedData))):
                    decrypted += chr(ord(encryptedData[i]) ^ ord(counterCrypt[i]))

                port = decrypted[:2]
                mac = decrypted[2:]

                self.verifyMac_legacy(port, mac)
                self.counter += x + 1

                self.profile.setCounter(self.counter)
                self.profile.storeCounter()

                return int(unpack("!H", port)[0])

            except MacFailedException.MacFailedException:
                pass

        raise MacFailedException.MacFailedException, "Ciphertext failed to decrypt in range..."


    def decrypt(self, encryptedData, windowSize):
        for x in range(windowSize):
            try:
                counterCrypt = self.encryptCounter(self.counter + x)

                decrypted_port = str()

                encrypted_port = encryptedData[:2]
                mac = encryptedData[2:14]
                source_ip = encryptedData[14:]

                self.verifyMac(self.counter + x, encrypted_port + source_ip, mac)

                for i in range((len(encrypted_port))):
                    decrypted_port += chr(ord(encrypted_port[i]) ^ ord(counterCrypt[i]))

                self.counter += x + 1
                self.profile.setCounter(self.counter)
                self.profile.storeCounter()

                # return PORT
                port = int(unpack("!H", decrypted_port)[0])
                return port

            except MacFailedException.MacFailedException:
                pass

        raise MacFailedException.MacFailedException, "Ciphertext failed to decrypt in range..."
