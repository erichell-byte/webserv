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

ResponseMessage::~ResponseMessage(){}

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
    envMap["PATH_TRANSLATED"] = "/Users/erichell/Downloads/webserverToSend/www/post.php";
    envMap["CONTENT_LENGTH"] = std::to_string(0);
    if (client.request.target.find('?') != std::string::npos)
		envMap["QUERY_STRING"] = client.request.target.substr(client.request.target.find('?') + 1);
	else
		envMap["QUERY_STRING"];
    if (client.request.headers.find("Content-Type") != client.request.headers.end())
		envMap["CONTENT_TYPE"] = client.request.headers["Content-Type"];
    envMap["SCRIPT_NAME"] = "/Users/erichell/Downloads/webserverToSend/www/post.php";
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


int ResponseMessage::ExecCgi(Client &client)
{
    char **args = nullptr;
	char **env = nullptr;
	std::string path = "/usr/bin/php";
	int ret;
	int tubes[2];
    // chdir("/Users/erichell/Downloads/webserverToSend/www");
    args = (char **) (malloc(sizeof(char *) * 3));
	args[0] = strdup(path.c_str()); // путь "/usr/bin/php"
	args[1] = strdup("/Users/erichell/Downloads/webserverToSend/www/post.php"); // полный путь до скрипта
	args[2] = nullptr;
    env = setEnv(client);
    client.tmp_fd = open(TMP_PATH, O_WRONLY | O_CREAT, 0666);
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
        // waitpid(client.cgi_pid, NULL, 0);
        close(tubes[0]);
        client.write_fd = tubes[1];
        client.read_fd = open(TMP_PATH, O_RDONLY);

    }
    freeAll(args, env);
}

std::string ResponseMessage::PrepareResponse(Client &client,std::vector<server> &servers)
{
    std::string response;
    std::ifstream inf;
    std::string serverHost;
    RequestMessage requestMessage = client.request;
    int i = 0;
    for (std::vector<server>::const_iterator it = servers.begin(); it != servers.end(), i <= servers.size(); ++it, ++i)
    {
        serverHost = servers[i].host + ":" + std::to_string(servers[i].port);
        if (requestMessage.headers.find("Host")->second == serverHost)
        {
            // client.request.headers["Root"] = servers
            if (client.request.target.find("post.php") != std::string::npos)
            {
                ExecCgi(client);
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
                // добавить проверку разрешен ли метод
                // если не content type в запросе выдать ошибку
                // проверяем длину контента, если 0 то выдаем 411 ошибку
                // проверяем длину контента если она больше заданной в конфиге  и данные не чанкед выкидываем 413 ошибку
                PreparePost(requestMessage, servers[i]);
            }
            return (getResponse());
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
    setResponse(this->fullResponse);
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
        char buf1[255];
        std::string file;
        file = requestMessage.target.substr(requestMessage.target.find_last_of('/') + 1, requestMessage.target.find('?'));
        temp = requestMessage.target;
        strcpy(buf1,requestMessage.target.c_str());
        if (strstr(buf1, "/autoindex") != nullptr){
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
        else if (requestMessage.target == it->locationURI)
        {   
            if (it->root.c_str() != nullptr)
                chdir(it->root.c_str());
            inf.open(it->index);
            getline (inf, buf, '\0');
            inf.close();
            this->answerNum = "200";
            this->contentType = "text/html";
            this->headerResponse = "HTTP/1.1 " + this->answerNum + " OK\nContent-Length:" + to_string(buf.size()) + "\r\nContent-Type: " + this->contentType + "\r\n\r\n";
            this->fullResponse = this->headerResponse + buf;
            buf.clear();
            return(setResponse(this->fullResponse));
        }
        // else if ()
        // {

        // }
        else if (requestMessage.target == "/stylesheet.css")
        {
            if (it->root.c_str() != nullptr)
                chdir(it->root.c_str());
            inf.open("stylesheet.css");
            getline (inf, buf, '\0');
            inf.close();
            this->answerNum = "200";
            this->contentType = "text/css";
            this->headerResponse = "HTTP/1.1 " + this->answerNum + " OK\nContent-Length:" + to_string(buf.size()) + "\r\nContent-Type: " + this->contentType + "\r\n\r\n";
            buf = this->headerResponse + buf;
            return(setResponse(buf));
        }
        // else if (requestMessage.target == "/favicon.ico")
        // {
             // inf.open("/Users/erichell/WEBSERVER/webserv/www/favicon.ico");

        // }
        // else
        // {
        //     inf.open("/Users/erichell/WEBSERVER/webserv/www/custom_404.html");
        //     getline (inf, buf, '\0');
        //     inf.close();
        //     this->contentType = "text/html";
        //     this->answerNum = "404";
        //     this->headerResponse = "HTTP/1.1 " + this->answerNum + " OK\nContent-Length:" + to_string(buf.size()) + "\r\nContent-Type: " + this->contentType + "\r\n\r\n";
        //     buf = this->headerResponse + buf;
        //     setResponse(buf);

        // }
    }
}

std::string ResponseMessage::getAnswerNum(void)
{
    return (this->answerNum);
}


void    ResponseMessage::setResponse(std::string &resp)
{
    this->fullResponse = resp;
}

 std::string ResponseMessage::getResponse()
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