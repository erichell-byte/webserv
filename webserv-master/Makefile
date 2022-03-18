NAME = webserv
 CFLAGS = -Wall -Wextra -Werror -std=c++98
 FILES = Utils Request Config main
 FILES = main Client HTTP Server Utils Webserv
 SRCS = $(addprefix srcs/, $(addsuffix .cpp, $(FILES)))
 OBJS = $(SRCS:.cpp=.o)
 CXX = clang++ -g
 DIR_CONFIGS =	./config/
 CONFIG	= default.cfg
 PWD = $(shell pwd)
all : $(NAME) $(DIR_CONFIGS)$(CONFIG)
	@cat $(DIR_CONFIGS)/default.cfg | sed 's=PWD=$(PWD)=g' > $(DIR_CONFIGS)localhost.cfg 
	mkdir Download
$(NAME) : $(OBJS)
	$(CXX) $(CFLAGS) $(SRCS) -o $(NAME)	
clean:
	rm -f $(OBJS)
fclean: 
	rm $(OBJS)
	rm -Rfv Download 
	rm $(NAME) 
re: fclean all
.PHONY: clean fclean re all