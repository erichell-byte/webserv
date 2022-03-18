#include "server.hpp"
#include <iostream>

server::server(void)
{
	port = 0;
	host = "";
	client_max_body_size = 1000;
	std::vector<std::string>	server_names;
	std::vector<errorPage> 		errorPages;
	std::vector<location> 		locations;
}

void server::debugPrint(void)
{
	std::cout << "port = " << port << " host = " << host << \
	" client_max_body_size = " << client_max_body_size << std::endl;

	int i = 0;
	for (std::vector<std::string>::const_iterator it = server_names.begin(); \
	it != server_names.end(); ++it, ++i)		
		std::cout << " server_names[" << i << "] = " << server_names[i];
	std::cout << std::endl;

	i = 0;
	for (std::vector<errorPage>::const_iterator it = errorPages.begin(); \
	it != errorPages.end(); ++it, ++i)
	{
		int n = 0;
		for (std::vector<int>::const_iterator it = errorPages[i].errorCode.begin(); \
	it != errorPages[i].errorCode.end(); ++it, ++n)
		std::cout << " errorPage[" << i << "].errorCode[" << n << "] = "\
		<< errorPages[i].errorCode[n];

		std::cout << " errorPage[" << i << "] = " << errorPages[i].errorPage;
		std::cout << std::endl;
	}
	
	i = 0;
	for (std::vector<location>::const_iterator it = locations.begin(); \
	it != locations.end(); ++it, ++i)
	{
		std::cout << " locations[" << i << "].locationURI = " << locations[i].locationURI \
		<< " locations[" << i << "].proxy_pass = "  << locations[i].proxy_pass  \
		<< " locations[" << i << "].root = " << locations[i].root
		<< " locations[" << i << "].autoindex = " << locations[i].autoindex 
		<< " locations[" << i << "].index = " << locations[i].index
		<< " locations[" << i << "].fastcgi_path = " << locations[i].fastcgi_path 
		<< " locations[" << i << "].fastcgi_fileRequested = " << locations[i].fastcgi_fileRequested
		<< " locations[" << i << "].upload_store = " << locations[i].upload_store << std::endl;
	}
	std::cout << std::endl;
}
