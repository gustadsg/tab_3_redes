#include "common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <algorithm>

#include <arpa/inet.h>


void logexit(const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage) {
    if (addrstr == NULL || portstr == NULL) {
    }
    uint16_t port = (uint16_t)atoi(portstr); 
    if (port == 0) {
        return -1;
    }
    port = htons(port); 

    struct in_addr inaddr4; 
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; 
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET) {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        port = ntohs(addr4->sin_port); 
    }
    else if (addr->sa_family == AF_INET6)  {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        port = ntohs(addr6->sin6_port); 
    }
    else  {
        logexit("unknown protocol family.");
    }
    if (str)  {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t)atoi(portstr); 
    if (port == 0) {
        return -1;
    }
    port = htons(port); 

    memset(storage, 0, sizeof(*storage));
    if (0 == strcmp(proto, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    }
    else if (0 == strcmp(proto, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    }
    else {
        return -1;
    }
}


int testa_exibidores(vector<client> &clients, unsigned short id) {
    int aux = -1;
    for (long unsigned int s; s < clients.size(); s++) {
        if (clients[s].id == id) {
            aux++;
        }
    }
    return aux;
}

unsigned short returnsID(const vector<client> client, char c) {
    if (client.size() == 1) {
        switch (c) {
        case 'e':
            return 4096; 
            break;
        case 'i':
            return 1; 
            break;
        default:
            return -1;
            break;
        }
    }
    else {
        long unsigned int s = 1;
        int aux = 0;
        for (s = 1; s < client.size(); s++) {
            if (client[s].id - client[s - 1].id > 1) {
                aux++;
                break;
            }
        }

        if (aux == 0) {
            return (client.back().id + 1);
        }
        else {
            return (client[s - 1].id + 1);
        }
    }
}

struct header mountHeader(std::string input, int issuerID, int lastMessageOrder) {
	struct header header;

	transform(input.begin(), input.end(), input.begin(), ::tolower);

	header.msg_origem = issuerID;

	if (!(input.rfind("planet", 0) == 0 || input.rfind("planetlist", 0) == 0))
		header.msg_contagem = lastMessageOrder++;

	// hi
	if (input.rfind("hi", 0) == 0) {
		header.msg_tipo = 3;
		header.msg_contagem = 1;

		return header;
	}

	// kill
	if (input.rfind("kill", 0) == 0) {
		header.msg_tipo = 4;

		return header;
	}

	// msg
	if (input.rfind("msg", 0) == 0) {
		header.msg_tipo = 5;

		return header;
	}

	// creq
	if (input.rfind("creq", 0) == 0) {
		header.msg_tipo = 6;

		return header;
	}

	// origin
	if (input.rfind("origin", 0) == 0) {
		header.msg_tipo = 8;

		return header;
	}

    // planetlist
	if (input.rfind("planetlist", 0) == 0) {
		header.msg_tipo = 10;
		header.msg_contagem = lastMessageOrder;

		return header;
	}

	// planet
	if (input.rfind("planet", 0) == 0){
		header.msg_tipo = 9;
		header.msg_contagem = lastMessageOrder;

		return header;
	}

    return header;
}

