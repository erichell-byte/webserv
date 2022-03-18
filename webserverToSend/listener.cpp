#include "listener.hpp"
#include "responseMessage.hpp"
#include <string>

int Listener::createSocket(server instance)
{
	int         socketfd;
    sockaddr_in serv_addr;

    std::memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(instance.host.c_str());
    serv_addr.sin_port = htons(instance.port);
    if (serv_addr.sin_addr.s_addr == -1)
        perror("inet_addr");

    socketfd = socket(PF_INET, SOCK_STREAM, 0);
    if (socketfd == -1)
    {
        std::cerr << strerror(errno) << std::endl;
        std::exit(1);
    }
 
    int yes = 1;
    int setsockoptResult = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if ( setsockoptResult == -1)
    {
        std::cerr << strerror(errno) << std::endl;
        std::exit(1);
    }

    int bindResult = bind(socketfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (bindResult == -1)
    {
        std::cerr << strerror(errno) << std::endl;
        std::exit(1);
    }

    int listenResult = listen(socketfd, 10);
    if (listenResult == -1)
    {
        std::cerr << strerror(errno) << std::endl;
        std::exit(1);
    }

    return (socketfd);
}

void	Listener::acceptConnection(int i)
{
    int     clientfd;
    int     n = serversNumb;

    std::cout << "Accept connection " << std::endl;
    clientfd = accept(fds[i].fd, (struct sockaddr*)NULL, NULL);
 //   fcntl(clientfd, F_SETFL, O_NONBLOCK);
    fcntl(clientfd, F_SETFL, fcntl(fds[i].fd, F_GETFL) | O_NONBLOCK);
    
    while (n < MAX_CLIENTS)
    {
        if (fds[n].fd == 0)
        {
            fds[n].fd = clientfd;
            fds[n].events = POLLIN;
            clientsNumb++;
            break;
        }
        n++;
    }
}

void Listener::receiveFromClient(int i)
{
    int         inputMsgSize;
    char        sendBuff[BUF_SIZE];
    std::string fromBuffer;
    ResponseMessage response;

    std::cout << "Start receiving ..." << std::endl;
    
    std::memset(sendBuff, '0', sizeof(sendBuff));
    inputMsgSize = recv(fds[i].fd, sendBuff , this->client[i].howManyReceive, 0);
    if (inputMsgSize <= 0)
    {
        std::cout << "close connection" << std::endl;
        close(fds[i].fd);
        fds[i].fd = 0;
        fds[i].events = 0;
        fds[i].revents = 0;
        this->client[i] = Client();
        clientsNumb--;
        return ;
    }
    sendBuff[inputMsgSize] = 0;
    fromBuffer = sendBuff;
    this->client[i].incomeMessage += fromBuffer;

    switch (this->client[i].whatToReceive)
    {
    case HEADERS:
        this->client[i].receiveHeaders(i, sendBuff);
        break;
    case CONTENTLENGTH:
        this->client[i].contentLength();
        break;
    case CHUNKEDSIZE:
        this->client[i].chunkSize();
        break;
    case CHUNKEDDATA:
        this->client[i].chunkData();
        break;        
    case CHUNKEDTRAILER:
        this->client[i].chunkTrailer();
        break;                    
    default:
        break;
    }

    if (this->client[i].requestIsReady == 1)
    {
        this->client[i].request.debugPrint();
        this->client[i].messageToSendAsStr = response.PrepareResponse(this->client[i], servers);
        this->client[i].messageToSend = this->client[i].messageToSendAsStr.c_str();
        this->client[i].bytesLeft = this->client[i].messageToSendAsStr.size();
        this->client[i].bytesSend = 0;
        fds[i].events = POLLIN | POLLOUT;
        this->client[i].requestIsReady = 0;
    }
}



void Listener::sendAllData(int i)
{
    int n;

    std::cout << "Start sendAllData ..." << std::endl;
    std::cout << std::endl << "RESPONCE to send: " << client[i].messageToSendAsStr << std::endl;

    n = send(fds[i].fd, 
        client[i].messageToSend + client[i].bytesSend, 
         client[i].bytesLeft, 
        0);
    if (n == -1)
        return ;
    this->client[i].bytesSend += n;
    this->client[i].bytesLeft -= n;
    if (client[i].bytesLeft == 0)
        fds[i].events = POLLIN;
    return ;
}

int	Listener::listenPoll(void)
{
    int socketfd;
    int i = 0;

    clientsNumb = 0;
    serversNumb = servers.size();
   
    std::memset(fds, 0, (MAX_CLIENTS + 1) * sizeof(struct pollfd));

    for (std::vector<server>::const_iterator it = this->servers.begin(); 
		it != this->servers.end(); ++it, ++i)		
	{
        socketfd = this->createSocket(servers[i]);
        fds[i].fd = socketfd;
        fds[i].events = POLLIN | POLLOUT;
    }

    while(1) 
	{
        i = 0;
        int pollResult = poll(fds, clientsNumb + serversNumb, -1);
        if (pollResult > 0)
        {
            std::cout << "start poll" << std::endl;
            while (i < MAX_CLIENTS)
            {
                if ((fds[i].revents & POLLIN) && (i < serversNumb))
                    acceptConnection(i++);
                if ((fds[i].revents & POLLIN) && (i >= serversNumb))
                    receiveFromClient(i++);      
                if (fds[i].revents & POLLOUT)
                    sendAllData(i++);
                i++;             
            } 
        }
    }
	 return (0);
}


std::string Listener::to_string(int n)
{
    char buf[40];
    sprintf(buf,"%d",n);
    return buf;
}
