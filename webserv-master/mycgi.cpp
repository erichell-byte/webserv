#include <iostream>
# define RED "\x1b[41m"
# define UNDER "\x1b[4m"
# define END "\x1b[0m"
int main(int argc, char **argv, char **env)
{
	std::cout << "argc = " << argc << std::endl;
	std::cout << "-----------------------ARGV----------------------" << std::endl;
	while (*argv)
	{
		std::cout << *argv++ << std::endl;		
	}
	std::cout << "-----------------------ENV-----------------------" << std::endl;
	while (*env)
	{
		std::cout << *env++ << std::endl;
	}
	std::cout << RED UNDER "Hello World, motherfucker!" END << std::endl;
	return (0);
}