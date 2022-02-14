#pragma once

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <algorithm>

#include <arpa/inet.h>

using namespace std;

struct header{
    // 8 bytes header struct declaration
    unsigned short msg_tipo;
    unsigned short msg_origem;
    unsigned short msg_destino;
    unsigned short msg_contagem;
    unsigned short exibidor_do_emissor;
    unsigned short ID_Cliente_Planeta_Pedido;
};

struct client{
    int socket;
    unsigned short id;
    std::string planet;
};

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);

unsigned short getsType();

unsigned short getsDestiny(unsigned short destiny);

int checksExib(vector<client> &clients, unsigned short id);

unsigned short returnsID(const vector<client> client, char c);

struct header mountHeader(std::string input, int issuerID, int lastMessageOrder);

void SavePlanet(std::string planetName);