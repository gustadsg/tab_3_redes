#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include "common.h"
#include "istream"

// socket libraries:
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <pthread.h>

#define BUFSZ 500

std::vector<std::string> split(std::string str, char delimitador)
{
	std::vector<std::string> internal;
	int start = 0;

	for (long unsigned int i = 0; i < str.length(); i++)
	{
		if (str[i] == delimitador)
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
	//std::string Nome_Planeta = Gera_Nome_Planeta(); // gets planet name

	std::string input;

	if (argv[2] < 0)
	{
		std::cout << " missing exhibitor's ID" << std::endl;
		exit(EXIT_FAILURE);
	}

	unsigned short exibidor_ID = atoi(argv[2]);

	// socket parse:
	struct sockaddr_storage storage;

	if (0 != addrparse(ip, port, &storage))
	{
		printf("error\n");
		exit(EXIT_FAILURE);
	}

	int socket_emissor;
	std::cout << "criando o socket" << std::endl;
	socket_emissor = socket(storage.ss_family, SOCK_STREAM, 0);
	if (socket_emissor == -1)
	{
		logexit("socket_emissor");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(socket_emissor, addr, sizeof(storage)))
	{
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	std::cout << "connected to " << addrstr << std::endl; // connection success

	// communication variables:
	size_t count;
	struct header emissor_Header;
	unsigned short emissor_ID = 1;

	emissor_Header.msg_contagem = 0;
	emissor_Header.msg_origem = emissor_ID;
	emissor_Header.msg_destino = exibidor_ID;
	emissor_Header.exibidor_do_emissor = exibidor_ID;

	// sends an "HI" message type (3)
	std::string entrada; 
	std::cin >> entrada;
	if(entrada != "hi"){
		emissor_Header.msg_tipo == 2;
	}
	else{
		emissor_Header.msg_tipo = 3;
		count = send(socket_emissor, &emissor_Header, sizeof(header), 0);	
		if (count != sizeof(header)) {
			logexit("send"); // in case of error
		}
		
		count = recv(socket_emissor, &emissor_Header, sizeof(header), 0);	// receiving ok for hi message
		std::cout << count << " " <<emissor_ID << endl;
		emissor_Header.msg_tipo = 1;
	}

	if (emissor_Header.msg_tipo == 1)	{
		// inform to server the origin planet
		std::cin.ignore();
		getline(std::cin, input, '\n');
		std::vector<std::string> args = split(input, ' ');
		if(args[0] != "origin"){
			emissor_Header.msg_tipo == 2;
			count = send(socket_emissor, &emissor_Header, sizeof(header), 0);

		}
		else{
			emissor_Header.msg_tipo = 8;
			emissor_Header.msg_destino = exibidor_ID;
			emissor_Header.msg_origem = emissor_ID;
			emissor_Header.msg_contagem += 1;
			unsigned short size = atoi(args[1].c_str());
			count = send(socket_emissor, &emissor_Header, sizeof(header), 0);

			count = send(socket_emissor, &size, sizeof(size), 0);		// sends message's size first
			count = send(socket_emissor, args[2].c_str(), size, 0); // sends message

			count = recv(socket_emissor, &emissor_Header, sizeof(header), 0); // receives an "OK" message
			std::cout << count << endl;
			if (count != sizeof(header)) {
				logexit("send"); // in case of error
			}
			
			recv(socket_emissor, &emissor_Header, sizeof(header), 0);	// receiving ok for hi message
		}

		if (emissor_Header.msg_tipo == 1)	{
			std::cout << "< ok" << std::endl;
		}

		// connection accepted
		while (1)	{
			std::cout << "> ";
			getline(std::cin, input, '\n');
			std::vector<std::string> args = split(input, ' ');
			emissor_Header = mountHeader(input, emissor_ID, emissor_Header.msg_contagem);

			if (emissor_Header.msg_tipo == 4)
			{
				emissor_Header.msg_destino = exibidor_ID;
				count = send(socket_emissor, &emissor_Header, sizeof(header), 0);
				close(socket_emissor);
				std::cout << "\n connection terminated" << std::endl;
				exit(EXIT_SUCCESS);
			}
			else
			{
				if (emissor_Header.msg_tipo != 9)
					emissor_Header.msg_destino = atoi(args[1].c_str());

				switch (emissor_Header.msg_tipo)
				{

					case 5: // msg idPlanet numCharsMsg message
					{
						int startOfMessage = args[0].length() + args[1].length() + args[2].length() + 3;
						std::string message = input.substr(startOfMessage);
						count = send(socket_emissor, &emissor_Header, sizeof(header), 0);
						unsigned short size = atoi(args[2].c_str());
						count = send(socket_emissor, &size, sizeof(size), 0);		// sends message's size first
						count = send(socket_emissor, message.c_str(), size, 0); // sends message

						count = recv(socket_emissor, &emissor_Header, sizeof(header), 0); // receives an "OK" message

						if (emissor_Header.msg_tipo == 1)
						{
							std::cout << "< ok" << std::endl;
						}
					}
					break;
					case 6: // creq idPlanet
						count = send(socket_emissor, &emissor_Header, sizeof(header), 0);

						recv(socket_emissor, &emissor_Header, sizeof(header), 0); // receives an "OK" message
						if (emissor_Header.msg_tipo == 1)
						{
							std::cout << "< ok" << std::endl;
						}
						break;
					case 9: // planet idToFind
						emissor_Header.msg_destino = emissor_Header.exibidor_do_emissor;
						emissor_Header.ID_Cliente_Planeta_Pedido = atoi(args[1].c_str());  // msg_origem = client to find the planet name
						count = send(socket_emissor, &emissor_Header, sizeof(header), 0);
						recv(socket_emissor, &emissor_Header, sizeof(header), 0); // receives an "OK" message
						if (emissor_Header.msg_tipo == 1)
						{
							std::cout << "< ok" << std::endl;
						}
						break;
					case 10: // planet Exhibitor
						count = send(socket_emissor, &emissor_Header, sizeof(header), 0);
						recv(socket_emissor, &emissor_Header, sizeof(header), 0); // receives an "OK" message
						if (emissor_Header.msg_tipo == 1)
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
	else if (emissor_Header.msg_tipo == 2)	{
		std::cout << "\n communication failed" << std::endl;
		close(socket_emissor);
		exit(EXIT_FAILURE);
	}
	else {
		std::cout << "\n unknown message type" << std::endl;
		close(socket_emissor);
		exit(EXIT_FAILURE);
	}
}
