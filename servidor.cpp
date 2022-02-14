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

extern std::vector<string> allSavedPlanets;

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

    std::vector<client> exhibitors; // exhibtiors vector
    std::vector<client> issuers;    // issuers vector

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
    for (;;)
    {
        read_fds = master; // copy it
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for (i = 0; i <= fdmax; i++)
        {
            struct header servHeader;
            if (FD_ISSET(i, &read_fds))
            { // we got one!!
                if (i == listener)
                {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                                   (struct sockaddr *)&remoteaddr,
                                   &addrlen);

                    if (newfd == -1)
                    {
                        perror("accept");
                    }
                    else
                    {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax)
                        { // keep track of the max
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
                else
                {
                    // handle data from a client
                    if ((nbytes = recv(i, &servHeader, sizeof(header), 0)) <= 0)
                    {
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
                        if (servHeader.msgType != 9){
                            std::cout << "received from " << servHeader.msgOrigin << ": ";
                            std::cout << servHeader.msgType << " " << servHeader.msgOrigin << " " << servHeader.msgDestiny << " " << servHeader.msgOrder << std::endl;
                        }
                        switch (servHeader.msgType)
                        {

                        case 1:
                        { //"OK" message, from exhibitor to issuer
                            for (long unsigned int s = 0; s < issuers.size(); s++)
                            {
                                if (issuers[s].id == servHeader.msgDestiny)
                                {
                                    send(issuers[s].socket, &servHeader, sizeof(header), 0);
                                    break;
                                }
                            }

                            break;
                        }
                        case 3:
                            if (servHeader.msgOrigin == 0)
                            {
                                // recognizes client: exhibitor
                                struct client exhibitor;
                                exhibitor.socket = i;
                                exhibitors.push_back(exhibitor);
                                exhibitor.id = returnsID(exhibitors, 'e');
                                exhibitors.back().id = exhibitor.id;

                                servHeader.msgType = 1;
                                servHeader.msgDestiny = exhibitor.id;
                                servHeader.msgOrigin = 65535;

                                send(i, &servHeader, sizeof(header), 0);
                            }
                            else if (servHeader.msgOrigin > 0)
                            {
                                // recognize client: issuer
                                struct client issuer;
                                issuer.socket = i;
                                issuers.push_back(issuer);
                                issuer.id = returnsID(issuers, 'i');

                                // checks given exhibitors id:
                                int aux01 = -1;

                                for (long unsigned int t = 0; t < exhibitors.size(); t++)
                                {
                                    if (exhibitors[t].id == servHeader.msgDestiny)
                                    {
                                        aux01++;

                                        break;
                                    }
                                }

                                if (aux01 >= 0)
                                {
                                    // recognizes associated exhibitor's id
                                    servHeader.msgType = 1;
                                    servHeader.msgDestiny = issuer.id;
                                    servHeader.msgOrigin = 65535;
                                }
                                else
                                {
                                    // wrong exhibitor's id was sent
                                    servHeader.msgType = 2;
                                    servHeader.msgDestiny = servHeader.msgOrigin;
                                    servHeader.msgOrigin = 65535;
                                }
                                send(i, &servHeader, sizeof(header), 0);
                            }
                            else
                            {
                                // can't recognize client
                                servHeader.msgType = 2;
                                servHeader.msgDestiny = servHeader.msgOrigin;
                                servHeader.msgOrigin = 65535;
                                send(i, &servHeader, sizeof(header), 0);
                                close(i); // ends connection for safety
                            }

                            break;
                        case 4:
                        {
                            std::cout << "Received kill from " << servHeader.msgOrigin << endl;
                            // erases exhibitor's position and closes socket
                            for (long unsigned int s = 0; s < exhibitors.size(); s++)
                            {
                                if (exhibitors[s].id == servHeader.msgDestiny)
                                {
                                    close(exhibitors[s].socket);
                                    exhibitors.erase(exhibitors.begin() + (s - 1));
                                }
                            }

                            // erases issuer's vector position
                            for (long unsigned int s = 0; s < issuers.size(); s++)
                            {
                                if (issuers[s].id == servHeader.msgOrigin)
                                {
                                    close(issuers[s].socket);
                                    issuers.erase(issuers.begin() + (s - 1));
                                }
                            }
                            break;
                        }
                        case 5:
                        {
                            memset(buf, 0, BUFSZ);
                            size = 0;

                            nbytes = recv(i, &size, sizeof(size), 0); // receives the message size first
                            nbytes = recv(i, buf, size, 0);           // receives message
                            std::cout << "message content: " << buf << endl;
            
                            if (servHeader.msgDestiny == 0)
                            {
                                for (j = 0; j <= fdmax; j++)
                                {
                                    // send to everyone!
                                    if (FD_ISSET(j, &master))
                                    {
                                        // except the listener and ourselves
                                        if (j != listener && j != i)
                                        {
                                            send(j, &servHeader, sizeof(header), 0);
                                            size = strlen(buf);
                                            send(j, &size, sizeof(size), 0); // sends message's size
                                            send(j, buf, size, 0);           // sends message

                                            if (send(j, &servHeader, sizeof(header), 0) == -1)
                                            {
                                                perror("send");
                                            }
                                        }
                                    }
                                }
                            }

                            else
                            {
                                int aux = 0;
   
                               for (long unsigned int s = 0; s <= exhibitors.size(); s++)
                                {
                                    if (exhibitors[s].id == servHeader.msgDestiny)
                                    {
                                        send(exhibitors[s].socket, &servHeader, sizeof(header), 0);
                                        size = strlen(buf);
                                        send(exhibitors[s].socket, &size, sizeof(size), 0); // sends message's size
                                        send(exhibitors[s].socket, buf, size, 0);           // sends message
                                        aux++;

                                        if (send(exhibitors[s].socket, &servHeader, sizeof(header), 0) == -1)
                                        {
                                            perror("send");
                                        }
                                        break;
                                    }
                                }
                                    if (aux == 0) // could not find specified exhibitor
                                    {
                                        servHeader.msgType = 2; // sendes "ERROR"(2) type message
                                        servHeader.msgDestiny = servHeader.msgOrigin;
                                        servHeader.msgOrigin = 65535;
                                        send(i, &servHeader, sizeof(header), 0);
                                    }
                            }
                            servHeader.msgType = 1; // sends "OK"(1) type message
                            send(i, &servHeader, sizeof(header), 0);
                            break;
                        }
                        case 6:
                        {
                            servHeader.msgType = 7; // sends CLIST
                            unsigned short N = issuers.size() + exhibitors.size();
                            unsigned short clist[N];
                            for (long unsigned int s = 0; s < N; s++)
                            {
                                if (s >= issuers.size())
                                {
                                    clist[s] = exhibitors[N - s].id;
                                }
                                else
                                {
                                    clist[s] = issuers[s].id;
                                }
                            }

                            if (servHeader.msgDestiny == 0)
                            {
                                for (j = 0; j <= fdmax; j++)
                                {
                                    // send to everyone!
                                    if (FD_ISSET(j, &master))
                                    {
                                        // except the listener and ourselves
                                        if (j != listener && j != i)
                                        {
                                            send(j, &servHeader, sizeof(header), 0);
                                            send(j, &N, sizeof(N), 0);
                                            send(j, &clist, N, 0);
                                            if (send(j, &servHeader, sizeof(header), 0) == -1)
                                            {
                                                perror("send");
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                int aux = 0;
                                for (long unsigned int s = 0; s <= exhibitors.size(); s++)
                                {
                                    if (exhibitors[s].id == servHeader.msgDestiny)
                                    {
                                        send(exhibitors[s].socket, &servHeader, sizeof(header), 0);
                                        send(j, &N, sizeof(N), 0);
                                        send(j, &clist, N, 0);
                                        aux++;

                                        if (send(exhibitors[s].socket, &servHeader, sizeof(header), 0) == -1)
                                        {
                                            perror("send");
                                        }
                                        break;
                                    }
                                }

                                if (aux == 0)  // could not find specified exhibitor
                                {
                                    servHeader.msgType = 2; // sendes "ERROR"(2) type message
                                    servHeader.msgDestiny = servHeader.msgOrigin;
                                    servHeader.msgOrigin = 65535;
                                    send(i, &servHeader, sizeof(header), 0);
                                }
                            }
                            break;
                        }
                        case 8:
                        {
                            memset(buf, 0, BUFSZ);
                            nbytes = recv(i, &size, sizeof(size), 0); // receives the message size first
                            nbytes = recv(i, buf, size, 0);           // receives message

                            for (long unsigned int j = 0; j < exhibitors.size(); j++)
                            {
                                if (exhibitors[j].id == servHeader.msgOrigin)
                                {
                                    // save info in exhibitors vector
                                    strtok(buf, " ");
                                    strtok(NULL, " ");
                                    char *nameOfPlanet = strtok(NULL, " ");
                                    exhibitors[j].planet = nameOfPlanet;
                                    break;
                                }
                            }

                            for (long unsigned int j = 0; j < issuers.size(); j++)
                            {
                                if (issuers[j].id == servHeader.msgOrigin)
                                {
                                    // save info in issuers vector
                                    issuers[j].planet = buf;
                                    break;
                                }
                            }

                            servHeader.msgType = 1;                       // sends "OK"(1) type message
                            servHeader.msgDestiny = servHeader.msgOrigin; // back to the origin client
                            servHeader.msgOrigin = 65535;                 // from server
                            send(i, &servHeader, sizeof(header), 0);      // sends message

                            break;
                        }
                        case 9:
                        {
                            std::cout << "received planet from " << servHeader.msgOrigin << " to " << servHeader.ClientToFindPlanetName << ": ";
                            std::cout << servHeader.msgType << " " << servHeader.msgOrigin << " " << servHeader.ClientToFindPlanetName << " " << servHeader.msgOrder << std::endl;
                            int clientToFindPlanet = servHeader.ClientToFindPlanetName;
                            int clientWhoAskedForPlanetExhibitor = servHeader.msgDestiny;
                            std::cout << "Exibidor do cliente que pediu:" << clientWhoAskedForPlanetExhibitor;
                            int aux = 0;

                            // find client that matches the id in the exhibitors vector
                            for (long unsigned int j = 0; j < exhibitors.size(); j++)
                            {
                                if (exhibitors[j].id == clientToFindPlanet)
                                {
                                    // send to the client
                                    aux = 1;
                                    servHeader.msgType = 10;
                                    send(exhibitors[j].socket, &servHeader, sizeof(header), 0);

                                    // assemble the message
                                    memset(buf, 0, BUFSZ);
                                    std::ostringstream msg;
                                    msg << "planet of " << clientToFindPlanet << ": " << exhibitors[j].planet << std::endl;
                                    memcpy(buf, msg.str().c_str(), msg.str().size());
                                    break;
                                }
                            }
                            if(aux == 0){
                                // find client that matches the id in the issuers vector
                                for (long unsigned int j = 0; j < issuers.size(); j++)
                                {
                                    if (issuers[j].id == clientToFindPlanet)
                                    {
                                        // send to the client
                                        servHeader.msgType = 5;
                                        send(issuers[j].socket, &servHeader, sizeof(header), 0);

                                        // assemble the message
                                        memset(buf, 0, BUFSZ);
                                        std::ostringstream msg;
                                        msg << "planet of " << clientToFindPlanet << ": " << issuers[j].planet << std::endl;
                                        memcpy(buf, msg.str().c_str(), msg.str().size());
                                        break;
                                    }
                                }
                            }

                            for (long unsigned int j = 0; j < exhibitors.size(); j++)
                            {
                                if (exhibitors[j].id == clientWhoAskedForPlanetExhibitor)
                                {
                                size = strlen(buf);
                                send(exhibitors[j].socket, &size, sizeof(size), 0); // sends message's size
                                send(exhibitors[j].socket, buf, size, 0); // sends message
                                servHeader.msgType = 1; // sends "OK"(1) type message
                                send(i, &servHeader, sizeof(header), 0);
                                break;
                                }
                            }
                            break;
                        }
                        case 10:
                        {
                           
                            std::string allSavedPlanetsString;
                            for (long unsigned int j = 0; j < allSavedPlanets.size(); j++)
                            {
                                allSavedPlanetsString += allSavedPlanets[j];
                            }

                            std::cout << allSavedPlanetsString << std::endl;

                            servHeader.msgType = 1; // sends "OK"(1) type message
                            send(i, &servHeader, sizeof(header), 0);

                            // send to the exhibtor
                            servHeader.msgType = 5;
                            send(i, &servHeader, sizeof(header), 0);

                            memset(buf, 0, BUFSZ);
                            memcpy(buf, allSavedPlanetsString.c_str(), allSavedPlanetsString.size());
                            size = strlen(buf);

                            send(i, &size, sizeof(size), 0); // sends message's size

                            // sends message
                            send(i, buf, size, 0);

                            break;
                        }
                        default:
                        {
                            printf("\nERROR\n");
                            exit(EXIT_FAILURE);
                            std::cout << "sent: " << servHeader.msgType << " " << servHeader.msgDestiny << " " << servHeader.msgOrigin << " " << servHeader.msgOrder << std::endl;
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
