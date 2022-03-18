#include "parser.hpp"    
#include "server.hpp"

std::string stringEraseBeginSpace(std::string line)
{
	size_t pos = line.find_first_not_of(" \t\n\r\f\v");
	if (pos == std::string::npos)
		return ("");
	line.erase(0, pos);	
	return (line);
}

std::vector<std::string>	stringToStringVector(std::string incomeString)
{
	std::istringstream 			incomeStringStream( incomeString );
	std::string 				partOfString;
	std::vector<std::string> 	result;

	while ( std::getline( incomeStringStream, partOfString , ' '))
	{
		result.push_back(partOfString);
	}
	return (result);
}

std::vector<int>	stringToIntVector(std::string incomeString)
{
	std::istringstream 			incomeStringStream( incomeString );
	std::string 				partOfString;
	int							buffer;
	std::vector<int> 			result;

	while ( std::getline( incomeStringStream, partOfString , ' '))
	{
		buffer = atoi( partOfString.c_str() );
		if (buffer == 0)
			exit(EXIT_FAILURE);;
		result.push_back(buffer);
	}
	return (result);
}


std::vector<server> parser::fileParser(const char* file_name)
{
	std::ifstream file( file_name );

	while ( std::getline( file, line ) )   
	{
		if (line.find_first_not_of(' ') == std::string::npos)
			continue ;
		
		if( line == "server{" )
			this->serverParser(file);
	}
	return (servers);
}

void parser::serverParser(std::ifstream& file)
{
	server instance;
	currentServer = instance;
	
	while ( std::getline( file, line ) ) 
	{
		line = stringEraseBeginSpace(line);
		if (line.empty() == 1)
			continue ;

		std::istringstream iss( line ); 
		if ( std::getline( iss, key , ' '))
		{
			std::getline( iss, value, ';' );
		}
		
		switch (StringToServerType(key))
		{
			case host:
			{
				currentServer.host = value;
				break ;
			}
			case port:
			{
				currentServer.port = atoi( value.c_str() );
				if (currentServer.port == 0)
				{
					std::cout << "Error with port" << std::endl;
					exit(EXIT_FAILURE);
				}
					
				break;
			}
			case server_names:
			{
				currentServer.server_names = stringToStringVector(value);
				break;
			}	
			case error_page:
			{
				this->errorPageParser( value );
				break ;
			}

			case client_max_body_size:
			{
				currentServer.client_max_body_size = atoi( value.c_str() );
				if (currentServer.client_max_body_size == 0)
				{
					std::cout << "Error with client_max_body_size" << std::endl;
					exit(EXIT_FAILURE);
				}
				break;
			}

			case locationT:
			{
				this->locationParser(file);
				break;
			}
			
			case closeServer:
			{
				servers.push_back(currentServer);
				return ;
			}
			default:
				std::cout <<  "ITS \"ERROR\"" << std::endl;
				exit(EXIT_FAILURE) ;
			//	LogError("Unknown TestType ", s_mapTestTypeToString[testType]);
		}

	}
}

void parser::locationParser(std::ifstream& file)
{
	struct location LocationInstance;
	currentLocation = LocationInstance;
	while ( std::getline( file, line ) ) 
	{
		line = stringEraseBeginSpace(line);
		if (line.empty() == 1)
			continue ;

		std::istringstream iss( line ); 
		if ( std::getline( iss, key , ' '))
		{
			std::getline( iss, value, ';' );
		}
		switch (StringToLocationType(key))
		{
			case uri:
			{
				this->currentLocation.locationURI = value;
				break;
			}
			case root:
			{
				this->currentLocation.root = value;
				break;
			}
			case proxy_pass:
			{
				this->currentLocation.proxy_pass = value;
				break;
			}
			case limit_except:
			{
				currentLocation.limit_except = stringToStringVector(value);
				break;
			}
			case autoindex:
			{
				if (value == "on")
					currentLocation.autoindex = 1;
				else
					currentLocation.autoindex = 0;
				break;
			}
			case index:
			{
				this->currentLocation.index = value;
				break;
			}
			case fastcgi_pass:
			{
				this->currentLocation.fastcgi_path = value;
				break;
			}
			case upload_store:
			{
				this->currentLocation.upload_store = value;
				break;
			}
			case closeLocation:
			{
				currentServer.locations.push_back(currentLocation);
				return ;
			}
			case auth:
			{
				this->currentLocation.auth = value;
				break;
			}
			default:
			{
				std::cout <<  "ITS \"ERROR\" in location" << std::endl;
				exit(EXIT_FAILURE) ;
			//	LogError("Unknown TestType ", s_mapTestTypeToString[testType]);
				break ;
			}
		}
	}
}

void parser::errorPageParser(std::string errorPageValue)
{
	struct errorPage 	currentErrorPage;
	std::istringstream 	errorPageStream( errorPageValue );
	std::string 		errorPageCode;
	std::string 		errorPageURI;

	std::getline( errorPageStream, errorPageCode , '/');
	std::getline( errorPageStream, errorPageURI , ';');
	
	currentErrorPage.errorCode = stringToIntVector(errorPageCode);
	currentErrorPage.errorPage = errorPageURI;
	this->currentServer.errorPages.push_back(currentErrorPage);
}
