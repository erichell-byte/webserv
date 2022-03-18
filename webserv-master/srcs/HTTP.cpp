#include "HTTP.hpp"

HTTP::HTTP()
{
	assignMIME();
}

HTTP::~HTTP()
{

}

void HTTP::parseRequest(Client &client, std::vector<Config> &config)
{
	Request request;
	std::string tmp;
	std::string buffer;

	buffer = std::string(client.buffer);
	if (buffer[0] == '\r')
		buffer.erase(buffer.begin());
	if (buffer[0] == '\n')
		buffer.erase(buffer.begin());
	ft::get_next_line(buffer, request.method, ' ');
	ft::get_next_line(buffer, request.uri, ' ');
	ft::get_next_line(buffer, request.version);
	if (parseHeaders(buffer, request))
		request.isValid = checkSyntax(request);
	if (request.uri != "*" || request.method != "OPTIONS")
		getConf(client, request, config);
	if (request.isValid)
	{
		if (client.clientConfig["root"][0] != '\0')
			chdir(client.clientConfig["root"].c_str());
		if (request.method == "POST" || request.method == "PUT")
			client.status = BODYPARSING;
		else
			client.status = CODE;
	} else
	{
		request.method = "BAD";
		client.status = CODE;
	}
	client.request = request;
	tmp = client.buffer;
	tmp = tmp.substr(tmp.find("\r\n\r\n") + 4);
	strcpy(client.buffer, tmp.c_str());
}

bool HTTP::parseHeaders(std::string &buf, Request &req)
{
	size_t pos;
	std::string line;
	std::string key;
	std::string value;

	while (!buf.empty())
	{
		ft::get_next_line(buf, line);
		if (line.empty() || line[0] == '\n' || line[0] == '\r')
			break;
		if (line.find(':') != std::string::npos)
		{
			pos = line.find(':');
			key = line.substr(0, pos);
			if (line[pos + 1] == ' ')
				value = line.substr(pos + 2);
			else
				value = line.substr(pos + 1);
			if (ft::isSpace(value[0]) || ft::isSpace(key[0]) || value.empty() || key.empty())
			{
				req.isValid = false;
				return (false);
			}
			req.headers[key] = value;
			req.headers[key].pop_back(); //remove '\r'
		} else
		{
			req.isValid = false;
			return (false);
		}
	}
	return (true);
}

void HTTP::parseBody(Client &client)
{
	if (client.request.headers.find("Content-Length") != client.request.headers.end())
		getBody(client);
	else if (client.request.headers["Transfer-Encoding"] == "chunked")
		dechunkBody(client);
	else
	{
		client.request.method = "BAD";
		client.status = CODE;
	}
	if (client.status == CODE)
		utils::showMessage(
				"body size parsed from " + client.ip + ":" + std::to_string(client.port) + ": " + std::to_string(client.request.body.size()), BLUE);
}

void HTTP::getBody(Client &client)
{
	unsigned int bytes;

	if (client.chunk.length == 0)
		client.chunk.length = atoi(client.request.headers["Content-Length"].c_str());
	if (client.chunk.length < 0)
	{
		client.request.method = "BAD";
		client.status = CODE;
		return;
	}
	bytes = strlen(client.buffer);
	if (bytes >= client.chunk.length)
	{
		memset(client.buffer + client.chunk.length, 0, BUFFER_SIZE - client.chunk.length);
		client.request.body += client.buffer;
		client.chunk.length = 0;
		client.status = CODE;
	} else
	{
		client.chunk.length -= bytes;
		client.request.body += client.buffer;
		memset(client.buffer, 0, BUFFER_SIZE + 1);
	}
}

void HTTP::dechunkBody(Client &client)
{
	if (strstr(client.buffer, "\r\n") && !client.chunk.isFound)
	{
		client.chunk.length = findLength(client);
		if (client.chunk.length == 0)
			client.chunk.isDone = true;
		else
			client.chunk.isFound = true;
	} else if (client.chunk.isFound)
		fillBody(client);
	if (client.chunk.isDone)
	{
		memset(client.buffer, 0, BUFFER_SIZE + 1);
		client.status = CODE;
		client.chunk.isFound = false;
		client.chunk.isDone = false;
		return;
	}
}

void HTTP::getConf(Client &client, Request &request, std::vector<Config> &config)
{
	std::map<std::string, std::string> elem;
	std::string temp;
	std::string file;
	struct stat info = {};
	Config current;

	if (!request.isValid)
	{
		client.clientConfig["error"] = config[0]["server|"]["error"];
		return;
	}
	std::vector<Config>::iterator it(config.begin());
	while (it != config.end())
	{
		if (request.headers["Host"] == (*it)["server|"]["server_name"])
		{
			current = *it;
			break;
		}
		++it;
	}
	if (it == config.end())
		current = config[0];
	file = request.uri.substr(request.uri.find_last_of('/') + 1, request.uri.find('?'));
	temp = request.uri;
	do
	{
		if (current.find("server|location " + temp + "|") != current.end())
		{
			elem = current["server|location " + temp + "|"];
			break;
		}
		temp = temp.substr(0, temp.find_last_of('/'));
	} while (temp != "");
	if (elem.empty())
		if (current.find("server|location /|") != current.end())
			elem = current["server|location /|"];
	if (!elem.empty())
	{
		client.clientConfig = elem;
		client.clientConfig["path"] = request.uri.substr(0, request.uri.find('?'));
		if (elem.find("root") != elem.end())
			client.clientConfig["path"].replace(0, temp.size(), elem["root"]);
	}
	for (std::map<std::string, std::string>::iterator it = current["server|"].begin(); it != current["server|"].end(); ++it)
	{
		if (client.clientConfig.find(it->first) == client.clientConfig.end())
			client.clientConfig[it->first] = it->second;
	}
	lstat(client.clientConfig["path"].c_str(), &info);
	if (S_ISDIR(info.st_mode))
		if (client.clientConfig["index"][0] && client.clientConfig["autoindex"] != "on")
			client.clientConfig["path"] += "/" + elem["index"];
	if (request.method == "GET")
		client.clientConfig["savedpath"] = client.clientConfig["path"];
	utils::showMessage("path requested from " + client.ip + ":" + std::to_string(client.port) + ": " + client.clientConfig["path"], WHITE);
}

void HTTP::negotiate(Client &client)
{
	std::multimap<std::string, std::string> languageMap;
	std::multimap<std::string, std::string> charsetMap;
	int fd = -1;
	std::string path;
	std::string ext;

	if (client.request.headers.find("Accept-Language") != client.request.headers.end())
		parseAcceptLanguage(client, languageMap);
	if (client.request.headers.find("Accept-Charset") != client.request.headers.end())
		parseAcceptCharset(client, charsetMap);
	if (!languageMap.empty())
	{
		for (std::multimap<std::string, std::string>::reverse_iterator it(languageMap.rbegin()); it != languageMap.rend(); ++it)
		{
			if (!charsetMap.empty())
			{
				for (std::multimap<std::string, std::string>::reverse_iterator it2(charsetMap.rbegin()); it2 != charsetMap.rend(); ++it2)
				{
					ext = it->second + "." + it2->second;
					path = client.clientConfig["savedpath"] + "." + ext;
					fd = open(path.c_str(), O_RDONLY);
					if (fd != -1)
					{
						client.response.headers["Content-Language"] = it->second;
						break;
					}
					ext = it2->second + "." + it->second;
					path = client.clientConfig["savedpath"] + "." + ext;
					fd = open(path.c_str(), O_RDONLY);
					if (fd != -1)
					{
						client.response.headers["Content-Language"] = it->second;
						break;
					}
				}
			} else
			{
				ext = it->second;
				path = client.clientConfig["savedpath"] + "." + ext;
				fd = open(path.c_str(), O_RDONLY);
				if (fd != -1)
				{
					client.response.headers["Content-Language"] = it->second;
					break;
				}
			}
		}
	} else if (languageMap.empty())
	{
		if (!charsetMap.empty())
		{
			for (std::multimap<std::string, std::string>::reverse_iterator it2(charsetMap.rbegin()); it2 != charsetMap.rend(); ++it2)
			{
				ext = it2->second;
				path = client.clientConfig["savedpath"] + "." + it2->second;
				fd = open(path.c_str(), O_RDONLY);
				if (fd != -1)
					break;
			}
		}
	}
	if (fd != -1)
	{
		client.clientConfig["path"] = path;
		client.response.headers["Content-Location"] = client.request.uri + "." + ext;
		if (client.read_fd != -1)
			close(client.read_fd);
		client.read_fd = fd;
		client.response.statusCode = OK;
	}
}

void HTTP::doAutoindex(Client &client)
{
	DIR *dir;
	struct dirent *current;

	close(client.read_fd);
	client.read_fd = -1;
	dir = opendir(client.clientConfig["path"].c_str());
	client.response.body = "<html>\n<body>\n";
	client.response.body += "<h1>Autoindex: on</h1>\n";
	while ((current = readdir(dir)) != nullptr)
	{
		if (current->d_name[0] != '.')
		{
			client.response.body += "<a href=\"" + client.request.uri;
			if (client.request.uri != "/")
				client.response.body += "/";
			client.response.body += current->d_name;
			client.response.body += "\">";
			client.response.body += current->d_name;
			client.response.body += "</a><br>\n";
		}
	}
	closedir(dir);
	client.response.body += "</body>\n</html>\n";
}

bool HTTP::checkSyntax(const Request &request)
{
	if (request.method.size() == 0 || request.uri.size() == 0
		|| request.version.size() == 0)
		return (false);
	if (request.method != "GET" && request.method != "POST"
		&& request.method != "HEAD" && request.method != "PUT"
		&& request.method != "CONNECT" && request.method != "TRACE"
		&& request.method != "OPTIONS" && request.method != "DELETE")
		return (false);
	if (request.method != "OPTIONS" && request.uri[0] != '/') //OPTIONS can have * as uri
		return (false);
	if (request.version != "HTTP/1.1\r" && request.version != "HTTP/1.1")
		return (false);
	if (request.headers.find("Host") == request.headers.end())
		return (false);
	return (true);
}

void HTTP::execCGI(Client &client)
{
	char **args = nullptr;
	char **env = nullptr;
	std::string path;
	int ret;
	int tubes[2];

	if (client.clientConfig["php"][0] && client.clientConfig["path"].find(".php") != std::string::npos)
		path = client.clientConfig["php"];
	else if (client.clientConfig["exec"][0])
		path = client.clientConfig["exec"];
	else
		path = client.clientConfig["path"];
	close(client.read_fd);
	client.read_fd = -1;
	args = (char **) (malloc(sizeof(char *) * 3));
	args[0] = strdup(path.c_str()); // путь "/usr/bin/php"
	args[1] = strdup(client.clientConfig["path"].c_str()); // полный путь до скрипта
	args[2] = nullptr; // без переменной окружения
	env = setEnv(client);
	client.tmp_fd = open(TMP_PATH, O_WRONLY | O_CREAT, 0666);
	pipe(tubes);
	utils::showMessage("executing CGI for " + client.ip + ":" + std::to_string(client.port), BLUE);
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
	} else
	{
		close(tubes[0]);
		client.write_fd = tubes[1];
		client.read_fd = open(TMP_PATH, O_RDONLY);
		client.setFileToWrite(true);
	}
	ft::freeAll(args, env);
}

void HTTP::parseCGIResult(Client &client)
{
	size_t pos;
	std::string headers;
	std::string key;
	std::string value;

	if (client.response.body.find("\r\n\r\n") == std::string::npos)
		return;
	headers = client.response.body.substr(0, client.response.body.find("\r\n\r\n") + 1);
	pos = headers.find("Status");
	if (pos != std::string::npos)
	{
		client.response.statusCode.clear();
		pos += 8;
		while (headers[pos] != '\r')
		{
			client.response.statusCode += headers[pos];
			pos++;
		}
	}
	pos = 0;
	while (headers[pos])
	{
		while (headers[pos] && headers[pos] != ':')
		{
			key += headers[pos];
			++pos;
		}
		++pos;
		while (headers[pos] && headers[pos] != '\r')
		{
			value += headers[pos];
			++pos;
		}
		client.response.headers[key] = value;
		key.clear();
		value.clear();
		if (headers[pos] == '\r')
			pos++;
		if (headers[pos] == '\n')
			pos++;
	}
	pos = client.response.body.find("\r\n\r\n") + 4;
	client.response.body = client.response.body.substr(pos);
	client.response.headers["Content-Length"] = std::to_string(client.response.body.size());
}

void HTTP::createResponse(Client &client)
{
	std::map<std::string, std::string>::const_iterator b;

	client.textResponse = client.response.version + " " + client.response.statusCode + "\r\n";
	b = client.response.headers.begin();
	while (b != client.response.headers.end())
	{
		if (!b->second.empty())
			client.textResponse += b->first + ": " + b->second + "\r\n";
		++b;
	}
	client.textResponse += "\r\n";
	client.textResponse += client.response.body;
	client.response.clear();
}

void HTTP::dispatcher(Client &client)
{
	typedef void    (HTTP::*ptr)(Client &client);
	std::map<std::string, ptr> map;

	map["GET"] = &HTTP::handleGet;
	map["HEAD"] = &HTTP::handleHead;
	map["PUT"] = &HTTP::handlePut;
	map["POST"] = &HTTP::handlePost;
	map["CONNECT"] = &HTTP::handleConnect;
	map["TRACE"] = &HTTP::handleTrace;
	map["OPTIONS"] = &HTTP::handleOptions;
	map["DELETE"] = &HTTP::handleDelete;
	map["BAD"] = &HTTP::handleBadRequest;

	(this->*map[client.request.method])(client);
}

void HTTP::handleGet(Client &client)
{
	struct stat file_info = {};
	std::string credential;

	switch (client.status)
	{
		case CODE:
			getStatusCode(client);
			fstat(client.read_fd, &file_info);
			if (S_ISDIR(file_info.st_mode) && client.clientConfig["autoindex"] == "on")
				doAutoindex(client);
			if (client.response.statusCode == NOTFOUND)
				negotiate(client);
			if (((client.clientConfig.find("CGI") != client.clientConfig.end() &&
				  client.request.uri.find(client.clientConfig["CGI"]) != std::string::npos)
				 || (client.clientConfig.find("php") != client.clientConfig.end() && client.request.uri.find(".php") != std::string::npos))
				&& client.response.statusCode == OK)
			{
				execCGI(client);
				client.status = CGI;
			} else
				client.status = HEADERS;
			client.setFileToRead(true);
			break;
		case CGI:
			if (client.read_fd == -1)
			{
				parseCGIResult(client);
				client.status = HEADERS;
			}
			break;
		case HEADERS:
			lstat(client.clientConfig["path"].c_str(), &file_info);
			if (!S_ISDIR(file_info.st_mode))
				client.response.headers["Last-Modified"] = getLastModified(client.clientConfig["path"]);
			if (client.response.headers["Content-Type"][0] == '\0')
				client.response.headers["Content-Type"] = findType(client);
			if (client.response.statusCode == UNAUTHORIZED)
				client.response.headers["WWW-Authenticate"] = "Basic";
			else if (client.response.statusCode == NOTALLOWED)
				client.response.headers["Allow"] = client.clientConfig["methods"];
			client.response.headers["Date"] = ft::getDate();
			client.response.headers["Server"] = "webserv";
			client.status = BODY;
			break;
		case BODY:
			if (client.read_fd == -1)
			{
				client.response.headers["Content-Length"] = std::to_string(client.response.body.size());
				createResponse(client);
				client.status = RESPONSE;
			}
			break;
	}
}

void HTTP::handleHead(Client &client)
{
	struct stat file_info = {};

	switch (client.status)
	{
		case CODE:
			getStatusCode(client);
			fstat(client.read_fd, &file_info);
			if (S_ISDIR(file_info.st_mode) && client.clientConfig["autoindex"] == "on")
				doAutoindex(client);
			else if (client.response.statusCode == NOTFOUND)
				negotiate(client);
			fstat(client.read_fd, &file_info);
			if (client.response.statusCode == OK)
			{
				client.response.headers["Last-Modified"] = getLastModified(client.clientConfig["path"]);
				client.response.headers["Content-Type"] = findType(client);
			} else if (client.response.statusCode == UNAUTHORIZED)
				client.response.headers["WWW-Authenticate"] = "Basic";
			else if (client.response.statusCode == NOTALLOWED)
				client.response.headers["Allow"] = client.clientConfig["methods"];
			client.response.headers["Date"] = ft::getDate();
			client.response.headers["Server"] = "webserv";
			client.response.headers["Content-Length"] = std::to_string(file_info.st_size);
			createResponse(client);
			client.status = RESPONSE;
			break;
	}
}

void HTTP::handlePost(Client &client)
{
	switch (client.status)
	{
		case BODYPARSING:
			parseBody(client);
			break;
		case CODE:
			getStatusCode(client);
			if (((client.clientConfig.find("CGI") != client.clientConfig.end() &&
				  client.request.uri.find(client.clientConfig["CGI"]) != std::string::npos)
				 || (client.clientConfig.find("php") != client.clientConfig.end() && client.request.uri.find(".php") != std::string::npos))
				&& client.response.statusCode == OK)
			{
				execCGI(client);
				client.status = CGI;
				client.setFileToRead(true);
			} else
			{
				if (client.response.statusCode == OK || client.response.statusCode == CREATED)
					client.setFileToWrite(true);
				else
					client.setFileToRead(true);
				client.status = HEADERS;
			}
			break;
		case CGI:
			if (client.read_fd == -1)
			{
				parseCGIResult(client);
				client.status = HEADERS;
			}
			break;
		case HEADERS:
			if (client.response.statusCode == UNAUTHORIZED)
				client.response.headers["WWW-Authenticate"] = "Basic";
			else if (client.response.statusCode == NOTALLOWED)
				client.response.headers["Allow"] = client.clientConfig["methods"];
			client.response.headers["Date"] = ft::getDate();
			client.response.headers["Server"] = "webserv";
			if (client.response.statusCode == CREATED)
				client.response.body = "File created\n";
			else if (client.response.statusCode == OK && !((client.clientConfig.find("CGI") != client.clientConfig.end() &&
															client.request.uri.find(client.clientConfig["CGI"]) != std::string::npos)
														   || (client.clientConfig.find("php") != client.clientConfig.end() &&
															   client.request.uri.find(".php") != std::string::npos)))
				client.response.body = "File modified\n";
			client.status = BODY;
			break;
		case BODY:
			if (client.read_fd == -1 && client.write_fd == -1)
			{
				if (client.response.headers["Content-Length"][0] == '\0')
					client.response.headers["Content-Length"] = std::to_string(client.response.body.size());
				createResponse(client);
				client.status = RESPONSE;
			}
			break;
	}
}

void HTTP::handlePut(Client &client)
{
	std::string path;
	std::string body;

	switch (client.status)
	{
		case BODYPARSING:
			parseBody(client);
			break;
		case CODE:
			if (getStatusCode(client))
				client.setFileToWrite(true);
			else
				client.setFileToRead(true);
			client.response.headers["Date"] = ft::getDate();
			client.response.headers["Server"] = "webserv";
			if (client.response.statusCode == CREATED || client.response.statusCode == NOCONTENT)
			{
				client.response.headers["Location"] = client.request.uri;
				if (client.response.statusCode == CREATED)
					client.response.body = "Ressource created\n";
			}
			if (client.response.statusCode == NOTALLOWED)
				client.response.headers["Allow"] = client.clientConfig["methods"];
			else if (client.response.statusCode == UNAUTHORIZED)
				client.response.headers["WWW-Authenticate"] = "Basic";
			client.status = BODY;
			break;
		case BODY:
			if (client.write_fd == -1 && client.read_fd == -1)
			{
				client.response.headers["Content-Length"] = std::to_string(client.response.body.size());
				createResponse(client);
				client.status = RESPONSE;
			}
			break;
	}
}

void HTTP::handleConnect(Client &client)
{
	switch (client.status)
	{
		case CODE:
			getStatusCode(client);
			client.setFileToRead(true);
			client.response.headers["Date"] = ft::getDate();
			client.response.headers["Server"] = "webserv";
			client.status = BODY;
			break;
		case BODY:
			if (client.read_fd == -1)
			{
				client.response.headers["Content-Length"] = std::to_string(client.response.body.size());
				createResponse(client);
				client.status = RESPONSE;
			}
			break;
	}
}

void HTTP::handleTrace(Client &client)
{
	switch (client.status)
	{
		case CODE:
			getStatusCode(client);
			client.response.headers["Date"] = ft::getDate();
			client.response.headers["Server"] = "webserv";
			if (client.response.statusCode == OK)
			{
				client.response.headers["Content-Type"] = "message/http";
				client.response.body = client.request.method + " " + client.request.uri + " " + client.request.version + "\r\n";
				for (std::map<std::string, std::string>::iterator it(client.request.headers.begin());
					 it != client.request.headers.end(); ++it)
				{
					client.response.body += it->first + ": " + it->second + "\r\n";
				}
			} else
				client.setFileToRead(true);
			client.status = BODY;
			break;
		case BODY:
			if (client.read_fd == -1)
			{
				client.response.headers["Content-Length"] = std::to_string(client.response.body.size());
				createResponse(client);
				client.status = RESPONSE;
			}
			break;
	}
}

void HTTP::handleOptions(Client &client)
{
	switch (client.status)
	{
		case CODE:
			getStatusCode(client);
			client.response.headers["Date"] = ft::getDate();
			client.response.headers["Server"] = "webserv";
			if (client.request.uri != "*")
				client.response.headers["Allow"] = client.clientConfig["methods"];
			createResponse(client);
			client.status = RESPONSE;
			break;
	}
}

void HTTP::handleDelete(Client &client)
{
	switch (client.status)
	{
		case CODE:
			std::cout << "here\n";
			if (!getStatusCode(client))
				client.setFileToRead(true);
			client.response.headers["Date"] = ft::getDate();
			client.response.headers["Server"] = "webserv";
			if (client.response.statusCode == OK)
			{
				unlink(client.clientConfig["path"].c_str());
				client.response.body = "File deleted\n";
			} else if (client.response.statusCode == NOTALLOWED)
				client.response.headers["Allow"] = client.clientConfig["methods"];
			else if (client.response.statusCode == UNAUTHORIZED)
				client.response.headers["WWW-Authenticate"] = "Basic";
			client.status = BODY;
			break;
		case BODY:
			if (client.read_fd == -1)
			{
				client.response.headers["Content-Length"] = std::to_string(client.response.body.size());
				createResponse(client);
				client.status = RESPONSE;
			}
			break;
	}
}

void HTTP::handleBadRequest(Client &client)
{
	struct stat file_info = {};

	switch (client.status)
	{
		case CODE:
			client.response.version = "HTTP/1.1";
			client.response.statusCode = BADREQUEST;
			getErrorPage(client);
			fstat(client.read_fd, &file_info);
			client.setFileToRead(true);
			client.response.headers["Date"] = ft::getDate();
			client.response.headers["Server"] = "webserv";
			client.status = BODY;
			break;
		case BODY:
			if (client.read_fd == -1)
			{
				client.response.headers["Content-Length"] = std::to_string(client.response.body.size());
				createResponse(client);
				client.status = RESPONSE;
			}
			break;
	}
}

std::string HTTP::findType(Client &client)
{
	std::string extension;
	size_t pos;

	if (client.clientConfig["path"].find_last_of('.') != std::string::npos)
	{
		pos = client.clientConfig["path"].find('.');
		extension = client.clientConfig["path"].substr(pos, client.clientConfig["path"].find('.', pos + 1) - pos);
		if (MIMETypes.find(extension) != MIMETypes.end())
			return (MIMETypes[extension]);
		else
			return (MIMETypes[".bin"]);
	}
	return ("");
}

void HTTP::getErrorPage(Client &client)
{
	std::string path;

	path = client.clientConfig["error"] + "/" + client.response.statusCode.substr(0, 3) + ".html";
	client.clientConfig["path"] = path;
	client.read_fd = open(path.c_str(), O_RDONLY);
}

std::string HTTP::getLastModified(const std::string &path)
{
	char buf[BUFFER_SIZE + 1];
	int ret;
	struct tm *tm;
	struct stat file_info = {};

	if (lstat(path.c_str(), &file_info) == -1)
		return ("");
	tm = localtime(&file_info.st_mtime);
	ret = strftime(buf, BUFFER_SIZE, "%a, %d %b %Y %T %Z", tm);
	buf[ret] = '\0';
	return (buf);
}

int HTTP::findLength(Client &client)
{
	std::string to_convert;
	int len;
	std::string tmp;

	to_convert = client.buffer;
	to_convert = to_convert.substr(0, to_convert.find("\r\n"));
	while (to_convert[0] == '\n')
		to_convert.erase(to_convert.begin());
	if (to_convert.empty())
		len = 0;
	else
		len = fromHexa(to_convert.c_str());
	len = fromHexa(to_convert.c_str());
	tmp = client.buffer;
	tmp = tmp.substr(tmp.find("\r\n") + 2);
	strcpy(client.buffer, tmp.c_str());
	return (len);
}

void HTTP::fillBody(Client &client)
{
	std::string tmp;

	tmp = client.buffer;
	if (tmp.size() > client.chunk.length)
	{
		client.request.body += tmp.substr(0, client.chunk.length);
		tmp = tmp.substr(client.chunk.length + 1);
		memset(client.buffer, 0, BUFFER_SIZE + 1);
		strcpy(client.buffer, tmp.c_str());
		client.chunk.length = 0;
		client.chunk.isFound = false;
	} else
	{
		client.request.body += tmp;
		client.chunk.length -= tmp.size();
		memset(client.buffer, 0, BUFFER_SIZE + 1);
	}
}

int HTTP::fromHexa(const char *nb)
{
	char base[17] = "0123456789abcdef";
	char base2[17] = "0123456789ABCDEF";
	int result = 0;
	int i;
	int index;

	i = 0;
	while (nb[i])
	{
		int j = 0;
		while (base[j])
		{
			if (nb[i] == base[j])
			{
				index = j;
				break;
			}
			j++;
		}
		if (j == 16)
		{
			j = 0;
			while (base2[j])
			{
				if (nb[i] == base2[j])
				{
					index = j;
					break;
				}
				j++;
			}
		}
		result += index * ft::pow(16, (int) (strlen(nb) - 1) - i);
		i++;
	}
	return (result);
}

std::string HTTP::decode64(const char *data)
{
	while (*data != ' ')
		data++;
	data++;
	unsigned int len = strlen(data);
	unsigned char *p = (unsigned char *) data;
	int pad = len > 0 && (len % 4 || p[len - 1] == '=');
	const size_t L = ((len + 3) / 4 - pad) * 4;
	std::string str(L / 4 * 3 + pad, '\0');

	for (size_t i = 0, j = 0; i < L; i += 4)
	{
		int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
		str[j++] = n >> 16;
		str[j++] = n >> 8 & 0xFF;
		str[j++] = n & 0xFF;
	}
	if (pad)
	{
		int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
		str[str.size() - 1] = n >> 16;

		if (len > L + 2 && p[L + 2] != '=')
		{
			n |= B64index[p[L + 2]] << 6;
			str.push_back(n >> 8 & 0xFF);
		}
	}
	if (str.back() == 0)
		str.pop_back();
	return (str);
}

void HTTP::parseAcceptLanguage(Client &client, std::multimap<std::string, std::string> &map)
{
	std::string language;
	std::string to_parse;
	std::string q;

	to_parse = client.request.headers["Accept-Language"];
	int i = 0;
	while (to_parse[i] != '\0')
	{
		language.clear();
		q.clear();
		while (to_parse[i] && to_parse[i] != ',' && to_parse[i] != ';')
		{
			language += to_parse[i];
			++i;
		}
		if (to_parse[i] == ',' || to_parse[i] == '\0')
			q = "1";
		else if (to_parse[i] == ';')
		{
			i += 3;
			while (to_parse[i] && to_parse[i] != ',')
			{
				q += to_parse[i];
				++i;
			}
		}
		if (to_parse[i])
			++i;
		std::pair<std::string, std::string> pair(q, language);
		map.insert(pair);
	}
}

void HTTP::parseAcceptCharset(Client &client, std::multimap<std::string, std::string> &map)
{
	std::string charset;
	std::string to_parse;
	std::string q;

	to_parse = client.request.headers["Accept-Charset"];
	int i = 0;
	while (to_parse[i] != '\0')
	{
		charset.clear();
		q.clear();
		while (to_parse[i] && to_parse[i] != ',' && to_parse[i] != ';')
		{
			charset += to_parse[i];
			++i;
		}
		if (to_parse[i] == ',' || to_parse[i] == '\0')
			q = "1";
		else if (to_parse[i] == ';')
		{
			i += 3;
			while (to_parse[i] && to_parse[i] != ',')
			{
				q += to_parse[i];
				++i;
			}
		}
		if (to_parse[i])
			++i;
		std::pair<std::string, std::string> pair(q, charset);
		map.insert(pair);
	}
}

char **HTTP::setEnv(Client &client)
{
	char **env;
	std::map<std::string, std::string> envMap;
	size_t pos;

	envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	envMap["SERVER_SOFTWARE"] = "webserv";
	envMap["REQUEST_URI"] = client.request.uri;
	envMap["REQUEST_METHOD"] = client.request.method;
	envMap["REMOTE_ADDR"] = client.ip;
	envMap["PATH_INFO"] = client.request.uri;
	envMap["PATH_TRANSLATED"] = client.clientConfig["path"];
	envMap["CONTENT_LENGTH"] = std::to_string(client.request.body.size());

	if (client.request.uri.find('?') != std::string::npos)
		envMap["QUERY_STRING"] = client.request.uri.substr(client.request.uri.find('?') + 1);
	else
		envMap["QUERY_STRING"];
	if (client.request.headers.find("Content-Type") != client.request.headers.end())
		envMap["CONTENT_TYPE"] = client.request.headers["Content-Type"];
	if (client.clientConfig.find("exec") != client.clientConfig.end())
		envMap["SCRIPT_NAME"] = client.clientConfig["exec"];
	else
		envMap["SCRIPT_NAME"] = client.clientConfig["path"];
	if (client.clientConfig["listen"].find(':') != std::string::npos)
	{
		envMap["SERVER_NAME"] = client.clientConfig["listen"].substr(0, client.clientConfig["listen"].find(':'));
		envMap["SERVER_PORT"] = client.clientConfig["listen"].substr(client.clientConfig["listen"].find(':') + 1);
	} else
		envMap["SERVER_PORT"] = client.clientConfig["listen"];
	if (client.request.headers.find("Authorization") != client.request.headers.end())
	{
		pos = client.request.headers["Authorization"].find(' ');
		envMap["AUTH_TYPE"] = client.request.headers["Authorization"].substr(0, pos);
		envMap["REMOTE_USER"] = client.request.headers["Authorization"].substr(pos + 1);
		envMap["REMOTE_IDENT"] = client.request.headers["Authorization"].substr(pos + 1);
	}
	if (client.clientConfig.find("php") != client.clientConfig.end() && client.request.uri.find(".php") != std::string::npos)
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

void HTTP::assignMIME()
{
	MIMETypes[".txt"] = "text/plain";
	MIMETypes[".bin"] = "application/octet-stream";
	MIMETypes[".jpeg"] = "image/jpeg";
	MIMETypes[".jpg"] = "image/jpeg";
	MIMETypes[".html"] = "text/html";
	MIMETypes[".htm"] = "text/html";
	MIMETypes[".png"] = "image/png";
	MIMETypes[".bmp"] = "image/bmp";
	MIMETypes[".pdf"] = "application/pdf";
	MIMETypes[".tar"] = "application/x-tar";
	MIMETypes[".json"] = "application/json";
	MIMETypes[".css"] = "text/css";
	MIMETypes[".js"] = "application/javascript";
	MIMETypes[".mp3"] = "audio/mpeg";
	MIMETypes[".avi"] = "video/x-msvideo";
}

int HTTP::getStatusCode(Client &client)
{
	typedef int    (HTTP::*ptr)(Client &client);
	std::map<std::string, ptr> function;
	std::string credential;
	int result;
	function["GET"] = &HTTP::GETStatus;
	function["HEAD"] = &HTTP::GETStatus;
	function["PUT"] = &HTTP::PUTStatus;
	function["POST"] = &HTTP::POSTStatus;
	function["CONNECT"] = &HTTP::CONNECTStatus;
	function["TRACE"] = &HTTP::TRACEStatus;
	function["OPTIONS"] = &HTTP::OPTIONSStatus;
	function["DELETE"] = &HTTP::DELETEStatus;

	if (client.request.method != "CONNECT"
		&& client.request.method != "TRACE"
		&& client.request.method != "OPTIONS")
	{
		client.response.version = "HTTP/1.1";
		client.response.statusCode = OK;
		if (client.clientConfig["methods"].find(client.request.method) == std::string::npos)
			client.response.statusCode = NOTALLOWED;
		else if (client.clientConfig.find("auth") != client.clientConfig.end())
		{
			client.response.statusCode = UNAUTHORIZED;
			if (client.request.headers.find("Authorization") != client.request.headers.end())
			{
				credential = decode64(client.request.headers["Authorization"].c_str());
				if (credential == client.clientConfig["auth"])
					client.response.statusCode = OK;
			}
		}
	}

	result = (this->*function[client.request.method])(client);

	if (result == 0)
		getErrorPage(client);
	return (result);
}

int HTTP::GETStatus(Client &client)
{
	struct stat info = {};

	if (client.response.statusCode == OK)
	{
		errno = 0;
		client.read_fd = open(client.clientConfig["path"].c_str(), O_RDONLY);
		if (client.read_fd == -1 && errno == ENOENT)
			client.response.statusCode = NOTFOUND;
		else if (client.read_fd == -1)
			client.response.statusCode = INTERNALERROR;
		else
		{
			fstat(client.read_fd, &info);
			if (!S_ISDIR(info.st_mode)
				|| (S_ISDIR(info.st_mode) && client.clientConfig["autoindex"] == "on"))
				return (1);
			else
				client.response.statusCode = NOTFOUND;
		}
	}
	return (0);
}

int HTTP::POSTStatus(Client &client)
{
	std::string credential;
	int fd;
	struct stat info = {};

	if (client.response.statusCode == OK && client.clientConfig.find("max_body") != client.clientConfig.end()
		&& client.request.body.size() > (unsigned long) atoi(client.clientConfig["max_body"].c_str()))
		client.response.statusCode = REQTOOLARGE;
	if (client.response.statusCode == OK)
	{
		if (client.clientConfig.find("CGI") != client.clientConfig.end()
			&& client.request.uri.find(client.clientConfig["CGI"]) != std::string::npos)
		{
			if (client.clientConfig["exec"][0])
				client.read_fd = open(client.clientConfig["exec"].c_str(), O_RDONLY);
			else
				client.read_fd = open(client.clientConfig["path"].c_str(), O_RDONLY);
			fstat(client.read_fd, &info);
			if (client.read_fd == -1 || S_ISDIR(info.st_mode))
				client.response.statusCode = INTERNALERROR;
			else
				return (1);
		} else
		{
			errno = 0;
			fd = open(client.clientConfig["path"].c_str(), O_RDONLY);
			if (fd == -1 && errno == ENOENT)
				client.response.statusCode = CREATED;
			else if (fd != -1)
				client.response.statusCode = OK;
			close(fd);
			client.write_fd = open(client.clientConfig["path"].c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
			if (client.write_fd == -1)
				client.response.statusCode = INTERNALERROR;
			else
				return (1);
		}
	}
	return (0);
}

int HTTP::PUTStatus(Client &client)
{
	int fd;
	struct stat info = {};
	int save_err;


	if (client.response.statusCode == OK && client.clientConfig.find("max_body") != client.clientConfig.end()
		&& client.request.body.size() > (unsigned long) atoi(client.clientConfig["max_body"].c_str()))
		client.response.statusCode = REQTOOLARGE;
	else if (client.response.statusCode == OK)
	{
		errno = 0;
		fd = open(client.clientConfig["path"].c_str(), O_RDONLY);
		save_err = errno;
		fstat(fd, &info);
		if (S_ISDIR(info.st_mode))
			client.response.statusCode = NOTFOUND;
		else
		{
			if (fd == -1 && save_err == ENOENT)
				client.response.statusCode = CREATED;
			else if (fd == -1)
			{
				client.response.statusCode = INTERNALERROR;
				return (0);
			} else
			{
				client.response.statusCode = NOCONTENT;
				if (close(fd) == -1)
				{
					client.response.statusCode = INTERNALERROR;
					return (0);
				}
			}
			client.write_fd = open(client.clientConfig["path"].c_str(), O_WRONLY | O_CREAT, 0666);
			if (client.write_fd == -1)
			{
				client.response.statusCode = INTERNALERROR;
				return (0);
			}
			return (1);
		}
	}
	return (0);
}

int HTTP::CONNECTStatus(Client &client)
{
	client.response.version = "HTTP/1.1";
	client.response.statusCode = NOTIMPLEMENTED;
	return (0);
}

int HTTP::TRACEStatus(Client &client)
{
	client.response.version = "HTTP/1.1";
	if (client.clientConfig["methods"].find(client.request.method) == std::string::npos)
	{
		client.response.statusCode = NOTALLOWED;
		return (0);
	} else
	{
		client.response.statusCode = OK;
		return (1);
	}
}

int HTTP::OPTIONSStatus(Client &client)
{
	client.response.version = "HTTP/1.1";
	client.response.statusCode = NOCONTENT;
	return (1);
}

int HTTP::DELETEStatus(Client &client)
{
	int fd;
	struct stat info = {};
	int save_err;

	if (client.response.statusCode == OK)
	{
		errno = 0;
		fd = open(client.clientConfig["path"].c_str(), O_RDONLY);
		save_err = errno;
		fstat(fd, &info);
		if ((fd == -1 && save_err == ENOENT) || S_ISDIR(info.st_mode))
			client.response.statusCode = NOTFOUND;
		else if (fd == -1)
			client.response.statusCode = INTERNALERROR;
		else
			return (1);
	}
	return (0);
}
