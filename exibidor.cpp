#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include "common.h"

// socket libraries:
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <pthread.h>

#define BUFSZ 500

int main(int argc, char **argv)
{

	char string[14];
	strcpy(string, argv[1]);
	char *ip;
	char *port;
	std::string planetName = randomPlanetName(); // gets planet name
	std::cout << "Planet name: " << planetName << std::endl;

	ip = strtok(string, ":");
	port = strtok(NULL, ":");

	char buf[BUFSZ];

	struct sockaddr_storage storage;

	if (0 != addrparse(ip, port, &storage))
	{
		printf("error\n");
		exit(EXIT_FAILURE);
	}

	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1)
	{
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(storage)))
	{
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("connected to %s\n", addrstr);

	struct header exhibitorHeader;

	unsigned short exhibitorID = 0;

	exhibitorHeader.msgOrder = 0;
	exhibitorHeader.msgDestiny = 65535; // server's ID
	exhibitorHeader.msgOrigin = exhibitorID;

	if (exhibitorHeader.msgOrder == 0)
	{
		std::cout << "sending hi message" << std::endl;
		exhibitorHeader.msgType = 3; // sends an "HI" message (3)
		send(s, &exhibitorHeader, sizeof(header), 0);
		std::cout << "receiving ok for hi message" << std::endl;
		recv(s, &exhibitorHeader, sizeof(header), 0);

		exhibitorID = exhibitorHeader.msgDestiny;

		std::cout << "\nexhibitor connected - ID: " << exhibitorID << std::endl;
	}

	if (exhibitorHeader.msgType == 1)
	{

		// inform to server the origin planet
		std::cout << "informing planet to server" << std::endl;
		memset(buf, 0, BUFSZ);
		std::ostringstream msg;
		msg << "origin " << planetName.length() << " " << planetName.c_str() << std::endl;
		memcpy(buf, msg.str().c_str(), msg.str().length());

		unsigned short size_planet = strlen(buf);
		exhibitorHeader.msgType = 8;
		exhibitorHeader.msgDestiny = 65535;
		exhibitorHeader.msgOrigin = exhibitorID;
		send(s, &exhibitorHeader, sizeof(header), 0);
		send(s, &size_planet, sizeof(size_planet), 0);
		send(s, buf, size_planet, 0);
		recv(s, &exhibitorHeader, sizeof(header), 0); // receives an "OK" message

		if (exhibitorHeader.msgType == 1)
		{
			std::cout << "origin message delivered!" << std::endl;
		}

		while (1)
		{

			recv(s, &exhibitorHeader, sizeof(header), 0);

			if (exhibitorHeader.msgDestiny != exhibitorID)
			{
				exhibitorHeader.msgType = 2;
				exhibitorHeader.msgDestiny = exhibitorHeader.msgOrigin;
				exhibitorHeader.msgOrigin = exhibitorID;
			}
			else
			{

				switch (exhibitorHeader.msgType)
				{
				case 4:
					close(s);
					std::cout << "\n connection terminated" << std::endl;
					exit(EXIT_SUCCESS);
					break;
				case 5:
					unsigned short size;
					memset(buf, 0, BUFSZ);

					recv(s, &size, sizeof(size), 0);
					recv(s, buf, size, 0);

					std::cout << "\n< message from " << exhibitorHeader.msgOrigin << ": " << buf << std::endl;

					exhibitorHeader.msgType = 1; // "OK" message
					exhibitorHeader.msgDestiny = exhibitorHeader.msgOrigin;
					exhibitorHeader.msgOrigin = exhibitorID;

					send(s, &exhibitorHeader, sizeof(header), 0);

					break;
				case 7:
				{
					unsigned short N = 0;

					recv(s, &N, sizeof(N), 0);
					std::cout << "CLIST: " << N << " clients connected:" << std::endl;
					unsigned short clist[N];
					recv(s, clist, N, 0);

					for (int i = 0; i < N; i++)
					{
						std::cout << clist[i] << std::endl;
					}

					exhibitorHeader.msgOrigin = 1;
					exhibitorHeader.msgDestiny = exhibitorHeader.msgOrigin;
					exhibitorHeader.msgOrigin = exhibitorID;

					send(s, &exhibitorHeader, sizeof(exhibitorHeader), 0);
					break;
				}

				default:
					printf("\nERROR\n");
					exit(EXIT_FAILURE);
					break;
				}
			}
		}
	}
	else if (exhibitorHeader.msgType == 2)
	{
		std::cout << "\ncommunication failed" << std::endl;
		close(s);
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "\nunknown message type" << std::endl;
		close(s);
		exit(EXIT_FAILURE);
	}
}
