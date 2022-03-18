#include <string>
#include "Utils.hpp"

namespace ft
{
	bool isSpace(int c)
	{
		if (c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f' ||
			c == ' ')
			return (true);
		return (false);
	}

	void get_next_line(std::string &buffer, std::string &line)
	{
		size_t pos;

		pos = buffer.find("\n");
		if (pos != std::string::npos)
		{
			line = std::string(buffer, 0, pos++);
			buffer = buffer.substr(pos);
		} else
		{
			if (buffer[buffer.size() - 1] == '\n')
				buffer = buffer.substr(buffer.size());
			else
			{
				line = buffer;
				buffer = buffer.substr(buffer.size());
			}
		}
	}

	void get_next_line(std::string &buffer, std::string &line, char delChar)
	{
		size_t pos;

		pos = buffer.find(delChar);
		if (pos != std::string::npos)
		{
			line = std::string(buffer, 0, pos++);
			buffer = buffer.substr(pos);
		} else
		{
			if (buffer[buffer.size() - 1] == delChar)
				buffer = buffer.substr(buffer.size());
			else
			{
				line = buffer;
				buffer = buffer.substr(buffer.size());
			}
		}
	}

	int pow(int n, int power)
	{
		if (power < 0)
			return (0);
		if (power == 0)
			return (1);
		return (n * pow(n, power - 1));
	}

	std::string getDate()
	{
		struct timeval time;
		struct tm *tm;
		char buf[BUFFER_SIZE + 1];
		int ret;

		gettimeofday(&time, NULL);
		tm = localtime(&time.tv_sec);
		ret = strftime(buf, BUFFER_SIZE, "%a, %d %b %Y %T %Z", tm);
		buf[ret] = '\0';
		return (buf);
	}

	void freeAll(char **args, char **env)
	{
		free(args[0]);
		free(args[1]);
		free(args);
		int i = 0;
		while (env[i])
		{
			free(env[i]);
			++i;
		}
		free(env);
	}
}

namespace utils
{
	std::string getTime()
	{
		time_t now = 0;
		tm *ltm = NULL;
		char buffer[1024];
		std::string result;

		now = time(0);
		if (now)
			ltm = localtime(&now);
		strftime(buffer, 1024, "%d/%m/%y %T", ltm);
		result = buffer;
		result.insert(result.begin(), '[');
		result.insert(result.end(), ']');
		return (result);
	}

	void showMessage(std::string text, const std::string& color)
	{
		std::string log;

		log += getTime();
		log += " " + text;
		std::cout << color << log << END << std::endl;
	}
}
