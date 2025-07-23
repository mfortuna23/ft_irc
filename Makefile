NAME    = ircserv
CPP     = c++
CPPFLG  = -Wall -Wextra -Werror -std=c++98
SRC     = main.cpp Server.cpp irc_utils.cpp Client.cpp Commands.cpp Channel.cpp
OBJDIR  = obj
OBJS    = $(SRC:%.cpp=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CPP) $(CPPFLG) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	$(CPP) $(CPPFLG) -g -I . -c $< -o $@

clean:
	rm -fr $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all
