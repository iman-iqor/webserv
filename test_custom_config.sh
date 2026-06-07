#!/bin/bash

# Color codes for beautiful terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m'

# Server targets based on your config.conf
SERVER_MAIN="127.0.0.1:9090"
SERVER_TEST="127.0.0.1:7070"
TIMEOUT=5

PASSED=0
FAILED=0
TOTAL=0

# Pre-test Setup: Create directories and files needed for testing
echo -e "${BLUE}Setting up test environment (creating dummy files)...${NC}"
mkdir -p src/www/file
mkdir -p src/www/upload
touch src/www/index.html
echo "hello world" > src/www/file/delete_me.txt

# Array of test cases: [description | curl_command | expected_status]
declare -a TESTS=(
    # === 1. ROOT LOCATION ( / ) ===
    # Config says: methods GET; index index.html; autoindex on;
    "GET to / (Should Succeed)|curl -s -o /dev/null -w '%{http_code}' ${SERVER_MAIN}/|200"
    "POST to / (Method Not Allowed)|curl -X POST -s -o /dev/null -w '%{http_code}' ${SERVER_MAIN}/|405"
    "DELETE to / (Method Not Allowed)|curl -X DELETE -s -o /dev/null -w '%{http_code}' ${SERVER_MAIN}/|405"
    "GET to non-existent file (404 Not Found)|curl -s -o /dev/null -w '%{http_code}' ${SERVER_MAIN}/does_not_exist.html|404"

    # === 2. UPLOAD LOCATION ( /upload ) ===
    # Config says: methods POST;
    "POST to /upload (Should Create)|curl -X POST -d 'test data' -s -o /dev/null -w '%{http_code}' ${SERVER_MAIN}/upload|201"
    "GET to /upload (Method Not Allowed)|curl -s -o /dev/null -w '%{http_code}' ${SERVER_MAIN}/upload|405"

    # === 3. DELETE LOCATION ( /file ) ===
    # Config says: methods DELETE;
    "DELETE existing file in /file|curl -X DELETE -s -o /dev/null -w '%{http_code}' ${SERVER_MAIN}/file/delete_me.txt|200"
    "DELETE missing file in /file (404 Not Found)|curl -X DELETE -s -o /dev/null -w '%{http_code}' ${SERVER_MAIN}/file/ghost.txt|404"
    "GET to /file (Method Not Allowed)|curl -s -o /dev/null -w '%{http_code}' ${SERVER_MAIN}/file|405"

    # === 4. REDIRECTION LOCATION ( /old-page ) ===
    # Config says: return_code 301;
    "GET to /old-page (Should Redirect 301)|curl -s -o /dev/null -w '%{http_code}' ${SERVER_MAIN}/old-page|301"

    # === 5. SERVER 2 (PORT 7070) ===
    # Config says: root /tmp; methods GET POST DELETE;
    "GET to Server 2 on Port 7070|curl -s -o /dev/null -w '%{http_code}' ${SERVER_TEST}/|200"
    "POST to Server 2 on Port 7070 (Allowed)|curl -X POST -d 'test' -s -o /dev/null -w '%{http_code}' ${SERVER_TEST}/|201"
)

echo -e "\n${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Starting webserv Custom Config Test Suite${NC}"
echo -e "${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"

for test in "${TESTS[@]}"; do
    IFS='|' read -r description command expected_status <<< "$test"
    
    ((TOTAL++))
    
    # Run the command and capture just the HTTP status code
    actual_status=$(eval "$command")
    
    if [ "$actual_status" == "$expected_status" ]; then
        # Check if the code was 204 (No Content) which is also highly acceptable for DELETE
        echo -e "${GREEN}✓ PASSED${NC} : $description (Got $actual_status)"
        ((PASSED++))
    elif [ "$expected_status" == "200" ] && [ "$actual_status" == "204" ] && [[ "$description" == *"DELETE"* ]]; then
        # Special catch for DELETE (200 and 204 are both valid successes)
        echo -e "${GREEN}✓ PASSED${NC} : $description (Got $actual_status No Content)"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAILED${NC} : $description"
        echo -e "   Expected: ${YELLOW}$expected_status${NC}, Got: ${RED}${actual_status:-ERROR}${NC}"
        ((FAILED++))
    fi
done

echo -e "\n${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Test Summary${NC}"
echo -e "${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}✓ Passed:${NC}  $PASSED"
echo -e "${RED}✗ Failed:${NC}  $FAILED"
echo -e "${BLUE}Total:${NC}   $TOTAL"

# Post-test Cleanup
rm -f src/www/file/delete_me.txt

if [ $FAILED -eq 0 ]; then
    echo -e "\n${GREEN}🎉 All customized config tests passed! Your routing logic is solid.${NC}\n"
    exit 0
else
    echo -e "\n${RED}❌ Some tests failed! Check the output above to see which routes are acting up.${NC}\n"
    exit 1
fi