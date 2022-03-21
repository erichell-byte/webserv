#ifndef RESPONSEMESSAGE_HPP
#define RESPONSEMESSAGE_HPP 
#include "server.hpp"
#include "parser.hpp"
#include "requestMessage.hpp"
#include <iostream>
#include "client.hpp"

#define TMP_PATH	"/tmp/cgi.tmp"
enum 
{
    GET,
    POST,
    DELETE
};

class Client;

class ResponseMessage
{
    private:
        std::string bodyResponse;
        std::string headerResponse;
        std::string fullResponse;
        std::string answerNum;
        std::string contentType;
        parser parser;
    public:
        std::string PrepareResponse(Client &client,std::vector<server> &servers);
        void        setResponse(std::string &resp);
        std::string getResponse();
        void CheckLocation();
        void PrepareGet(RequestMessage &requestMessage, server &servers);
        void PreparePost(RequestMessage &requestMessage, server &servers);
        void ClearAll(Client &client);
        std::string getAnswerNum(void);
        std::string to_string(int n);
        int ExecCgi(Client &client);
        void ReadFile(Client &client);
        void ParseResultCgi(Client &client);
        void setErrorPage(int ErrorNum);
        char **setEnv(Client &client);
        ResponseMessage();
        ~ResponseMessage();
        void generateAutoindex(std::string itl, char *buffer);
        

};
void freeAll(char **args, char **env);
#endif
