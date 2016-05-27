# unixChat

5/26/2016: Updated to remove warnings for client program.

To run the program, open a shell for the server, and 1 shell per desired client.

To compile the server, type: gcc chatserverC.c -o chatserverC -pthread

and to run, type: ./chatserverC

To compile the client (which needs a running server), type: gcc chatclientF.c -o chatclientF -pthread

and to run, type: ./chatclientF \<server's local IP address\>

If you start more than one client, you can chat between terminals on your local network.
