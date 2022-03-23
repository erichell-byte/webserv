#ifndef RESPONSEMESSAGE_HPP
#define RESPONSEMESSAGE_HPP 
#include "server.hpp"
#include "parser.hpp"
#include "requestMessage.hpp"
#include <iostream>
#include "client.hpp"

#define TMP_PATH	"/tmp/cgi.tmp"
#define BUFFER_SIZE 32767
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
        bool isValid;
        parser parser;
    public:
        std::string PrepareResponse(Client &client,std::vector<server> &servers);
        void        setResponse(std::string &resp);
        std::string getFullResponse();
        void CheckLocation();
        void PrepareGet(RequestMessage &requestMessage, server &servers);
        void PreparePost(RequestMessage &requestMessage, server &servers);
        void ClearAll(Client &client);
        std::string getAnswerNum(void);
        std::string to_string(int n);
        int ExecCgi(Client &client, server server);
        void ReadFile(Client &client);
        void ParseResultCgi(Client &client);
        char **setEnv(Client &client);
        bool CheckSyntaxRequest(Client &client);
        ResponseMessage();
        ~ResponseMessage();
        void generateAutoindex(std::string itl, char *buffer);
        

};
void freeAll(char **args, char **env);
#endif
