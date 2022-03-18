#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "Server.hpp"

class Webserv
{
public:
	fd_set w_read_set;
	fd_set w_write_set;
	fd_set m_read_set; //дескриптор готовый к считыванию
	fd_set m_write_set;
	
	std::vector<Server> servers;


	static std::string readFile(char *file);

	void getContent(std::string &buffer, std::string &context, std::string prec, size_t &nb_line, Config &config);

	Webserv();

	virtual ~Webserv();

	static void exit(int sig);

	void parse(char *file, std::vector<Server> &servs);

	void init();

	void run();

	void stopServer();

	static int getMaxFd(std::vector<Server> &servs);

	static int getOpenFd(std::vector<Server> &servs);

	void handleCLI();
};

#endif
