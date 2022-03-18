#include "Server.hpp"

Server::Server() : serverSocket(-1), maxFd(-1), port(-1)
{
	memset(&addr, 0, sizeof(addr));
}

Server::~Server()
{
	Client *client = nullptr;

	if (serverSocket != -1)
	{
		for (std::vector<Client *>::iterator it(clients.begin()); it != clients.end(); ++it)
		{
			client = *it;
			*it = NULL;
			if (client)
				delete client;
		}
		while (!tmpClients.empty())
		{
			close(tmpClients.front());
			tmpClients.pop();
		}
		clients.clear();
		close(serverSocket);
		FD_CLR(serverSocket, read_set);
		utils::showMessage("[" + std::to_string(port) + "] " + "closed", GREEN);
	}
}

int Server::getMaxFd()
{
	Client *client;

	for (std::vector<Client *>::iterator it(clients.begin()); it != clients.end(); ++it)
	{
		client = *it;
		if (client->read_fd > maxFd)
			maxFd = client->read_fd;
		if (client->write_fd > maxFd)
			maxFd = client->write_fd;
	}
	return (maxFd);
}

int Server::getFd() const
{
	return (serverSocket);
}

int Server::getOpenFd()
{
	int nb = 0;
	Client *client;

	for (std::vector<Client *>::iterator it(clients.begin()); it != clients.end(); ++it)
	{
		client = *it;
		nb += 1;
		if (client->read_fd != -1)
			nb += 1;
		if (client->write_fd != -1)
			nb += 1;
	}
	nb += tmpClients.size();
	return (nb);
}

void Server::init(fd_set *readSet, fd_set *writeSet)
{
	int yes = 1;
	std::string to_parse;
	std::string host;

	write_set = writeSet;
	read_set = readSet;

	to_parse = config[0]["server|"]["listen"];
	errno = 0;
	if ((serverSocket = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		throw (WebservException("socket()", std::string(strerror(errno))));
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) //без задержки
		throw (WebservException("setsockopt()", std::string(strerror(errno))));
	if (to_parse.find(':') != std::string::npos)
	{
		host = to_parse.substr(0, to_parse.find(":"));
		if ((port = atoi(to_parse.substr(to_parse.find(":") + 1).c_str())) < 0)
			throw (WebservException("Wrong port", std::to_string(port)));
		addr.sin_addr.s_addr = inet_addr(host.c_str());
		addr.sin_port = htons(port);
	} else
	{
		addr.sin_addr.s_addr = INADDR_ANY;
		if ((port = atoi(to_parse.c_str())) < 0)
			throw (WebservException("Wrong port", std::to_string(port)));
		addr.sin_port = htons(port);
	}
	addr.sin_family = AF_INET;
	if (bind(serverSocket, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		throw (WebservException("bind()", std::string(strerror(errno)))); //связывает сокет и порт
	if (listen(serverSocket, 256) == -1)
		throw (WebservException("listen()", std::string(strerror(errno)))); //переводим сокет в слушание
	if (fcntl(serverSocket, F_SETFL, O_NONBLOCK) == -1)
		throw (WebservException("fcntl()", std::string(strerror(errno))));
	FD_SET(serverSocket, read_set);
	maxFd = serverSocket;
	utils::showMessage("[" + std::to_string(port) + "] " + "listening...", BLUE);
}

void Server::refuseConnection()
{
	int fd = -1;
	struct sockaddr_in info = {};
	socklen_t len;

	errno = 0;
	len = sizeof(struct sockaddr);
	if ((fd = accept(serverSocket, (struct sockaddr *) &info, &len)) == -1)
		throw (WebservException("accept()", std::string(strerror(errno))));
	if (tmpClients.size() < 10)
	{
		tmpClients.push(fd);
		FD_SET(fd, write_set);
	} else
		close(fd);
}

void Server::acceptConnection()
{
	int fd = -1;
	struct sockaddr_in info = {};
	socklen_t len;
	Client *newOne = nullptr;

	memset(&info, 0, sizeof(struct sockaddr));
	errno = 0;
	len = sizeof(struct sockaddr);
	if ((fd = accept(serverSocket, (struct sockaddr *) &info, &len)) == -1)
		throw (WebservException("accept()", std::string(strerror(errno))));
	if (fd > maxFd)
		maxFd = fd;
	newOne = new Client(fd, read_set, write_set, info);
	clients.push_back(newOne);
	utils::showMessage("[" + std::to_string(port) + "] " + "connected clients: " + std::to_string(clients.size()), UNDER WHITE);
}

int Server::readRequest(std::vector<Client *>::iterator it)
{
	int bytes;
	int ret;
	Client *client = nullptr;
	std::string log;

	client = *it;
	bytes = strlen(client->buffer);
	ret = read(client->clientSock, client->buffer + bytes, BUFFER_SIZE - bytes);
	bytes += ret;
	if (ret > 0)
	{
		client->buffer[bytes] = '\0';
		if (strstr(client->buffer, "\r\n\r\n") != nullptr
			&& client->status != BODYPARSING)
		{
			log = "REQUEST:\n";
			log += client->buffer;
			utils::showMessage(log, GREEN);
			client->lastDate = ft::getDate();
			http.parseRequest(*client, config);
			client->setWriteState(true);
		}


		if (client->status == BODYPARSING)
			http.parseBody(*client);
		return (1);
	} else
	{
		*it = NULL;
		clients.erase(it);
		delete client;
		utils::showMessage("[" + std::to_string(port) + "] " + "connected clients: " + std::to_string(clients.size()), WHITE);
		return (0);
	}
}

int Server::writeResponse(std::vector<Client *>::iterator it)
{
	unsigned long bytes;
	std::string tmp;
	std::string log;
	Client *client = nullptr;

	client = *it;
	switch (client->status)
	{
		case RESPONSE:
			log = "RESPONSE:\n";
			log += client->textResponse.substr(0, 128);
			utils::showMessage(log, BLUE);
			bytes = write(client->clientSock, client->textResponse.c_str(), client->textResponse.size());
			if (bytes < client->textResponse.size() && bytes >= 0)
				client->textResponse = client->textResponse.substr(bytes);
			else
			{
				client->textResponse.clear();
				client->setToStandBy();
			}
			client->lastDate = ft::getDate();
			break;
		case STANDBY:
			if (getTimeDiff(client->lastDate) >= TIMEOUT)
				client->status = DONE;
			break;
		case DONE:
			delete client;
			clients.erase(it);
			utils::showMessage("[" + std::to_string(port) + "] " + "connected clients: " + std::to_string(clients.size()), UNDER WHITE);
			return (0);
		default:
			http.dispatcher(*client);
	}
	return (1);
}

void Server::sendRefuseConnection(int fd)
{
	Response response;
	std::string str;
	int ret = 0;

	response.version = "HTTP/1.1";
	response.statusCode = UNAVAILABLE;
	response.headers["Retry-After"] = RETRY;
	response.headers["Date"] = ft::getDate();
	response.headers["Server"] = "webserv";
	response.body = UNAVAILABLE;
	response.headers["Content-Length"] = std::to_string(response.body.size());
	std::map<std::string, std::string>::const_iterator b = response.headers.begin();
	str = response.version + " " + response.statusCode + "\r\n";
	while (b != response.headers.end())
	{
		if (b->second != "")
			str += b->first + ": " + b->second + "\r\n";
		++b;
	}
	str += "\r\n";
	str += response.body;
	ret = write(fd, str.c_str(), str.size());
	if (ret >= -1)
	{
		close(fd);
		FD_CLR(fd, write_set);
		tmpClients.pop();
	}
	utils::showMessage("[" + std::to_string(port) + "] " + "connection refused, sent 503", RED);
}


int Server::getTimeDiff(const std::string &start)
{
	struct tm start_tm = {};
	struct tm *now_tm;
	struct timeval time = {};
	int result;

	strptime(start.c_str(), "%a, %d %b %Y %T", &start_tm);
	gettimeofday(&time, nullptr);
	now_tm = localtime(&time.tv_sec);
	result = (now_tm->tm_hour - start_tm.tm_hour) * 3600;
	result += (now_tm->tm_min - start_tm.tm_min) * 60;
	result += (now_tm->tm_sec - start_tm.tm_sec);
	return (result);
}
