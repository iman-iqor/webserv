# ===================== #
#       VARIABLES       #
# ===================== #

NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRC = \
	src/main.cpp \
	src/config/Parser.cpp \
	src/config/tokenizer.cpp\
	src/http/Request.cpp \
	src/http/Response.cpp \
	src/server/Server.cpp \
	src/server/Client.cpp \
	src/server/Socket.cpp

OBJ = $(SRC:.cpp=.o)

# ===================== #
#         RULES         #
# ===================== #

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

# ===================== #
#       PHONY           #
# ===================== #

.PHONY: all clean fclean re