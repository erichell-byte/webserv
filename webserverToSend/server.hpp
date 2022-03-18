#pragma once
#include <string>
#include <vector>

struct errorPage
{
	std::vector<int> 	errorCode;
	std::string 		errorPage;
};

struct location
{
	std::string 				locationURI;
	std::vector<std::string> 	limit_except;
	std::string					proxy_pass;
	std::string 				root;
	int 						autoindex;
	std::string 				index;
	std::string 				fastcgi_path;
	std::string					fastcgi_fileRequested;
	std::string					upload_store;
	std::string					auth;
	location() : autoindex(0) { };
};

class server {
public:
	int 						port;
	std::string 				host;
	std::vector<std::string>	server_names;
	size_t 						client_max_body_size;
	std::vector<errorPage> 		errorPages;
	std::vector<location> 		locations;

	server(void);
	void debugPrint(void);
};
