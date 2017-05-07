# knockknock-win
Knockknock deamon implementation for MS Windows operating system. Created by Petr Klejch.

## About 
This tool is build on existing implementation of port knocking -- knockknock by Moxie Marlinspike. It implements single packet authorization concept. It uses one TCP SYN packet with encrypted message inside TCP and IP header.

Client side supports automatic sending of authentication packets. You don't have to send anything manually !

Client side support any application protocol using TCP or UDP.
Client side supports only IPv4 for now.
## Installation 
In bin folder there are installation packages with compiled binaries for client and server.
You need to have installed [Microsoft Visual C++ Redistributable for Visual Studio 2012 for x86](https://www.microsoft.com/en-us/download/details.aspx?id=30679) on both machines.

## Usage
If you install this tool as Windows Service, it will be controlled from Service Manager (Start -> Services) or Win+R and type "services.msc".
You can start, stop service, set automatic start after boot or set automatic restart in case of crash.

Before using this tool, you have to generate profiles with knockknock-genprofile binary, which is in server side package. Use it from command line like this:

    knockknock-genprofile <profile_name> <port>

Port is used to identify different users on server. Profile will be generated in folder conf/profiles/profile_name.
Then you have to securely transport generated profile to client computer and store it in conf/domain_name_or_IP_of_a_server.

When you start client side of service, it will automatically send authentication packets to defined server.

## Compatibility
Windows 7 and higher.

## Powered by

 - [WinDivert](https://github.com/basil00/Divert/) -- library for easy manipulation with packets.
 - [Knockknock](https://github.com/moxie0/knockknock) -- source implementation for knockknock server and authentication protocol.
 - [PyInstaller](http://www.pyinstaller.org/)  -- tool for converting Python projects into standalone executable binaries.
 - [InnoSetup](http://www.jrsoftware.org/isinfo.php) -- tool for creating installation packages.


