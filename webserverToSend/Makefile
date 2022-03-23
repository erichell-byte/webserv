NAME = webserv

CXX = clang++ -g -std=c++98 -pedantic
# CXXLAGS = -Wall -Werror -Wextra
RM = rm -f

FILES =  $(wildcard $/*.cpp)
OBJS = $(FILES:%.cpp=%.o)
PWD = $(shell pwd)
CONFIG	= server_settings.txt
%.o: %.cpp %.hpp
	$(CXX) $(CXXLAGS) -c $< -o $@

$(NAME): $(OBJS) 
	$(CXX) $(CXXLAGS) $(OBJS) -o $(NAME)

all: $(NAME) $(CONFIG)
	@cat default.txt | sed 's=PWD=$(PWD)=g' > server_settings.txt

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY:all clean fclean re
