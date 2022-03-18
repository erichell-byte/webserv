#pragma once
#ifndef __LISTENER_HPP
#define __LISTENER_HPP

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cerrno>
#include <map>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

#include "server.hpp"
#include "requestMessage.hpp"
#include "responseMessage.hpp"

#include "client.hpp"

#define MAX_CLIENTS 10


class Listener 
{
private:
	
public:
	std::vector<server> servers;
	struct pollfd 		fds[MAX_CLIENTS + 1];
	int 				clientsNumb;
	int					serversNumb;

	std::map<int, Client> client;

	int 	createSocket(server instance);
	int		listenPoll(void);
	void	acceptConnection(int i);
	void	receiveFromClient(int i);
	void 	sendAllData(int i);

	std::string to_string(int n);
};

#endif
