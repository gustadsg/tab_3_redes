#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include "common.h"

// socket libraries:
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <pthread.h>

#define BUFSZ 500

using namespace std;

string randomPlanetName();

int main(int argc, char **argv)
{
	char string[14];
	strcpy(string, argv[1]);
	char *ip = strtok(string, ":"); // gets IP
	char *port = strtok(NULL, ":"); // gets port

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

	printf("connected to %s\n", addrstr); // connection success

	// communication variables:
	size_t count;
	struct header issuerHeader;

	char buf[BUFSZ];
	unsigned short issuerID = 1;

	issuerHeader.msgOrder = 0;
	issuerHeader.msgOrigin = issuerID;
	issuerHeader.msgDestiny = exhibitorID;
	std::string planetName = randomPlanetName();
	if (issuerHeader.msgOrder == 0)
	{
		// sends an "OI" message type (3)
		issuerHeader.msgType = 3;
		count = send(sock, &issuerHeader, sizeof(header), 0);

		if (count != sizeof(header))
		{
			logexit("send"); // in case of error
		}

		count = recv(sock, &issuerHeader, sizeof(header), 0);

		issuerID = issuerHeader.msgDestiny; // this issuer ID
	}

	if (issuerHeader.msgType == 1)
	{
		std::cout << "\nissuer connected - ID:" << issuerID << std::endl;
		// inform to server the origin planet
		memset(buf, 0, BUFSZ);
		sprintf(buf, "origin %d %s", planetName.length(), planetName.c_str());

		unsigned short size_planet = strlen(buf);
		issuerHeader.msgType = 8;
		issuerHeader.msgDestiny = exhibitorID;
		issuerHeader.msgOrigin = issuerID;
		count = send(sock, &issuerHeader, sizeof(header), 0); 
		count = send(sock, &size_planet, sizeof(size_planet), 0);
		count = send(sock, buf, size_planet, 0);

		count = recv(sock, &issuerHeader, sizeof(header), 0); // receives an "OK" message

                    if (issuerHeader.msgType == 1)
                    {
                        std::cout << "\n message delivered!" << std::endl;
                    }


		// connection accepted
		while (1)
		{

			issuerHeader.msgOrder++;
			issuerHeader.msgOrigin = issuerID;
			issuerHeader.msgType = getsType();

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

				issuerHeader.msgDestiny = getsDestiny(exhibitorID);

				switch (issuerHeader.msgType)
				{
				case 5:
				{
					count = send(sock, &issuerHeader, sizeof(header), 0);

					std::cout << "\n> send message to " << issuerHeader.msgDestiny << ": " << std::endl;
					memset(buf, 0, BUFSZ);
					std::cin.ignore();
					std::cin.getline(buf, BUFSZ);

					unsigned short size = strlen(buf);
					count = send(sock, &size, sizeof(size), 0); // sends message's size first
					count = send(sock, buf, size, 0);						// sends message

					count = recv(sock, &issuerHeader, sizeof(header), 0); // receives an "OK" message

					if (issuerHeader.msgType == 1)
					{
						std::cout << "\n message delivered!" << std::endl;
					}

					issuerHeader.msgOrder++;
				}
				break;
				case 6:
					count = send(sock, &issuerHeader, sizeof(header), 0);

					recv(sock, &issuerHeader, sizeof(header), 0); // receives an "OK" message
					if (issuerHeader.msgType == 1)
					{
						std::cout << "\n message delivered!" << std::endl;
					}

					issuerHeader.msgOrder++;
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

string randomPlanetName()
{
	vector<string> planets;
	planets.push_back("Mercury");
	planets.push_back("Venus");
	planets.push_back("Earth");
	planets.push_back("Mars");
	planets.push_back("Jupiter");
	planets.push_back("Saturn");
	planets.push_back("Urain");
	planets.push_back("Neptune");

	int random = rand() % planets.size();
	return planets[random];
}