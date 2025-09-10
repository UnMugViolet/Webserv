# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: unmugviolet <unmugviolet@student.42.fr>    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/04/24 11:06:17 by unmugviolet       #+#    #+#              #
#    Updated: 2025/09/10 14:33:00 by unmugviolet      ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = 			webserv

ARGS =			./configs/default.conf
STRESS_ARGS =	./configs/stress.conf

ADDR =			http://localhost:
PORT =			8080

MAIN_FILES = 	main.cpp
SRC_FILES = 	Webserv.cpp ConfigParser.cpp CGI.cpp Server.cpp RequestHandler.cpp Logger.cpp \
				ARequest.cpp GetRequest.cpp PostRequest.cpp DeleteRequest.cpp

SRC_DIR =		./srcs/
OBJ_DIR =		./objects/
INC_DIR =		./includes/
LOGS_DIR = 		./logs/

OBJ =	$(addprefix $(OBJ_DIR), $(SRC_FILES:.cpp=.o)) \
		$(MAIN_FILES:.cpp=.o)

CC = c++
MAKE = make
CFLAGS = -Wall -Werror -Wextra -g3 -std=c++98

DEFAULT = \033[0m
DEF_COLOR = \033[0;90m
WHITE = \033[1;37m
GREEN = \033[0;92m
YELLOW = \033[0;93m
CYAN = \033[0;96m

all: $(NAME)

$(NAME) : $(OBJ)
	@echo "$(GREEN)$(NAME) compiled!$(DEFAULT)"
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp | $(OBJ_DIR)
	@echo "$(YELLOW)Compiling: $< $(DEF_COLOR)"
	@$(CC) $(CFLAGS) -I $(INC_DIR) -c $< -o $@

%.o : %.cpp | $(OBJ_DIR)
	@echo "$(YELLOW)Compiling: $< $(DEF_COLOR)"
	@$(CC) $(CFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LOGS_DIR)

clean: 
	@rm -rf $(OBJ_DIR)
	@echo "$(GREEN)$(NAME) object directory cleaned!$(DEFAULT)"

fclean: clean
	@rm -rf $(NAME)
	@rm -rf $(LOGS_DIR)
	@echo "$(CYAN)$(NAME) executables and objects removed succesfully!$(DEFAULT)"

go: all
	@./$(NAME) $(ARGS)
	@rm -rf $(NAME)

gov: all
	@ valgrind --leak-check=full --show-leak-kinds=all ./$(NAME) $(ARGS)
	@rm -rf $(NAME)

stress: all
	@./$(NAME) $(STRESS_ARGS) &
	@sleep 2
	@siege -b $(ADDR)$(PORT) -t 1m
	@rm -rf $(NAME)
	@$(MAKE) kill

kill:
	@pkill -9 webserv

curl: 
	@$(MAKE) -j2 go &
	@sleep 2
	@curl $(ADDR)$(PORT)
	
re: fclean all

rego: re go

.PHONY: all clean fclean re go gov
