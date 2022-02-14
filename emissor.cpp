#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include "common.h"

// socket libraries:
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <pthread.h>

#define BUFSZ 500

std::vector<std::string> split(std::string str, char delimiter)
{
	std::vector<std::string> internal;
	int start = 0;

	for (long unsigned int i = 0; i < str.length(); i++)
	{
		if (str[i] == delimiter)
		{
			internal.push_back(str.substr(start, i - start));
			start = i + 1;
		}
		if (i == str.length() - 1)
		{
			internal.push_back(str.substr(start, i - start + 1));
		}
	}

	return internal;
}

int main(int argc, char **argv)
{
	char str[14];
	strcpy(str, argv[1]);
	char *ip = strtok(str, ":");								 // gets IP
	char *port = strtok(NULL, ":");							 // gets port
	std::string planetName = randomPlanetName(); // gets planet name

	std::string input;

	if (argv[2] < 0)
	{
		std::cout << " missing exhibitor's ID" << std::endl;
		exit(EXIT_FAILURE);
	}

	unsigned short exhibitorID = atoi(argv[2]);

	// socket parse:
	struct sockaddr_storage storage;

	if (0 != addrparse(ip, port, &storage))
	{
		printf("error\n");
		exit(EXIT_FAILURE);
	}

	int sock;
	std::cout << "creating socket" << std::endl;
	sock = socket(storage.ss_family, SOCK_STREAM, 0);
	if (sock == -1)
	{
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(sock, addr, sizeof(storage)))
	{
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	std::cout << "connected to " << addrstr << std::endl; // connection success

	// communication variables:
	size_t count;
	struct header issuerHeader;

	char buf[BUFSZ];
	unsigned short issuerID = 1;

	issuerHeader.msgOrder = 0;
	issuerHeader.msgOrigin = issuerID;
	issuerHeader.msgDestiny = exhibitorID;
	issuerHeader.exhibitorOffIssuer = exhibitorID;

	if (issuerHeader.msgOrder == 0)
	{
		// sends an "HI" message type (3)
		std::cout << "> hi" << std::endl;
		issuerHeader.msgType = 3;
		count = send(sock, &issuerHeader, sizeof(header), 0);

		if (count != sizeof(header))
		{
			logexit("send"); // in case of error
		}

		count = recv(sock, &issuerHeader, sizeof(header), 0);
		issuerID = issuerHeader.msgDestiny;

		std::cout << "< ok " << issuerID << std::endl;
	}

	if (issuerHeader.msgType == 1)
	{
		// inform to server the origin planet
		memset(buf, 0, BUFSZ);
		std::ostringstream msg;
		msg << "origin " << planetName.length() << " " << planetName.c_str() << std::endl;
		memcpy(buf, msg.str().c_str(), msg.str().length());
		std::cout << "> " << msg.str();

		unsigned short size_planet = strlen(buf);
		issuerHeader.msgType = 8;
		issuerHeader.msgDestiny = exhibitorID;
		issuerHeader.msgOrigin = issuerID;
		issuerHeader.msgOrder += 1;
		count = send(sock, &issuerHeader, sizeof(header), 0);
		count = send(sock, &size_planet, sizeof(size_planet), 0);
		count = send(sock, buf, size_planet, 0);
		count = recv(sock, &issuerHeader, sizeof(header), 0); // receives an "OK" message

		if (issuerHeader.msgType == 1)
		{
			std::cout << "< ok" << std::endl;
		}

		// connection accepted
		while (1)
		{
			std::cout << "> ";
			getline(std::cin, input, '\n');
			std::vector<std::string> args = split(input, ' ');
			issuerHeader = mountHeader(input, issuerID, issuerHeader.msgOrder);

			if (issuerHeader.msgType == 4)
			{
				issuerHeader.msgDestiny = exhibitorID;
				count = send(sock, &issuerHeader, sizeof(header), 0);
				close(sock);
				std::cout << "\n connection terminated" << std::endl;
				exit(EXIT_SUCCESS);
			}
			else
			{
				if (issuerHeader.msgType != 9)
					issuerHeader.msgDestiny = atoi(args[1].c_str());

				switch (issuerHeader.msgType)
				{
				case 5: // msg idPlanet numCharsMsg message
				{
					int startOfMessage = args[0].length() + args[1].length() + args[2].length() + 3;
					std::string message = input.substr(startOfMessage);
					count = send(sock, &issuerHeader, sizeof(header), 0);
					unsigned short size = atoi(args[2].c_str());
					count = send(sock, &size, sizeof(size), 0);		// sends message's size first
					count = send(sock, message.c_str(), size, 0); // sends message

					count = recv(sock, &issuerHeader, sizeof(header), 0); // receives an "OK" message

					if (issuerHeader.msgType == 1)
					{
						std::cout << "< ok" << std::endl;
					}
				}
				break;
				case 6: // creq idPlanet
					count = send(sock, &issuerHeader, sizeof(header), 0);

					recv(sock, &issuerHeader, sizeof(header), 0); // receives an "OK" message
					if (issuerHeader.msgType == 1)
					{
						std::cout << "< ok" << std::endl;
					}
					break;
				case 9: // planet idToFind
					issuerHeader.msgDestiny = issuerHeader.exhibitorOffIssuer;
					issuerHeader.ClientToFindPlanetName = atoi(args[1].c_str());  // msgOrigin = client to find the planet name
					count = send(sock, &issuerHeader, sizeof(header), 0);
					recv(sock, &issuerHeader, sizeof(header), 0); // receives an "OK" message
					if (issuerHeader.msgType == 1)
					{
						std::cout << "< ok" << std::endl;
					}
					break;
				case 10: // planet Exhibitor
					count = send(sock, &issuerHeader, sizeof(header), 0);
					recv(sock, &issuerHeader, sizeof(header), 0); // receives an "OK" message
					if (issuerHeader.msgType == 1)
					{
						std::cout << "< ok" << std::endl;
					}
					break;
				default:
					std::cout << "invalid option" << std::endl;
					break;
				}
			}
		}
	}
	else if (issuerHeader.msgType == 2)
	{
		std::cout << "\n communication failed" << std::endl;
		close(sock);
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "\n unknown message type" << std::endl;
		close(sock);
		exit(EXIT_FAILURE);
	}
}
