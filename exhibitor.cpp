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
#include <algorithm>
#define BUFSZ 500

std::string GerarPlanetaExibidor(){
	vector<std::string> planets;
    planets.push_back("Mercury");
    planets.push_back("Venus");
    planets.push_back("Earth");
    planets.push_back("Mars");
    planets.push_back("Jupiter");
    planets.push_back("Saturn");
    planets.push_back("Urain");
    planets.push_back("Neptune");

    srand(time(NULL));
    int random = rand() % planets.size();
    return planets[random];
}

int main(int argc, char **argv)
{

	char string[14];
	strcpy(string, argv[1]);
	char *ip;
	char *porta;
	
	std::string planetName = GerarPlanetaExibidor(); // gets planet name

	ip = strtok(string, ":");
	porta = strtok(NULL, ":");

	char buf[BUFSZ];

	struct sockaddr_storage storage;

	if (0 != addrparse(ip, porta, &storage))
	{
		printf("error\n");
		exit(EXIT_FAILURE);
	}

	int socket_exibidor;
	socket_exibidor = socket(storage.ss_family, SOCK_STREAM, 0);
	if (socket_exibidor == -1)
	{
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(socket_exibidor, addr, sizeof(storage)))
	{
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("connected to %socket_exibidor\n", addrstr);

	struct header exibidor_header;

	unsigned short exibidor_ID = 0;

	exibidor_header.msg_contagem = 0;
	exibidor_header.msg_destino = 65535; // server'socket_exibidor ID
	exibidor_header.msg_origem = exibidor_ID;

	if (exibidor_header.msg_contagem == 0)
	{
		// sends an "HI" message type (3)
		std::cout << "> hi" << std::endl;
		exibidor_header.msg_tipo = 3; 
		send(socket_exibidor, &exibidor_header, sizeof(header), 0);
		// receiving ok for hi message
		recv(socket_exibidor, &exibidor_header, sizeof(header), 0);

		exibidor_ID = exibidor_header.msg_destino;

		std::cout << "< ok " << exibidor_ID << std::endl;
	}

	if (exibidor_header.msg_tipo == 1)
	{

		// inform to server the origin planet
		memset(buf, 0, BUFSZ);
		std::ostringstream msg;



		msg << "origin " << planetName.length() << " " << planetName.c_str() << std::endl;
		memcpy(buf, msg.str().c_str(), msg.str().length());
		std::cout << "> " << msg.str();

		unsigned short size_planet = strlen(buf);
		exibidor_header.msg_tipo = 8;
		exibidor_header.msg_destino = 65535;
		exibidor_header.msg_origem = exibidor_ID;
		exibidor_header.msg_contagem += 1;
		send(socket_exibidor, &exibidor_header, sizeof(header), 0);
		send(socket_exibidor, &size_planet, sizeof(size_planet), 0);
		send(socket_exibidor, buf, size_planet, 0);
		recv(socket_exibidor, &exibidor_header, sizeof(header), 0); // receives an "OK" message

		if (exibidor_header.msg_tipo == 1)
		{
			std::cout << "< ok" << std::endl;
		}

		while (1) {
			recv(socket_exibidor, &exibidor_header, sizeof(header), 0);

			if (exibidor_header.msg_destino != exibidor_ID) {
				exibidor_header.msg_tipo = 2;
				exibidor_header.msg_destino = exibidor_header.msg_origem;
				exibidor_header.msg_origem = exibidor_ID;
			}
			else {
				switch (exibidor_header.msg_tipo) {
					case 1:

					recv(socket_exibidor, &exibidor_header, sizeof(header), 0);	// receiving ok for hi message

					case 4:
						close(socket_exibidor);
						
						std::cout << "< ok" << std::endl;
						exit(EXIT_SUCCESS);
						break;
					case 5:{
						unsigned short size = 0;
						memset(buf, 0, BUFSZ);

						recv(socket_exibidor, &size, sizeof(size), 0);
						std::cout << size << endl;
						recv(socket_exibidor, buf, size, 0);
						std::cout << buf << endl;

						std::cout << "< message from " << exibidor_header.msg_origem << ": " << buf << std::endl;

						exibidor_header.msg_tipo = 1; // "OK" message
						exibidor_header.msg_destino = exibidor_header.msg_origem;
						exibidor_header.msg_origem = exibidor_ID;

						send(socket_exibidor, &exibidor_header, sizeof(header), 0);

						break;}
					case 7:	{
						unsigned short N = 0;

						recv(socket_exibidor, &N, sizeof(N), 0);
						std::cout << "clist: " << N << " ";
						unsigned short clist[N];
						recv(socket_exibidor, clist, N, 0);

						for (int i = 0; i < N; i++) {
							std::cout << clist[i] << " ";
						}
						std::cout << endl;
						exibidor_header.msg_origem = 1;
						exibidor_header.msg_destino = exibidor_header.msg_origem;
						exibidor_header.msg_origem = exibidor_ID;

						send(socket_exibidor, &exibidor_header, sizeof(exibidor_header), 0);
						break;
					}
					case 9: {
						unsigned short size = 0;
						memset(buf, 0, BUFSZ);
						recv(socket_exibidor, &size, sizeof(size), 0);
						recv(socket_exibidor, buf, size, 0);
						std::cout << "planet of "<< exibidor_header.ID_Cliente_Planeta_Pedido << ": " << buf << endl;
						send(socket_exibidor, &exibidor_header, sizeof(header), 0);
						break;
					}
					case 10: {
						unsigned short size = 0;
						memset(buf, 0, BUFSZ);

						recv(socket_exibidor, &size, sizeof(size), 0);
						recv(socket_exibidor, buf, size, 0);

						std::cout << buf << std::endl;

						exibidor_header.msg_tipo = 1; // "OK" message
						exibidor_header.msg_destino = exibidor_header.msg_origem;
						exibidor_header.msg_origem = exibidor_ID;

						send(socket_exibidor, &exibidor_header, sizeof(header), 0);

						break;
					}
					default: {
						printf("\nERROR\n");
						exit(EXIT_FAILURE);
						break;
					}
				}
			}
		}
	}
	else if (exibidor_header.msg_tipo == 2)
	{
		std::cout << "\ncommunication failed" << std::endl;
		close(socket_exibidor);
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "\nunknown message type" << std::endl;
		close(socket_exibidor);
		exit(EXIT_FAILURE);
	}
}
