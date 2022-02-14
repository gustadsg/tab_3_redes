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

int main(int argc, char **argv)
{

	char string[14];
	strcpy(string, argv[1]);
	char *ip;
	char *porta;
	std::string planetName = "Netuno"; // gets planet name

	ip = strtok(string, ":");
	porta = strtok(NULL, ":");

	char buf[BUFSZ];

	struct sockaddr_storage storage;

	if (0 != addrparse(ip, porta, &storage))
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

	struct header exibidor_header;

	unsigned short exibidor_ID = 0;

	exibidor_header.msg_contagem = 0;
	exibidor_header.msg_destino = 65535; // server's ID
	exibidor_header.msg_origem = exibidor_ID;

	if (exibidor_header.msg_contagem == 0)
	{
		// sends an "HI" message type (3)
		std::cout << "> hi" << std::endl;
		exibidor_header.msg_tipo = 3; 
		send(s, &exibidor_header, sizeof(header), 0);
		// receiving ok for hi message
		recv(s, &exibidor_header, sizeof(header), 0);

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
		send(s, &exibidor_header, sizeof(header), 0);
		send(s, &size_planet, sizeof(size_planet), 0);
		send(s, buf, size_planet, 0);
		recv(s, &exibidor_header, sizeof(header), 0); // receives an "OK" message

		if (exibidor_header.msg_tipo == 1)
		{
			std::cout << "< ok" << std::endl;
		}

		while (1) {
			recv(s, &exibidor_header, sizeof(header), 0);

			if (exibidor_header.msg_destino != exibidor_ID) {
				exibidor_header.msg_tipo = 2;
				exibidor_header.msg_destino = exibidor_header.msg_origem;
				exibidor_header.msg_origem = exibidor_ID;
			}
			else {
				switch (exibidor_header.msg_tipo) {
					case 4:
						// exibidor_header.msg_tipo = 1; // "OK" message
						// exibidor_header.msg_destino = exibidor_header.msg_origem;
						// exibidor_header.msg_origem = exibidor_ID;
						//send(s, &exibidor_header, sizeof(header), 0);

						close(s);
						
						std::cout << "< ok" << std::endl;
						exit(EXIT_SUCCESS);
						break;
					case 5:
						unsigned short size;
						memset(buf, 0, BUFSZ);

						recv(s, &size, sizeof(size), 0);
						recv(s, buf, size, 0);

						std::cout << "< message from " << exibidor_header.msg_origem << ": " << buf << std::endl;

						exibidor_header.msg_tipo = 1; // "OK" message
						exibidor_header.msg_destino = exibidor_header.msg_origem;
						exibidor_header.msg_origem = exibidor_ID;

						send(s, &exibidor_header, sizeof(header), 0);

						break;
					case 7:	{
						unsigned short N = 0;

						recv(s, &N, sizeof(N), 0);
						std::cout << "clist: " << N << " ";
						unsigned short clist[N];
						recv(s, clist, N, 0);

						for (int i = 0; i < N; i++) {
							std::cout << clist[i] << " ";
						}
						std::cout << endl;
						exibidor_header.msg_origem = 1;
						exibidor_header.msg_destino = exibidor_header.msg_origem;
						exibidor_header.msg_origem = exibidor_ID;

						send(s, &exibidor_header, sizeof(exibidor_header), 0);
						break;
					}
					case 10: {
						memset(buf, 0, BUFSZ);

						recv(s, &size, sizeof(size), 0);
						recv(s, buf, size, 0);

						std::cout << buf << std::endl;

						exibidor_header.msg_tipo = 1; // "OK" message
						exibidor_header.msg_destino = exibidor_header.msg_origem;
						exibidor_header.msg_origem = exibidor_ID;

						send(s, &exibidor_header, sizeof(header), 0);

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
