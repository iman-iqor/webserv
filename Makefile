# ===================== #
#       VARIABLES       #
# ===================== #

NAME = webserv
MOCK_NAME = webserv_mock

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++11

MAIN = src/main.cpp
MAIN_MOCK = src/main_mock.cpp

SRC = \
	src/config/Parser.cpp \
	src/config/tokenizer.cpp\
	src/http/Header.cpp \
	src/http/Request.cpp \
	src/http/Response.cpp \
	src/server/Server.cpp \
	src/server/Client.cpp \
	src/server/Socket.cpp \
	src/utils/string_utils.cpp \
	src/utils/request_utils.cpp

OBJ = $(SRC:.cpp=.o)
MAIN_OBJ = $(MAIN:.cpp=.o)
MAIN_MOCK_OBJ = $(MAIN_MOCK:.cpp=.o)

# ===================== #
#         RULES         #
# ===================== #

all: $(NAME)

mock: $(MOCK_NAME)

$(NAME): $(OBJ) $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $(NAME)

$(MOCK_NAME): $(OBJ) $(MAIN_MOCK_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $(MOCK_NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(MAIN_OBJ) $(MAIN_MOCK_OBJ)

fclean: clean
	rm -f $(NAME) $(MOCK_NAME)

re: fclean all

# ===================== #
#       PHONY           #
# ===================== #

.PHONY: all mock clean fclean re