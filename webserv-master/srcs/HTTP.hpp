#ifndef HTTP_HPP
#define HTTP_HPP

#include "Client.hpp"

class Client;

class HTTP
{
	public:

	std::map<std::string, std::string> MIMETypes;

	HTTP();

	virtual ~HTTP();

	void parseRequest(Client &client, std::vector<Config> &config);

	void parseBody(Client &client);

	void dispatcher(Client &client);

	static void createResponse(Client &client);

	void handleGet(Client &client);

	void handleHead(Client &client);

	void handlePost(Client &client);

	void handlePut(Client &client);

	void handleConnect(Client &client);

	void handleTrace(Client &client);

	void handleOptions(Client &client);

	void handleDelete(Client &client);

	void handleBadRequest(Client &client);

	static void getConf(Client &client, Request &request, std::vector<Config> &config);

	void negotiate(Client &client);

	static void doAutoindex(Client &client);

	static bool checkSyntax(const Request &request);

	static bool parseHeaders(std::string &buf, Request &req);

	static void getBody(Client &client);

	void dechunkBody(Client &client);

	void execCGI(Client &client);

	static void parseCGIResult(Client &client);

	static std::string getLastModified(const std::string &path);

	std::string findType(Client &client);

	static void getErrorPage(Client &client);

	static int findLength(Client &client);

	static void fillBody(Client &client);

	static int fromHexa(const char *nb);

	static char **setEnv(Client &client);

	static std::string decode64(const char *data);

	static void parseAcceptLanguage(Client &client, std::multimap<std::string, std::string> &map);

	static void parseAcceptCharset(Client &client, std::multimap<std::string, std::string> &map);

	void assignMIME();

	int getStatusCode(Client &client);

	int GETStatus(Client &client);

	int POSTStatus(Client &client);

	int PUTStatus(Client &client);

	int CONNECTStatus(Client &client);

	int TRACEStatus(Client &client);

	int OPTIONSStatus(Client &client);

	int DELETEStatus(Client &client);
};

#endif
