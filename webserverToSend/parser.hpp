#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>
#include "server.hpp"

class parser
{
	std::string line;
	std::string key;
	std::string value;
	std::vector<server> servers;
	server currentServer;
	location currentLocation;
	
	enum ServerType
	{
		host,
		port,
		server_names,
		client_max_body_size,
		error_page,
		locationT,
		closeServer,
		invalidServerType
	};

	ServerType StringToServerType(std::string key)
	{
		if (key == "host") return (host);
		if (key == "port") return (port);
		if (key == "server_names") return (server_names);
		if (key == "client_max_body_size") return (client_max_body_size);
		if (key =="error_page") return (error_page);
		if (key == "location")  return (locationT);
		if (key == "}")  return (closeServer);
		return (invalidServerType);
	};
	
	enum LocationType
	{
		uri,
		root,
		proxy_pass,
		limit_except,
		autoindex,
		index,
		fastcgi_pass,
		upload_store,
		closeLocation,
		auth,
		invalidLocationType
	};

	LocationType StringToLocationType(std::string key)
	{
		if (key == "URI") return (uri);
		if (key == "root") return (root);
		if (key == "proxy_pass") return (proxy_pass);
		if (key =="limit_except") return (limit_except);
		if (key == "autoindex")  return (autoindex);
		if (key == "index")  return (index);
		if (key == "fastcgi_pass")  return (fastcgi_pass);
		if (key == "upload_store")  return (upload_store);
		if (key == "}")  return (closeLocation);
		if (key == "auth")	return (auth);
		return (invalidLocationType);
	};
	

public:
	std::vector<server> fileParser(const char* file_name);
	void serverParser(std::ifstream& file);
	void locationParser(std::ifstream& file);
	void errorPageParser(std::string errorPageValue);
};
