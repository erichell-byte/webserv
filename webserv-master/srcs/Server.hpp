#ifndef SERVER_HPP
#define SERVER_HPP

#include "HTTP.hpp"

class Server
{
	friend class Webserv;

private:
	int serverSocket;
	int maxFd;
	int port;
	struct sockaddr_in addr;
	fd_set *read_set;
	fd_set *write_set;
	HTTP http;

public:
	std::vector<Client *> clients;
	std::queue<int> tmpClients;
	std::vector<Config> config;

	Server();

	virtual ~Server();

	int getMaxFd();

	int getFd() const;

	int getOpenFd();

	void init(fd_set *readSet, fd_set *writeSet);

	void refuseConnection();

	void acceptConnection();

	int readRequest(std::vector<Client *>::iterator it);

	int writeResponse(std::vector<Client *>::iterator it);

	void sendRefuseConnection(int fd);

	static int getTimeDiff(const std::string &start);
};

#endif
