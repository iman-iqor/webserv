#include "utils.hpp"

std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    return s.substr(start, end - start + 1);
}

bool has_any(const std::string &s, const std::string &chars) {
    for (size_t i = 0; i < s.length(); ++i) {
        if (chars.find(s[i]) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool has_other(const std::string &s, std::string &chars) {
    for (size_t i = 0; i < s.length(); i++) {
        if (chars.find(s[i]) == std::string::npos) {
            return true;
        }
    }
    return false;
}

std::string& to_lower(std::string &s) {
    for (size_t i = 0; i < s.length(); ++i) {
        s[i] = std::tolower(s[i]);
    }
    return s;
}

std::string to_upper(const std::string &s) {
    std::string result = s;
    for (size_t i = 0; i < s.length(); ++i) {
        result[i] = std::toupper(s[i]);
    }
    return result;
}

std::string ft_itoa(int n) {
    std::stringstream ss;
    ss << n;
    return ss.str();
}

void print_safe_string(const std::string& str)
{
    // Save current stream flags so we can restore them later
    std::ios_base::fmtflags old_flags = std::cout.flags();
    char old_fill = std::cout.fill();

    for (size_t i = 0; i < str.size(); ++i) 
    {
        unsigned char ch = static_cast<unsigned char>(str[i]);

        // Check if the character is printable (and not a carriage return/newline)
        if (std::isprint(ch)) 
        {
            std::cout << ch;
        } 
        else 
        {
            // Print the non-printable byte as \xHH (e.g., \x00, \x0a)
            std::cout << "\\x" 
                      << std::hex 
                      << std::setw(2) 
                      << std::setfill('0') 
                      << static_cast<int>(ch);
        }
    }
    
    // Restore original stream states to avoid messing up subsequent couts
    std::cout.flags(old_flags);
    std::cout.fill(old_fill);
    std::cout << std::endl;
}