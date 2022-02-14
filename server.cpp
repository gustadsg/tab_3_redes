#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

#include "common.h"

#define BUFSZ 500

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char **argv)
{

    const char *PORT = argv[1]; // gets  PORT

    fd_set master;   // master file descriptor list
    fd_set read_fds; // temp file descriptor list for select()
    int fdmax;       // maximum file descriptor number

    int listener;                       // listening socket descriptor
    int newfd;                          // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[BUFSZ];
    unsigned short size;

    int nbytes;

    std::vector<client> exibidores; // exhibtiors vector
    std::vector<client> emissores;    // emissores vector

    char remoteIP[INET6_ADDRSTRLEN];

    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
    {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    printf("server ready for connections\n");
    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);

            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL)
    {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1)
    {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for (;;)  {
        read_fds = master; // copy it
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)  {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for (i = 0; i <= fdmax; i++)  {
            struct header server_header;
            if (FD_ISSET(i, &read_fds))  { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                                   (struct sockaddr *)&remoteaddr,
                                   &addrlen);

                    if (newfd == -1)  {
                        perror("accept");
                    }
                    else  {
                        FD_SET(newfd, &master);               // add to master set
                        if (newfd > fdmax)  {               // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                               "socket %d\n",
                               inet_ntop(remoteaddr.ss_family,
                                         get_in_addr((struct sockaddr *)&remoteaddr),
                                         remoteIP, INET6_ADDRSTRLEN),
                               newfd);
                    }
                }
                else  {
                    // handle data from a client
                    if ((nbytes = recv(i, &server_header, sizeof(header), 0)) <= 0)  {
                        // got error or connection closed by client
                        if (nbytes == 0)
                        {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master); // remove from master set
                    }
                    else
                    {
                        // we got some data from a client
                        if (server_header.msg_tipo != (9 && 8 && 3 && 5)){
                            std::cout << "received from " << server_header.msg_origem << ": ";
                            std::cout << server_header.msg_tipo << " " << server_header.msg_origem << " " << server_header.msg_destino << " " << server_header.msg_contagem << std::endl;
                        }
                        switch (server_header.msg_tipo) {

                            case 1:    {                                               //"OK" message, from exibidor to emissor
                                for (long unsigned int s = 0; s < emissores.size(); s++) {
                                    if (emissores[s].id == server_header.msg_destino)  {
                                        send(emissores[s].socket, &server_header, sizeof(header), 0);
                                        break;
                                    }
                                }
                                break;
                            }
                            case 3:
                                std::cout << "received hi: ";
                                std::cout << server_header.msg_tipo << " " << server_header.msg_origem << " " << 
                                    server_header.msg_destino << " " << server_header.msg_contagem << std::endl;

                                if (server_header.msg_origem == 0)   {
                                    // recognizes client: exibidor
                                    struct client exibidor;
                                    exibidor.socket = i;
                                    exibidores.push_back(exibidor);
                                    exibidor.id = returnsID(exibidores, 'e');
                                    exibidores.back().id = exibidor.id;

                                    server_header.msg_tipo = 1;
                                    server_header.msg_destino = exibidor.id;
                                    server_header.msg_origem = 65535;

                                    send(i, &server_header, sizeof(header), 0);
                                }
                                else if (server_header.msg_origem > 0)  {
                                    // recognize client: emissor
                                    struct client emissor;
                                    emissor.socket = i;
                                    emissores.push_back(emissor);
                                    emissor.id = returnsID(emissores, 'i');

                                    // checks given exibidores id:
                                    int aux01 = -1;

                                    for (long unsigned int t = 0; t < exibidores.size(); t++)   {
                                        if (exibidores[t].id == server_header.msg_destino)   {
                                            aux01++;

                                            break;
                                        }
                                    }

                                    if (aux01 >= 0)  {
                                        // reconhece o exibidor_ID associado
                                        server_header.msg_tipo = 1;
                                        server_header.msg_destino = emissor.id;
                                        server_header.msg_origem = 65535;
                                    }
                                    else  {
                                        // wrong exibidor's id was sent
                                        server_header.msg_tipo = 2;
                                        server_header.msg_destino = server_header.msg_origem;
                                        server_header.msg_origem = 65535;
                                    }
                                    send(i, &server_header, sizeof(header), 0);
                                }
                                else  {
                                    // não conseguiu reconhecer o client
                                    server_header.msg_tipo = 2;
                                    server_header.msg_destino = server_header.msg_origem;
                                    server_header.msg_origem = 65535;
                                    send(i, &server_header, sizeof(header), 0);
                                    close(i); // finaliza a conexão
                                }

                                break;
                            case 4:   {
                                std::cout << "Received kill from " << server_header.msg_origem << endl;
                                // erases exibidor's position and closes socket
                                for (long unsigned int s = 0; s < exibidores.size(); s++)  {
                                    if (exibidores[s].id == server_header.msg_destino)   {
                                        close(exibidores[s].socket);
                                        exibidores.erase(exibidores.begin() + (s - 1));
                                    }
                                }

                                // erases emissor's vector position
                                for (long unsigned int s = 0; s < emissores.size(); s++)  {
                                    if (emissores[s].id == server_header.msg_origem)   {
                                        close(emissores[s].socket);
                                        emissores.erase(emissores.begin() + (s - 1));
                                    }
                                }
                                break;
                            }
                            case 5:  {
                                memset(buf, 0, BUFSZ);
                                size = 0;

                                nbytes = recv(i, &size, sizeof(size), 0); // receives the message size first
                                nbytes = recv(i, buf, size, 0);           // receives message
                                std::cout << "sent message from " << server_header.msg_origem << " to " << server_header.msg_destino << ": ";
                                std::cout << server_header.msg_tipo << " " << server_header.msg_origem << " " << server_header.msg_destino << " " << server_header.msg_contagem << std::endl;
                                int aux = 0;
                                if (server_header.msg_destino == 0)  {
                                    for (j = 0; j <= fdmax; j++)  {
                                        // send to everyone!
                                        if (FD_ISSET(j, &master))  {
                                            // except the listener and ourselves
                                            if (j != listener && j != i)  {
                                                send(j, &server_header, sizeof(header), 0);
                                                size = strlen(buf);
                                                send(j, &size, sizeof(size), 0); // sends message's size
                                                send(j, buf, size, 0);           // sends message

                                                if (send(j, &server_header, sizeof(header), 0) == -1)  {
                                                    perror("send");
                                                }
                                            }
                                        }
                                    }
                                }

                                else  {
    
                                for (long unsigned int s = 0; s <= exibidores.size(); s++)  {
                                        if (exibidores[s].id == server_header.msg_destino)   {
                                            send(exibidores[s].socket, &server_header, sizeof(header), 0);
                                            size = strlen(buf);
                                            std::cout << buf << endl;
                                            send(exibidores[s].socket, &size, sizeof(size), 0); // sends message's size
                                            send(exibidores[s].socket, buf, size, 0);           // sends message
                                            aux++;

                                            if (send(exibidores[s].socket, &server_header, sizeof(header), 0) == -1)  {
                                                perror("send");
                                            }
                                            break;
                                        }
                                    }
                                if (aux == 0) {// could not find specified exibidor  {
                                    server_header.msg_tipo = 2; // sendes "ERROR"(2) type message
                                    server_header.msg_destino = server_header.msg_origem;
                                    server_header.msg_origem = 65535;
                                    send(i, &server_header, sizeof(header), 0);
                                }
                                }
                                server_header.msg_tipo = 1; // sends "OK"(1) type message
                                send(i, &server_header, sizeof(header), 0);
                                break;
                            }
                            case 6: {
                                server_header.msg_tipo = 7; // sends CLIST
                                unsigned short N = emissores.size() + exibidores.size();
                                unsigned short clist[N];
                                for (long unsigned int s = 0; s < N; s++)  {
                                    if (s >= emissores.size())  {
                                        clist[s] = exibidores[N - s].id;
                                    }
                                    else {
                                        clist[s] = emissores[s].id;
                                    }
                                }

                                if (server_header.msg_destino == 0)  {
                                    for (j = 0; j <= fdmax; j++)  {
                                        // send to everyone!
                                        if (FD_ISSET(j, &master))  {
                                            // except the listener and ourselves
                                            if (j != listener && j != i)  {
                                                send(j, &server_header, sizeof(header), 0);
                                                send(j, &N, sizeof(N), 0);
                                                send(j, &clist, N, 0);
                                                if (send(j, &server_header, sizeof(header), 0) == -1)      {
                                                    perror("send");
                                                }
                                            }
                                        }
                                    }
                                }
                                else  {
                                    int aux = 0;
                                    for (long unsigned int s = 0; s <= exibidores.size(); s++)  {
                                        if (exibidores[s].id == server_header.msg_destino)  {
                                            send(exibidores[s].socket, &server_header, sizeof(header), 0);
                                            send(j, &N, sizeof(N), 0);
                                            send(j, &clist, N, 0);
                                            aux++;

                                            if (send(exibidores[s].socket, &server_header, sizeof(header), 0) == -1)   {
                                                perror("send");
                                            }
                                            break;
                                        }
                                    }

                                    if (aux == 0)  {// could not find specified exibidor   
                                        server_header.msg_tipo = 2; // sendes "ERROR"(2) type message
                                        server_header.msg_destino = server_header.msg_origem;
                                        server_header.msg_origem = 65535;
                                        send(i, &server_header, sizeof(header), 0);
                                    }
                                }
                                break;
                            }
                            case 8:   {
                                memset(buf, 0, BUFSZ);
                                nbytes = recv(i, &size, sizeof(size), 0); // receives the message size first
                                nbytes = recv(i, buf, size, 0);           // receives message

                                for (long unsigned int j = 0; j < exibidores.size(); j++)  {
                                    if (exibidores[j].id == server_header.msg_origem) {
                                        // save info in exibidores vector
                                        strtok(buf, " ");
                                        strtok(NULL, " ");
                                        char *NomeDoPlaneta = strtok(NULL, " ");
                                        std::cout << NomeDoPlaneta;
                                        std::cout << "received "<< NomeDoPlaneta << "from " << exibidores[j].id << ": ";
                                        std::cout << server_header.msg_tipo << " " << server_header.msg_origem << " " << 
                                            server_header.msg_destino << " " << server_header.msg_contagem << std::endl;
                                        break;
                                    }
                                }

                                for (long unsigned int j = 0; j < emissores.size(); j++)  {
                                    if (emissores[j].id == server_header.msg_origem)  {
                                        // save info in emissores vector
                                        strtok(buf, " ");
                                        strtok(NULL, " ");
                                        char *NomeDoPlaneta = strtok(NULL, " ");
                                        emissores[j].planet = NomeDoPlaneta;
                                        std::cout << "received "<< NomeDoPlaneta << "from " << emissores[j].id << ": ";
                                        std::cout << server_header.msg_tipo << " " << server_header.msg_origem << " " << 
                                            server_header.msg_destino << " " << server_header.msg_contagem << std::endl;
                                        break;
                                    }
                                }

                                server_header.msg_tipo = 1;                       // sends "OK"(1) type message
                                server_header.msg_destino = server_header.msg_origem; // back to the origin client
                                server_header.msg_origem = 65535;                 // from server
                                send(i, &server_header, sizeof(header), 0);      // sends message

                                break;
                            }
                            case 9: {
                                std::cout << "received planet from " << server_header.msg_origem << " to " << server_header.ID_Cliente_Planeta_Pedido << ": ";
                                std::cout << server_header.msg_tipo << " " << server_header.msg_origem << " " << server_header.ID_Cliente_Planeta_Pedido << " " << server_header.msg_contagem << std::endl;
                                int clientToFindPlanet = server_header.ID_Cliente_Planeta_Pedido;
                                int clientWhoAskedForPlanetExhibitor = server_header.msg_destino;
                                int aux = 0;

                                // find client that matches the id in the exibidores vector
                                for (long unsigned int j = 0; j < exibidores.size(); j++)  {
                                    if (exibidores[j].id == clientToFindPlanet)   {
                                        // send to the client
                                        aux = 1;
                                        server_header.msg_tipo = 9;
                                        send(exibidores[j].socket, &server_header, sizeof(header), 0);

                                        // assemble the message
                                        memset(buf, 0, BUFSZ);
                                        std::ostringstream msg;
                                        msg << "planet of " << clientToFindPlanet << ": " << exibidores[j].planet << std::endl;
                                        memcpy(buf, msg.str().c_str(), msg.str().size());
                                        break;
                                    }
                                }
                                if(aux == 0){
                                    // find client that matches the id in the emissores vector
                                    for (long unsigned int j = 0; j < emissores.size(); j++)  {
                                        if (emissores[j].id == clientToFindPlanet)   {
                                            // send to the client
                                            server_header.msg_tipo = 9;
                                            send(emissores[j].socket, &server_header, sizeof(header), 0);

                                            // assemble the message
                                            memset(buf, 0, BUFSZ);
                                            std::ostringstream msg;
                                            msg << "planet of " << clientToFindPlanet << ": " << emissores[j].planet << std::endl;
                                            memcpy(buf, msg.str().c_str(), msg.str().size());
                                            break;
                                        }
                                    }
                                }

                                for (long unsigned int j = 0; j < exibidores.size(); j++)  {
                                    if (exibidores[j].id == clientWhoAskedForPlanetExhibitor)  {
                                        send(exibidores[j].socket, &server_header, sizeof(header), 0);
                                        size = strlen(buf);
                                        send(exibidores[j].socket, &size, sizeof(size), 0); // sends message's size
                                        send(exibidores[j].socket, buf, size, 0); // sends message
                                        server_header.msg_tipo = 1; // sends "OK"(1) type message
                                        send(i, &server_header, sizeof(header), 0);
                                        break;
                                    }
                                }
                                break;
                            }
                            case 10:  {
                                std::string planetName;
                                std::vector<string> allSavedPlanets;
                                for(long unsigned int i = 0; i < exibidores.size(); i++){
                                    planetName = exibidores[i].planet;
                                    int newPlanet = 0;
                                    if(allSavedPlanets.size() == NULL){
                                        newPlanet = 1; 
                                    }
                                    else {
                                        for(unsigned short int k = 0; k < allSavedPlanets.size(); k++){
                                            if (planetName == allSavedPlanets[k]){
                                                newPlanet = 0;
                                                break;
                                            }
                                        }
                                    }
                                    if(newPlanet == 1){
                                        allSavedPlanets.push_back(planetName);
                                    }
                                }
                                for(long unsigned int i = 0; i < emissores.size(); i++){
                                    planetName = emissores[i].planet;
                                    int newPlanet = 0;
                                    if(allSavedPlanets.size() == NULL){
                                        newPlanet = 1; 
                                    }
                                    else {
                                        for(unsigned short int k = 0; k < allSavedPlanets.size(); k++){
                                            if (planetName == allSavedPlanets[k]){
                                                newPlanet = 0;
                                                break;
                                            }
                                        }
                                    }
                                    if(newPlanet == 1){
                                        allSavedPlanets.push_back(planetName);
                                    }
                                }
                                std::string allSavedPlanetsString; 
                                for (long unsigned int j = 0; j < allSavedPlanets.size(); j++)
                                {
                                    allSavedPlanetsString += allSavedPlanets[j];
                                }

                                std::cout << allSavedPlanetsString << std::endl;

                                server_header.msg_tipo = 1; // sends "OK"(1) type message
                                send(i, &server_header, sizeof(header), 0);

                                // send to the exhibtor
                                server_header.msg_tipo = 5;
                                send(i, &server_header, sizeof(header), 0);

                                memset(buf, 0, BUFSZ);
                                memcpy(buf, allSavedPlanetsString.c_str(), allSavedPlanetsString.size());
                                size = strlen(buf);

                                send(i, &size, sizeof(size), 0); // sends message's size

                                // sends message
                                send(i, buf, size, 0);

                                break;
                            }
                            default: {
                                printf("\nERROR\n");
                                exit(EXIT_FAILURE);
                                std::cout << "sent: " << server_header.msg_tipo << " " << server_header.msg_destino << " " << server_header.msg_origem << " " << server_header.msg_contagem << std::endl;
                                break;
                            }
                        }
                    }
                } // END handle data from client
            }     // END got new incoming connection
        }         // END looping through file descriptors
    }             // END for(;;)
    return 0;
}
