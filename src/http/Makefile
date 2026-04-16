CXX = c++
# CXXFLAGS = -Wall -Wextra -Werror -std=c++98
NAME = test_http

SRC = \
	main.cpp \
	Header.cpp \
	Request.cpp \
	tests/test_utils.cpp \
	tests/headers_tests.cpp \
	tests/cookies_tests.cpp \
	tests/request_tests.cpp \
	../utils/string_utils.cpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) -g -o $(NAME)

%.o: %.cpp
	$(CXX) -c $< -g -o $@

test: $(NAME) clean
	./$(NAME)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all test clean fclean re