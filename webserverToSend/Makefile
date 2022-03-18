NAME = webserv

CXX = clang++ -g -std=c++98 -pedantic
# CXXLAGS = -Wall -Werror -Wextra
RM = rm -f

FILES =  $(wildcard $/*.cpp)
OBJS = $(FILES:%.cpp=%.o)
PWD = $(shell pwd)

%.o: %.cpp %.hpp
	$(CXX) $(CXXLAGS) -c $< -o $@

$(NAME): $(OBJS) 
	$(CXX) $(CXXLAGS) $(OBJS) -o $(NAME)

all: $(NAME)
##sed 's=PWD=$(PWD)=g' > ./www/server_settings.txt

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY:all clean fclean re
