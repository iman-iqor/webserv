NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRC = \
	main.cpp \
	src/config/Parser.cpp \
	src/config/tokenizer.cpp\
	src/http/Header.cpp \
	src/http/Request.cpp \
	src/http/Response.cpp \
	src/http/CgiHandler.cpp \
	src/Router/Router.cpp \
	src/Router/RouterHelper.cpp \
	src/Router/RouterGet.cpp \
	src/Router/RouterPost.cpp \
	src/Router/RouterDelete.cpp \
	src/server/Server.cpp \
	src/server/AcceptClient.cpp \
	src/server/HandleClient.cpp \
	src/server/handleFileUpload.cpp \
	src/server/handleDeleteFile.cpp \
	src/server/buildResponse.cpp \
	src/server/HandleCGI.cpp \
	src/server/Client.cpp \
	src/server/Socket.cpp \
	src/utils/string_utils.cpp \
	src/utils/location_tool.cpp \
	src/utils/request_utils.cpp

OBJ_DIR = obj_file
OBJ = $(addprefix $(OBJ_DIR)/,$(SRC:.cpp=.o))


all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	clear
	./webserv config.conf

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all


.PHONY: all clean fclean re
