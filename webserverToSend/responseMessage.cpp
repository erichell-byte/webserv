#include "responseMessage.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <cstdlib>

#include <map>
#include <queue>
#include <vector>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include "fstream"
#include <dirent.h>


ResponseMessage::ResponseMessage(){}

ResponseMessage::~ResponseMessage()
{
    this->fullResponse.clear();
    this->answerNum.clear();
    this->bodyResponse.clear();
    this->contentType.clear();
    this->headerResponse.clear(); 
}

char **ResponseMessage::setEnv(Client &client)
{
    char **env;
	std::map<std::string, std::string> envMap;
	size_t pos;
    envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	envMap["SERVER_SOFTWARE"] = "webserv";
    envMap["REQUEST_URI"] = client.request.target;
    envMap["REQUEST_METHOD"] = client.request.method;
    envMap["REMOTE_ADDR"] = client.request.headers["Host"].substr(0,client.request.headers["Host"].find_last_of(":"));
    envMap["PATH_INFO"] = client.request.target;
    envMap["PATH_TRANSLATED"] = "post.php";
    envMap["CONTENT_LENGTH"] = std::to_string(0);
    if (client.request.target.find('?') != std::string::npos)
		envMap["QUERY_STRING"] = client.request.target.substr(client.request.target.find('?') + 1);
	else
		envMap["QUERY_STRING"];
    if (client.request.headers.find("Content-Type") != client.request.headers.end())
		envMap["CONTENT_TYPE"] = client.request.headers["Content-Type"];
    envMap["SCRIPT_NAME"] = "post.php";
    envMap["SERVER_PORT"] = client.request.headers["Host"].substr(client.request.headers["Host"].find_first_of(":"));
    if (client.request.target.find(".php") != std::string::npos)
        envMap["REDIRECT_STATUS"] = "200";
    std::map<std::string, std::string>::iterator b = client.request.headers.begin();
	while (b != client.request.headers.end())
	{
		envMap["HTTP_" + b->first] = b->second;
		++b;
	}
    env = (char **) malloc(sizeof(char *) * (envMap.size() + 1));
	std::map<std::string, std::string>::iterator it = envMap.begin();
	int i = 0;
	while (it != envMap.end())
	{
		env[i] = strdup((it->first + "=" + it->second).c_str());
		std::cout << env[i] << std::endl;
		++i;
		++it;
	}
	env[i] = NULL;
	return (env);
}

void ResponseMessage::ParseResultCgi(Client &client)
{
    int tmp;
    size_t pos;
	std::string headers;
	std::string key;
	std::string value;

	if (this->bodyResponse.find("\r\n\r\n") == std::string::npos)
		return;
	headers = this->bodyResponse.substr(0, this->bodyResponse.find("\r\n\r\n") + 1);
	pos = headers.find("Status");
	if (pos != std::string::npos)
	{
		this->answerNum.clear();
		pos += 8;
		while (headers[pos] != '\r')
		{
			this->answerNum += headers[pos];
			pos++;
		}
	}
    pos = headers.find("Content-Type");
	if (pos != std::string::npos)
	{
		this->contentType.clear();
		pos += 14;
		while (headers[pos] != '\r')
		{
			this->contentType += headers[pos];
			pos++;
		}
	}
	pos = this->bodyResponse.find("\r\n\r\n") + 4;
	this->bodyResponse = this->bodyResponse.substr(pos);
    this->headerResponse = "HTTP/1.1 " + this->answerNum + "\nContent-Length:" + to_string(this->bodyResponse.size()) + "\r\nContent-Type: " + this->contentType + "\r\n\r\n";
    this->fullResponse = this->headerResponse + this->bodyResponse;
    
    // body.clear();
    headers.clear();
    key.clear();
    value.clear();
}

int ResponseMessage::ExecCgi(Client &client, server server)
{
    char **args = nullptr;
	char **env = nullptr;
	std::string path = "/usr/bin/php";
	int ret;
	int tubes[2];
    int cgiStatus = 0;
    // for (std::vector<location>::const_iterator it = server.locations.begin(); // переходим в папку со скриптом
    // it != server.locations.end(); ++it)
    // {
    //     if (it->locationURI == "/" && it->root != "")
    //         chdir(it->root.c_str());
    // }
    args = (char **) (malloc(sizeof(char *) * 3));
	args[0] = strdup(path.c_str()); // путь "/usr/bin/php"
	args[1] = strdup("post.php"); // полный путь до скрипта
	args[2] = nullptr;
    env = setEnv(client);
    client.tmp_fd = open(TMP_PATH, O_WRONLY | O_CREAT, 0666);
    close(client.read_fd);
    client.read_fd = -1;
	pipe(tubes);
    std::cout << "executing CGI"<< std::endl;
    if ((client.cgi_pid = fork()) == 0)
	{
		close(tubes[1]);
		dup2(tubes[0], 0);
		dup2(client.tmp_fd, 1);
		errno = 0;
		ret = execve(path.c_str(), args, env);
		if (ret == -1)
		{
			std::cerr << "Error with CGI: " << strerror(errno) << std::endl;
			exit(1);
		}
    }
    else
    {   
        waitpid(client.cgi_pid, &cgiStatus ,0);
        close(tubes[0]);
        client.write_fd = tubes[1];
        client.read_fd = open(TMP_PATH, O_RDONLY); 
    }
    freeAll(args, env);
}


void ResponseMessage::ReadFile(Client &client)
{
    char tmpBuffer[BUFFER_SIZE];
	int result;
	int cgiStatus = 0;

    // if (client.cgi_pid != -1)
	// {
	// 	if (waitpid((pid_t) client.cgi_pid, (int *) &cgiStatus, (int) WNOHANG) == 0)
    //         return;
    
	// 	else
	// 	{
	// 		if (WEXITSTATUS(cgiStatus) == 1)
	// 		{
	// 			close(client.tmp_fd);
	// 			client.tmp_fd = -1;
	// 			client.cgi_pid = -1;
	// 			close(client.read_fd);
	// 			unlink(TMP_PATH);
	// 			client.read_fd = -1;
	// 			this->bodyResponse = "Error with cgi\n";
	// 			return;
	// 		}
	// 	}
	// }
    result = read(client.read_fd, tmpBuffer, BUFFER_SIZE);
	if (result >= 0)
		tmpBuffer[result] = '\0';
	std::string tmp(tmpBuffer, result);
	this->bodyResponse += tmp;
	if (result == 0)
	{
	    close(client.read_fd);
	    client.read_fd = -1;
	}
    unlink(TMP_PATH);
}

bool ResponseMessage::CheckSyntaxRequest(Client &client)
{
    if (client.request.method.size() == 0 || client.request.target.size() == 0)
		return (false);
	if (client.request.method != "GET" && client.request.method != "POST"
		&& client.request.method != "DELETE")
		return (false);
	if (client.request.protocolVersion != "HTTP/1.1\r" && client.request.protocolVersion != "HTTP/1.1")
		return (false);
	if (client.request.headers.find("Host") == client.request.headers.end())
		return (false);
    // if ()
	return (true);
}

std::string ResponseMessage::PrepareResponse(Client &client,std::vector<server> &servers)
{
    std::string response;
    std::ifstream inf;
    std::string serverHost;
    RequestMessage requestMessage = client.request;
    std::string buf;
    this->fullResponse.clear();
    int i = 0;
    if (!CheckSyntaxRequest(client)) // обработка корректного реквеста и выдача 400 ошибки (искать инфу в проекте донор по поиску BAD) добавить проверку тела запроса и content length <= 0
    {
            inf.open("www/400.html");
            if (inf.is_open())
            {
                getline (inf, buf, '\0');
                inf.close();
                this->answerNum = "400 Bad Request";
                this->contentType = "text/html";
                this->headerResponse = "HTTP/1.1 " + this->answerNum + "\nContent-Length:" + to_string(buf.size()) + "\r\nContent-Type: " + this->contentType + "\r\n\r\n";
                this->fullResponse = this->headerResponse + buf;
                buf.clear();
                return (getFullResponse());
            }
    }
    else 
        for (std::vector<server>::const_iterator it = servers.begin(); it != servers.end(), i <= servers.size(); ++it, ++i)
        {
            serverHost = servers[i].host + ":" + std::to_string(servers[i].port);
            if (requestMessage.headers.find("Host")->second == serverHost)
            {
                if (client.request.target.find("post.php") != std::string::npos)
                {
                    ExecCgi(client, servers[i]);
                    ReadFile(client);
                    ParseResultCgi(client);
                }
                else if (requestMessage.method == "GET")
                {
                    // if (requestMessage.target == "/autoindex"){
                    //     autoindexGet(requestMessage, servers[i]);
                    //     return (getResponse());
                    // }

                    PrepareGet(requestMessage, servers[i]);
                }
                else if (requestMessage.method == "POST")
                {
                    // если не content type в запросе выдать ошибку
                    // проверяем длину контента, если 0 то выдаем 411 ошибку
                    // проверяем длину контента если она больше заданной в конфиге  и данные не чанкед выкидываем 413 ошибку
                    PreparePost(requestMessage, servers[i]);
                }
                if (this->answerNum == "404")
                {
                    for (std::vector<location>::const_iterator it = servers[i].locations.begin(); // переходим в папку со скриптом
                    it != servers[i].locations.end(); ++it)
                    {
                        if (it->locationURI == "/" && it->root != "")
                            chdir(it->root.c_str());
                    }
                    inf.open("404.html");
                    if (inf.is_open())
                    {
                        getline (inf, buf, '\0');
                        inf.close();
                        this->answerNum = "404 Not Found";
                        this->contentType = "text/html";
                        this->headerResponse = "HTTP/1.1 " + this->answerNum + "\nContent-Length:" + to_string(buf.size()) + "\r\nContent-Type: " + this->contentType + "\r\n\r\n";
                        this->fullResponse = this->headerResponse + buf;
                        buf.clear();
                        return (getFullResponse());
                    }
                }
               return (getFullResponse());
            }
        }
}
void* thread_for_dowland(void* response)
{

}

void ResponseMessage::PreparePost(RequestMessage &requestMessage, server &servers)
{
    pthread_t tid;
    pthread_create(&tid, NULL,&thread_for_dowland, &requestMessage);
    pthread_detach(tid);
}

void ResponseMessage::generateAutoindex(std::string itl, char *buffer){
    DIR *dir;
    struct dirent *current;
    dir = opendir(buffer);
	this->bodyResponse = "<html>\n<body>\n";
	this->bodyResponse += "<h1>Autoindex: on</h1>\n";
	while ((current = readdir(dir)) != nullptr)
    {
        if (current->d_name[0] != '.')
		{
			this->bodyResponse += "<a href=\""+ itl;
			    if (itl != "/")
                    this->bodyResponse += "/";
			this->bodyResponse += current->d_name;
			this->bodyResponse += "\">";
			this->bodyResponse += current->d_name;
			this->bodyResponse += "</a><br>\n";
		}
	}
    closedir(dir);
    this->answerNum = "200";
    this->contentType = "text/html";
	this->bodyResponse += "</body>\n</html>\n";
    this->headerResponse = "HTTP/1.1 " + this->answerNum + " OK\nContent-Length:" + to_string(bodyResponse.size()) + "\r\nContent-Type: " + this->contentType + "\r\n\r\n";
    this->fullResponse = this->headerResponse + this->bodyResponse;
}

void ResponseMessage::PrepareGet(RequestMessage &requestMessage, server &servers)
{
    std::string buf;
    std::ifstream inf;
    std::string path;
    std::string temp;
      
    for (std::vector<location>::const_iterator it = servers.locations.begin(); 
    it != servers.locations.end(); ++it)
    {
        char buf1[500];
        std::string file;
        file = requestMessage.target.substr(requestMessage.target.find_last_of('/') + 1, requestMessage.target.find('?'));
        temp = requestMessage.target;
        strcpy(buf1,requestMessage.target.c_str());
        if (strstr(buf1, "/autoindex") != nullptr){ // Aвтоиндекс
            char *path = "www";
            generateAutoindex(it->locationURI, path);
        }
        else if (strstr(buf1, "/auth/") != nullptr){
            std::string buf2 = "www/";
            buf2 += requestMessage.target.erase(0,5);
            char buf3[255];
            strcpy(buf3,buf2.c_str());
            generateAutoindex(it->locationURI, buf3);
        }
        else if (requestMessage.target == it->locationURI) // через GET отдаем страницы
        {   
            if (it->root.c_str() != nullptr)
                chdir(it->root.c_str());
            inf.open(it->index);
            // if (inf.is_open())
            // {
            getline (inf, buf, '\0');
            inf.close();
            this->answerNum = "200";
            this->contentType = "text/html";
            this->headerResponse = "HTTP/1.1 " + this->answerNum + " OK\nContent-Length:" + to_string(buf.size()) + "\r\nContent-Type: " + this->contentType + "\r\n\r\n";
            this->fullResponse = this->headerResponse + buf;
            buf.clear();
            return;
        }
        else if (requestMessage.target == "/stylesheet.css") // файл стилей
        {
            if (it->root.c_str() != nullptr)
                chdir(it->root.c_str());
            inf.open("stylesheet.css");
            getline (inf, buf, '\0');
            inf.close();
            this->answerNum = "200";
            this->contentType = "text/css";
            this->headerResponse = "HTTP/1.1 " + this->answerNum + " OK\nContent-Length:" + to_string(buf.size()) + "\r\nContent-Type: " + this->contentType + "\r\n\r\n";
            this->fullResponse = this->headerResponse + buf;
            return;
        }
        else if (requestMessage.target == "/favicon.ico")
        {
        //     char tmpBuffer[BUFFER_SIZE + 1];
	    //     int result;
        //     int fav_fd;
        //     if (it->root.c_str() != nullptr)
        //         chdir(it->root.c_str());
        //     fav_fd = open("favicon.ico", 0666);
        //     result = read(fav_fd, tmpBuffer, BUFFER_SIZE);
	    //     if (result >= 0)
		//         tmpBuffer[result] = '\0';
	    //     std::string tmp(tmpBuffer, result);
	    //     this->bodyResponse += tmp;
        //     this->contentType = "application/octet-stream";
        //     this->answerNum = "200";
        //     this->headerResponse = "HTTP/1.1 " + this->answerNum + " OK\nContent-Length:" + to_string(this->bodyResponse.size()) + "\r\nContent-Type: " + this->contentType + "\r\n\r\n";
        //     this->fullResponse = this->headerResponse + this->bodyResponse;
            return ;
        }
    }
    this->answerNum = "404";
}

std::string ResponseMessage::getAnswerNum(void)
{
    return (this->answerNum);
}


void    ResponseMessage::setResponse(std::string &resp)
{
    this->fullResponse = resp;
}

 std::string ResponseMessage::getFullResponse()
 {
     return (this->fullResponse);
 }

 std::string ResponseMessage::to_string(int n)
{
    char buf[40];
    sprintf(buf,"%d",n);
    return buf;
}

void freeAll(char **args, char **env)
	{
		free(args[0]);
		free(args[1]);
		free(args);
		int i = 0;
		while (env[i])
		{
			free(env[i]);
			++i;
		}
		free(env);
	}