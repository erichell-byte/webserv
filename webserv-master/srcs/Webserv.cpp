#include "Webserv.hpp"

extern bool isServerRunning;

Webserv::Webserv() : w_read_set(), w_write_set(), m_read_set(), m_write_set()
{

}

Webserv::~Webserv()
{

}

void Webserv::exit(int sig)
{
	(void) sig;

	std::cout << "\n" << "завершаю работу...\n";
	isServerRunning = false;
}

void Webserv::init()
{
	signal(SIGINT, exit);
	FD_ZERO(&m_read_set);
	FD_ZERO(&m_write_set);
	FD_ZERO(&w_read_set);
	FD_ZERO(&w_write_set);
	for (std::vector<Server>::iterator it(servers.begin()); it != servers.end(); ++it)
		it->init(&m_read_set, &m_write_set);

	int i = 0;
	for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); it++)
	{
		std::cout << RED UNDER << "Конфиг для " << i + 1 << "-го cервера:" << END << std::endl;
		for (std::vector<Config>::iterator it1 = it->config.begin(); it1 != it->config.end(); it1++)
		{
			for (std::map<std::string, std::map<std::string, std::string> >::iterator it2 = it1->begin(); it2 != it1->end(); it2++)
			{
				std::string tmp = (*it2).first.substr((*it2).first.find("|") + 1);
				std::cout << BLUE << tmp.substr(0, tmp.length() - 1) << END << std::endl;
				for (std::map<std::string, std::string>::iterator it3 = it2->second.begin(); it3 != it2->second.end(); it3++)
					std::cout << YELLOW << std::setw(12) << (*it3).first << " : " << (*it3).second << END << std::endl;
			}
			i++;
		}
	}
}

void Webserv::run()
{
	isServerRunning = true;
	std::cout << GREEN "Вебсервер успешно запущен!" << END << std::endl;
	FD_SET(0, &m_read_set);
	std::cout << "Введите \"help\" для отображения списка команд!" << std::endl;
	while (isServerRunning)
	{
		w_read_set = m_read_set;
		w_write_set = m_write_set;
		select(getMaxFd(servers) + 1, &w_read_set, &w_write_set, nullptr, nullptr);
		if (FD_ISSET(0, &w_read_set))
			this->handleCLI();
		for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
		{
			if (FD_ISSET(it->getFd(), &w_read_set))
			{
				try
				{
					if (!isServerRunning)
						break;
					if (getOpenFd(servers) > maxFD)
						it->refuseConnection();
					else
						it->acceptConnection();
				}
				catch (std::exception &e)
				{
					std::cerr << "Error: " << e.what() << std::endl;
				}
			}
			if (!it->tmpClients.empty())
				if (FD_ISSET(it->tmpClients.front(), &w_write_set))
					it->sendRefuseConnection(it->tmpClients.front());
			for (std::vector<Client *>::iterator client = it->clients.begin(); client != it->clients.end(); ++client)
			{
				if (FD_ISSET((*client)->clientSock, &w_read_set))
					if (!it->readRequest(client))
						break;
				if (FD_ISSET((*client)->clientSock, &w_write_set))
					if (!it->writeResponse(client))
						break;
				if ((*client)->write_fd != -1)
					if (FD_ISSET((*client)->write_fd, &w_write_set))
						(*client)->writeFile();
				if ((*client)->read_fd != -1)
					if (FD_ISSET((*client)->read_fd, &w_read_set))
						(*client)->readFile();
			}
		}
	}
	servers.clear();
}

void Webserv::stopServer() {
	FD_ZERO(&m_read_set);
	FD_ZERO(&m_write_set);
	FD_ZERO(&w_read_set);
	FD_ZERO(&w_write_set);
	std::cout << GREEN "Сервер успешко остановлен!\n" << END;
}

void Webserv::handleCLI() {
	std::string input;
	std::getline(std::cin, input);
	if (input == "exit") {
		std::cout << "Выключаю сервер..." << std::endl;
		stopServer();
		exit(0);
	}
	else if (input == "restart") {
		std::cout << "Перезагружаю сервер..." << std::endl;
		run();
	}
	else if (input == "help") {
		std::cout << "Список доступных команд:\n"
					 "\n"
					 "   help              помощь\n"
					 "   exit              завершить работу сервера\n"
					 "   restart           перезагрузить сервер\n" << std::endl;
	}
	else {
		std::cout << "Команда \"" << input << "\" не найдена. Введите \"help\" для отображения списка команд!" << std::endl;
	}
}

std::string Webserv::readFile(char *file)
{
	int fd;
	int ret;
	char buf[4096];
	std::string parsed;

	fd = open(file, O_RDONLY);
	while ((ret = read(fd, buf, 4095)) > 0)
	{
		buf[ret] = '\0';
		parsed += buf;
	}
	close(fd);
	return (parsed);
}

void Webserv::parse(char *file, std::vector<Server> &servs)
{
	size_t d;
	size_t nb_line;
	std::string context;
	std::string buffer;
	std::string line;
	Server server;
	Config tmp;

	buffer = readFile(file);
	nb_line = 0;
	if (buffer.empty())
		throw (WebservException(nb_line));
	while (!buffer.empty())
	{
		ft::get_next_line(buffer, line);
		nb_line++;

		while (ft::isSpace(line[0]))
			line.erase(line.begin());
		if (!line.compare(0, 6, "server"))
		{
			while (ft::isSpace(line[6]))
				line.erase(6, 1);
			if (line[6] != '{')
				throw (WebservException(nb_line));
			if (!line.compare(0, 7, "server{"))
			{
				d = 7;
				while (ft::isSpace(line[d]))
					line.erase(7, 1);
				if (line[d])
					throw (WebservException(nb_line));
				getContent(buffer, context, line, nb_line, tmp); //may throw exception
				std::vector<Server>::iterator it = servs.begin();
				while (it != servs.end())
				{
					if (tmp["server|"]["listen"] == it->config.back()["server|"]["listen"])
					{
						if (tmp["server|"]["server_name"] == it->config.back()["server|"]["server_name"])
							throw (WebservException(nb_line));
						else
							it->config.push_back(tmp);
						break;
					}
					++it;
				}
				if (it == servs.end())
				{
					server.config.push_back(tmp);
					servs.push_back(server);
				}
				server.config.clear();
				tmp.clear();
				context.clear();
			} else
				throw (WebservException(nb_line));
		} else if (line[0])
			throw (WebservException(nb_line));
	}
}

int Webserv::getMaxFd(std::vector<Server> &servs)
{
	int max = 0;
	int fd;

	for (std::vector<Server>::iterator it(servs.begin()); it != servs.end(); ++it)
	{
		fd = it->getMaxFd();
		if (fd > max)
			max = fd;
	}
	return (max);
}

int Webserv::getOpenFd(std::vector<Server> &servs)
{
	int nb = 0;

	for (std::vector<Server>::iterator it(servs.begin()); it != servs.end(); ++it)
	{
		nb += 1;
		nb += it->getOpenFd();
	}
	return (nb);
}

void Webserv::getContent(std::string &buffer, std::string &context, std::string prec, size_t &nb_line, Config &config)
{
	std::string line;
	std::string key;
	std::string value;
	size_t pos;
	size_t tmp;

	prec.pop_back();
	while (prec.back() == ' ' || prec.back() == '\t')
		prec.pop_back();
	context += prec + "|";
	while (ft::isSpace(line[0]))
		line.erase(line.begin());
	while (line != "}" && !buffer.empty())
	{
		ft::get_next_line(buffer, line);
		nb_line++;
		while (ft::isSpace(line[0]))
			line.erase(line.begin());
		if (line[0] != '}')
		{
			pos = 0;
			while (line[pos] && line[pos] != ';' && line[pos] != '{')
			{
				while (line[pos] && !ft::isSpace(line[pos]))
					key += line[pos++];
				while (ft::isSpace(line[pos]))
					pos++;
				while (line[pos] && line[pos] != ';' && line[pos] != '{')
					value += line[pos++];
			}
			tmp = 0;
			if (line[pos] != ';' && line[pos] != '{')
				throw (WebservException(nb_line));
			else
				tmp++;
			while (ft::isSpace(line[pos + tmp]))
				tmp++;
			if (line[pos + tmp])
				throw (WebservException(nb_line));
			else if (line[pos] == '{')
				getContent(buffer, context, line, nb_line, config);
			else
			{
				std::pair<std::string, std::string> temp(key, value);
				config[context].insert(temp);
				key.clear();
				value.clear();
			}

		} else if (line[0] == '}' && !buffer.empty())
		{
			pos = 0;
			while (ft::isSpace(line[++pos]))
				line.erase(line.begin());
			if (line[pos])
				throw (WebservException(nb_line));
			context.pop_back();
			context = context.substr(0, context.find_last_of('|') + 1);
		}
	}
	if (line[0] != '}')
		throw (WebservException(nb_line));
}

