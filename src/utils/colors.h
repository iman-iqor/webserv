#ifndef COLORS_H
# define COLORS_H

// Reset
# define RESET          "\033[00m"

// Standard Colors (Bold for better visibility in terminal)
# define RED            "\033[31m"      // Use for 4xx and 5xx Errors
# define GREEN          "\033[32m"      // Use for 2xx Success
# define YELLOW         "\033[33m"      // Use for Warnings / Config parsing
# define BLUE           "\033[34m"      // Use for Socket/Multiplexing info (Imane's part)
# define MAGENTA        "\033[35m"      // Use for Header info (Radouane's part)
# define CYAN           "\033[36m"      // Use for Request-Line / Method info
# define WHITE          "\033[37m"

// Bold Versions
# define BOLD_RED       "\033[1;31m"
# define BOLD_GREEN     "\033[1;32m"
# define BOLD_YELLOW    "\033[1;33m"
# define BOLD_BLUE      "\033[1;34m"
# define BOLD_MAGENTA   "\033[1;35m"
# define BOLD_CYAN      "\033[1;36m"

// Backgrounds (Useful for highlighting specific bytes in a buffer)
# define BG_RED         "\033[41m"
# define BG_GREEN       "\033[42m"
# define BG_YELLOW      "\033[43m"

#endif