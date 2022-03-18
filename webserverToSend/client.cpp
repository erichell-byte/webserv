#include "client.hpp"

Client::Client()
{
	this->whatToReceive = HEADERS;
	this->howManyReceive = BUF_SIZE;
	this->requestIsReady = 0;
}

void	Client::receiveHeaders(int i, char* sendBuff)
{
    std::string 	headers;
    std::size_t 	pos;
    ResponseMessage response;

    std::cout << "Start receiveHeaders ..." << std::endl;

    pos = this->incomeMessage.find("\r\n\r\n");
    if (pos == std::string::npos)
        return ;
    
    pos = this->incomeMessage.find("\r\n\r\n");
    headers = this->incomeMessage.substr(0, pos);
    this->incomeMessage = this->incomeMessage.substr(pos + 4);
    this->request.parceRequest(headers);

    // если нет тела сообщения
    if (this->request.headers.find("Transfer-Encoding") == this->request.headers.end()
        &&  this->request.headers.find("Content-Length") == this->request.headers.end())
    {
        this->bytesSend = 0;
		this->requestIsReady = 1;
    }
    else
        this->handleTailMessage();
}

void Client::handleTailMessage()
{
    int         chunkSize = 0;
    int         result = 0;

    std::cout << "Start handleTailMessage ..." << std::endl;
    if (this->request.headers.find("Transfer-Encoding") != this->request.headers.end())
    {
        this->whatToReceive = CHUNKEDSIZE;
		while (result != -1)
        {
            chunkSize = this->chunkSize();
			if (chunkSize == -1)
				break ;
			if (chunkSize == 0)
				{
					this->chunkTrailer();
					break ;
				} 
            result = this->chunkData();	
        }
    }

     if (this->request.headers.find("Content-Length") != this->request.headers.end())
     {
        this->whatToReceive = CONTENTLENGTH;
        std::string howManyReceive = this->request.headers.find("Content-Length")->second;
        this->howManyReceive = 4 + atoi(howManyReceive.c_str() );
		this->contentLength();
     }
}

void Client::contentLength()
{
    std::size_t posCRLF;

    if (this->howManyReceive > this->incomeMessage.size())
    {
        this->howManyReceive = this->howManyReceive - this->incomeMessage.size();
        return  ;
    }
    else
    {
        posCRLF = this->incomeMessage.find("\r\n\r\n");
        this->request.body = incomeMessage.substr(0, posCRLF);
		this->incomeMessage =  this->incomeMessage.substr(posCRLF + 4);
		this->requestIsReady = 1;
        this->whatToReceive = HEADERS;
        this->howManyReceive = BUF_SIZE;
		return ;
    }
}

int    Client::chunkSize()
{
    std::string strChunkSize;
    int         chunkSize;
    std::size_t posCRLF;
    std::size_t posSpace;

    posCRLF = this->incomeMessage.find("\r\n");
    if (posCRLF == std::string::npos)
    {
        this->howManyReceive = 1;
        return (-1);
    }

//     узнаем размер чанка       
    strChunkSize = this->incomeMessage.substr(0, posCRLF);
    posSpace = strChunkSize.find(" ");
    strChunkSize = strChunkSize.substr(0, posSpace);
    std::stringstream ss;
    ss << std::hex << strChunkSize;
    ss >> chunkSize;

    this->incomeMessage = this->incomeMessage.substr(posCRLF + 2);
    if (chunkSize == 0)
    {
        this->whatToReceive = CHUNKEDTRAILER;
        this->howManyReceive = BUF_SIZE;
    }
    else 
    {
        this->whatToReceive = CHUNKEDDATA;
        this->howManyReceive = chunkSize + 2;
    }
    return (chunkSize);
}

int Client::chunkData()
{
    std::size_t posCRLF;
    int chunkSize = this->howManyReceive - 2;
    
    if (chunkSize > this->incomeMessage.size())
    {
        this->request.body += this->incomeMessage.substr(0);
        this->incomeMessage.clear();
        this->howManyReceive = chunkSize - this->incomeMessage.size();
        return (-1);
    }
    else
    {
        this->request.body += this->incomeMessage.substr(0, chunkSize);
        std::string debugdecoded =  this->request.body;
        this->incomeMessage = this->incomeMessage.substr(chunkSize + 2);
        this->whatToReceive = CHUNKEDSIZE;
        this->howManyReceive = 1;
    }
    return (0);
}

void	Client::chunkTrailer()
{
    if (this->incomeMessage == "\r\n")
        this->incomeMessage.clear();
	else
	{
		std::size_t posCRLF = incomeMessage.find("\r\n\r\n");
		if (posCRLF == std::string::npos)
			return ;
		this->incomeMessage = this->incomeMessage.substr(posCRLF + 4);
	}
	this->whatToReceive = HEADERS;
	this->howManyReceive = BUF_SIZE;
	this->requestIsReady = 1;
	return ;
}
