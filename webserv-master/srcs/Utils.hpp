#ifndef UTILS_HPP
#define UTILS_HPP

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

#define OK 				"200 OK"
#define CREATED			"201 Created"
#define NOCONTENT		"204 No Content"
#define BADREQUEST		"400 Bad Request"
#define UNAUTHORIZED	"401 Unauthorized"
#define NOTFOUND 		"404 Not Found"
#define NOTALLOWED		"405 Method Not Allowed"
#define REQTOOLARGE		"413 Request Entity Too Large"
#define UNAVAILABLE		"503 Service Unavailable"
#define NOTIMPLEMENTED	"501 Not Implemented"
#define INTERNALERROR	"500 Internal Server Error"
#define BUFFER_SIZE 32767

# define GREY			"\x1b[30m"
# define RED			"\x1b[31m"
# define GREEN			"\x1b[32m"
# define YELLOW			"\x1b[33m"
# define BLUE			"\x1b[34m"
# define PURPLE			"\x1b[35m"
# define CYAN			"\x1b[36m"
# define WHITE			"\x1b[37m"
# define END			"\x1b[0m"
# define UNDER			"\x1b[4m"

#define TMP_PATH 	"/tmp/cgi.tmp"

#define TIMEOUT 10
#define RETRY	"25"
#define maxFD 1023

typedef std::map<std::string, std::string> element;
typedef std::map<std::string, element> Config;

struct Request
{
	bool isValid;
	std::string method;
	std::string uri;
	std::string version;
	std::map<std::string, std::string> headers;
	std::string body;

	void clear()
	{
		method.clear();
		uri.clear();
		version.clear();
		headers.clear();
		body.clear();
	}
};

struct Response
{
	std::string version;
	std::string statusCode;
	std::map<std::string, std::string> headers;
	std::string body;

	void clear()
	{
		version.clear();
		statusCode.clear();
		headers.clear();
		body.clear();
	}
};

struct w_chunk
{
	unsigned int length;
	bool isDone;
	bool isFound;
};

enum status
{
	BODYPARSING,
	CODE,
	HEADERS,
	CGI,
	BODY,
	RESPONSE,
	STANDBY,
	DONE
};

static const int B64index[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
								  56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6,
								  7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,
								  0, 0, 0, 63, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
								  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

class WebservException : public std::exception
{
private:
	size_t line;
	std::string function;
	std::string error;

	WebservException()
	{ this->line = 0; };

public:
	explicit WebservException(size_t d)
	{
		this->line = d;
		this->error = "line " + std::to_string(this->line) + ": Invalid Webserv File";
	}

	WebservException(std::string function, std::string error)
	{
		this->error = function + ": " + error;
	}

	virtual const char *what() const throw()
	{
		return (error.c_str());
	}

	virtual ~WebservException() throw()
	{

	}
};

namespace ft
{
	bool isSpace(int c);

	void get_next_line(std::string &buffer, std::string &line);

	void get_next_line(std::string &buffer, std::string &line, char delChar);

	int pow(int n, int power);

	std::string getDate();

	void freeAll(char **args, char **env);
}

namespace utils
{
	void showMessage(std::string text, const std::string& color);
}

#endif
