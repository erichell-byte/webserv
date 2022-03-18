#include "Client.hpp"


Client::Client(int clientSocket, fd_set *new_read_set, fd_set *new_write_set, struct sockaddr_in info)
		:  status(STANDBY),  cgi_pid(-1),  tmp_fd(-1),  read_set(new_read_set),  write_set(new_write_set), chunk(), clientSock(clientSocket), read_fd(-1), write_fd(-1)
{
	ip = inet_ntoa(info.sin_addr);
	port = htons(info.sin_port);
	buffer = (char *) malloc(sizeof(char) * (BUFFER_SIZE + 1));
	memset((void *) buffer, 0, BUFFER_SIZE + 1);
	fcntl(clientSock, F_SETFL, O_NONBLOCK);
	FD_SET(clientSock, read_set);
	FD_SET(clientSock, write_set);
	chunk.length = 0;
	chunk.isDone = false;
	chunk.isFound = false;
	lastDate = ft::getDate();
	utils::showMessage("new connection from " + ip + ":" + std::to_string(port), GREEN);
}

Client::~Client()
{
	free(buffer);
	buffer = nullptr;
	if (clientSock != -1)
	{
		close(clientSock);
		FD_CLR(clientSock, read_set);
		FD_CLR(clientSock, write_set);
	}
	if (read_fd != -1)
	{
		close(read_fd);
		FD_CLR(read_fd, read_set);
	}
	if (write_fd != -1)
	{
		close(write_fd);
		FD_CLR(write_fd, write_set);
	}
	if (tmp_fd != -1)
	{
		close(tmp_fd);
		unlink(TMP_PATH);
	}
	utils::showMessage("connection closed from " + ip + ":" + std::to_string(port), RED);
}

void Client::setReadState(bool state)
{
	if (state)
		FD_SET(clientSock, read_set);
	else
		FD_CLR(clientSock, read_set);
}

void Client::setWriteState(bool state)
{
	if (state)
		FD_SET(clientSock, write_set);
	else
		FD_CLR(clientSock, write_set);
}

void Client::setFileToRead(bool state)
{
	if (read_fd != -1)
	{
		if (state)
			FD_SET(read_fd, read_set);
		else
			FD_CLR(read_fd, read_set);
	}
}

void Client::setFileToWrite(bool state)
{
	if (write_fd != -1)
	{
		if (state)
			FD_SET(write_fd, write_set);
		else
			FD_CLR(write_fd, write_set);
	}
}

void Client::readFile()
{
	char tmpBuffer[BUFFER_SIZE + 1];
	int result;
	int cgiStatus = 0;

	if (cgi_pid != -1)
	{
		if (waitpid((pid_t) cgi_pid, (int *) &cgiStatus, (int) WNOHANG) == 0)
			return;
		else
		{
			if (WEXITSTATUS(cgiStatus) == 1)
			{
				close(tmp_fd);
				tmp_fd = -1;
				cgi_pid = -1;
				close(read_fd);
				unlink(TMP_PATH);
				setFileToRead(false);
				read_fd = -1;
				response.body = "Error with cgi\n";
				return;
			}
		}
	}
	result = read(read_fd, tmpBuffer, BUFFER_SIZE);
	if (result >= 0)
		tmpBuffer[result] = '\0';
	std::string tmp(tmpBuffer, result);
	response.body += tmp;
	if (result == 0)
	{
		close(read_fd);
		unlink(TMP_PATH);
		setFileToRead(false);
		read_fd = -1;
	}
}

void Client::writeFile()
{
	int result = 0;
	result = write(write_fd, request.body.c_str(), request.body.size());
	if (cgi_pid != -1)
		utils::showMessage("sent " + std::to_string(result) + " bytes to CGI stdin", GREEN);
	else
		utils::showMessage("write " + std::to_string(result) + " bytes in file", GREEN);
	if ((unsigned long) result < request.body.size())
		request.body = request.body.substr(result);
	else
	{
		request.body.clear();
		close(write_fd);
		setFileToWrite(false);
		write_fd = -1;
	}
}

void Client::setToStandBy()
{
	utils::showMessage(request.method + " from " + ip + ":" + std::to_string(port) + " answered", YELLOW);
	status = STANDBY;
	setReadState(true);
	if (read_fd != -1)
		close(read_fd);
	read_fd = -1;
	if (write_fd != -1)
		close(write_fd);
	write_fd = -1;
	memset((void *) buffer, (int) 0, (size_t) (BUFFER_SIZE + 1));
	clientConfig.clear();
	request.clear();
	response.clear();
}
