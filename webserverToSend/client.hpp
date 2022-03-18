#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include "responseMessage.hpp"

#define BUF_SIZE 500
enum
{
	HEADERS = 1,
	CONTENTLENGTH = 2,
	CHUNKEDSIZE = 3,
	CHUNKEDDATA = 4,
	CHUNKEDTRAILER = 5
};

class Client
{
private:

public:
	int			requestIsReady;
	int			tmp_fd;
	int 		cgi_pid;
	int			read_fd;
	int			write_fd;


// to receive
	int				whatToReceive;
	int				howManyReceive;
	RequestMessage	request;
	std::string		incomeMessage;


// to sent
	int				bytesSend;
	int				bytesLeft;
	std::string		messageToSendAsStr;
	const char*		messageToSend;

	Client();

	void	receiveHeaders(int i, char* buffer);
	void 	handleTailMessage();
	int 	chunkSize();
	int 	chunkData();
	void	chunkTrailer();
	void 	contentLength();
};
