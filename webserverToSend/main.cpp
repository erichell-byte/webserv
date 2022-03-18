#include <vector>

#include "server.hpp"
#include "parser.hpp"
#include "listener.hpp"


int main()
{
	Listener 	my_listener;
	parser 		parser;

	my_listener.servers = parser.fileParser("server_settings.txt");

// Распечатать распарсенный конфиг
/*	int i = 0;
	for (std::vector<server>::const_iterator it = my_listener.servers.begin(); 
		it != my_listener.servers.end(); ++it, ++i)		
		my_listener.servers[i].debugPrint();
*/

	my_listener.listenPoll();
	return 0;
}
