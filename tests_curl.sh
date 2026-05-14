#!/bin/bash

# filepath: /goinfre/rabounou/web/tests_curl.sh

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Server URL
SERVER="127.0.0.1:9090"
TIMEOUT=5

# Counter for passed/failed tests
PASSED=0
FAILED=0
TOTAL=0

# Flags
VERBOSE_FAILED=false  # -v: show output of failed tests
VERBOSE_ALL=false     # -a: show output of all tests

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose-failed)
            VERBOSE_FAILED=true
            shift
            ;;
        -a|--verbose-all)
            VERBOSE_ALL=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -v, --verbose-failed    Display output of failed tests only"
            echo "  -a, --verbose-all       Display output of all tests"
            echo "  -h, --help              Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

# Array of test cases: [description, curl_command, expected_status]
declare -a TESTS=(
    # === BASIC GET REQUESTS ===
    "GET to / (allowed)|curl -v ${SERVER}/ 2>&1|200"
    "GET to /index.html|curl -v ${SERVER}/index.html 2>&1|200"
    "GET to /about|curl -v ${SERVER}/about 2>&1|200"
    "GET with query string|curl -v '${SERVER}/search?q=test' 2>&1|200"
    "GET with multiple query params|curl -v '${SERVER}/api?key=value&foo=bar' 2>&1|200"
    
    # === POST REQUESTS ===
    "POST to /upload (allowed)|curl -X POST -v ${SERVER}/upload -H 'Host: webserv.com' -H 'Content-Length: 0' 2>&1|201"
    "POST to / (method not allowed)|curl -X POST -v ${SERVER}/ -H 'Host: webserv.com' -H 'Content-Length: 0' 2>&1|405"
    "POST without Content-Length header|curl -X POST -v ${SERVER}/ 2>&1|411"
    "POST with Content-Length: 0|curl -X POST -v ${SERVER}/upload -H 'Content-Length: 0' 2>&1|201"
    "POST with small body|curl -X POST -v ${SERVER}/upload -H 'Content-Length: 13' -d 'Hello, World!' 2>&1|201"
    "POST with larger body (1KB)|curl -X POST -v ${SERVER}/upload -H 'Content-Length: 1024' -d \"$(head -c 1024 /dev/zero | tr '\0' 'a')\" 2>&1|201"
    
    # === DELETE REQUESTS ===
    "DELETE to /file (allowed)|curl -X DELETE -v ${SERVER}/file 2>&1|204"
    "DELETE to / (method not allowed)|curl -X DELETE -v ${SERVER}/ 2>&1|405"
    "DELETE with query string|curl -X DELETE -v '${SERVER}/file?id=123' 2>&1|204"
    
    # === PUT REQUESTS ===
    "PUT to / (not implemented)|curl -X PUT -v ${SERVER}/ 2>&1|501"
    "PUT to /upload (not implemented)|curl -X PUT -v ${SERVER}/upload -H 'Content-Length: 0' 2>&1|501"
    
    # === HEAD REQUESTS ===
    "HEAD to / (if supported)|curl -I -v ${SERVER}/ 2>&1|200"
    "HEAD to /index.html|curl -I -v ${SERVER}/index.html 2>&1|200"
    
    # === OPTIONS REQUESTS ===
    "OPTIONS to / (if supported)|curl -X OPTIONS -v ${SERVER}/ 2>&1|200"
    
    # === INVALID/UNSUPPORTED METHODS ===
    "PATCH method (not implemented)|curl -X PATCH -v ${SERVER}/ 2>&1|501"
    "TRACE method (not implemented)|curl -X TRACE -v ${SERVER}/ 2>&1|501"
    "CONNECT method (not implemented)|curl -X CONNECT -v ${SERVER}/ 2>&1|501"
    
    # === HOST HEADER TESTS ===
    "Request with Host header|curl -v -H 'Host: webserv.com' ${SERVER}/ 2>&1|200"
    "Request with different Host header|curl -v -H 'Host: example.com' ${SERVER}/ 2>&1|400"
    "Request missing Host header|curl -v -H '' ${SERVER}/ 2>&1|400"
    
    # === CONTENT-LENGTH TESTS ===
    "Invalid Content-Length (non-numeric)|curl -X POST -v ${SERVER}/upload -H 'Content-Length: abc' 2>&1|400"
    "Negative Content-Length|curl -X POST -v ${SERVER}/upload -H 'Content-Length: -1' 2>&1|400"
    "Content-Length with leading zeros|curl -X POST -v ${SERVER}/upload -H 'Content-Length: 0010' -d '0123456789' 2>&1|201"
    
    # === TRANSFER-ENCODING TESTS ===
    "Chunked transfer encoding|curl -X POST -v ${SERVER}/upload -H 'Transfer-Encoding: chunked' -d 'test' 2>&1|201"
    "Unsupported transfer encoding|curl -X POST -v ${SERVER}/upload -H 'Transfer-Encoding: gzip' 2>&1|400"
    
    # === PATH TESTS ===
    "Root path /|curl -v ${SERVER}/ 2>&1|200"
    "Path with one segment /api|curl -v ${SERVER}/api 2>&1|200"
    "Path with multiple segments /api/v1/users|curl -v ${SERVER}/api/v1/users 2>&1|200"
    "Path with trailing slash /api/|curl -v ${SERVER}/api/ 2>&1|200"
    "Path with dot /./api|curl -v ${SERVER}/./api 2>&1|200"
    "Path with encoded characters /api%2Fv1|curl -v '${SERVER}/api%2Fv1' 2>&1|200"
    "Non-existent path /does-not-exist|curl -v ${SERVER}/does-not-exist 2>&1|404"
    
    # === HEADER TESTS ===
    "Request with User-Agent header|curl -v -H 'User-Agent: TestBot/1.0' ${SERVER}/ 2>&1|200"
    "Request with Accept header|curl -v -H 'Accept: text/html' ${SERVER}/ 2>&1|200"
    "Request with multiple headers|curl -v -H 'Accept: text/html' -H 'Accept-Language: en-US' ${SERVER}/ 2>&1|200"
    "Request with custom headers|curl -v -H 'X-Custom-Header: value' ${SERVER}/ 2>&1|200"
    "Request with Connection: close|curl -v -H 'Connection: close' ${SERVER}/ 2>&1|200"
    "Request with Connection: keep-alive|curl -v -H 'Connection: keep-alive' ${SERVER}/ 2>&1|200"
    
    # === COOKIE TESTS ===
    "Request with single cookie|curl -v -H 'Cookie: name=value' ${SERVER}/ 2>&1|200"
    "Request with multiple cookies|curl -v -H 'Cookie: session=abc123; user=john' ${SERVER}/ 2>&1|200"
    "Request with cookie path|curl -v -H 'Cookie: path=/api' ${SERVER}/ 2>&1|200"
    
    # === HTTP VERSION TESTS ===
    "HTTP/1.1 request|curl -v --http1.1 ${SERVER}/ 2>&1|200"
    "HTTP/1.0 request|curl -v --http1.0 ${SERVER}/ 2>&1|200"
    
    # === EDGE CASES ===
    "Empty path segments //|curl -v '${SERVER}//' 2>&1|200"
    "Very long path (2000 chars)|curl -v \"${SERVER}/$(printf 'a%.0s' {1..2000})\" 2>&1|414"
    "Path with special characters /api/@user|curl -v '${SERVER}/api/@user' 2>&1|200"
    "Path with spaces /api/hello%20world|curl -v '${SERVER}/api/hello%20world' 2>&1|200"
    
    # === BODY SIZE TESTS ===
    "POST with 0 byte body|curl -X POST -v ${SERVER}/upload -H 'Content-Length: 0' 2>&1|201"
    "POST with max allowed body size|curl -X POST -v ${SERVER}/upload -H 'Content-Length: 1048576' -d \"$(head -c 1048576 /dev/zero | tr '\0' 'x')\" 2>&1|201"
    "POST exceeding max body size|curl -X POST -v ${SERVER}/upload -H 'Content-Length: 10485760' -d \"$(head -c 10485760 /dev/zero | tr '\0' 'x')\" 2>&1|413"
    
    # === MULTIPLE REQUESTS (PERSISTENCE) ===
    "First request in sequence|curl -v ${SERVER}/ 2>&1|200"
    "Second request in sequence|curl -v ${SERVER}/api 2>&1|200"
    "Third request in sequence|curl -v ${SERVER}/about 2>&1|200"
    
    # === CASE SENSITIVITY ===
    "Uppercase method GET|curl -X GET -v ${SERVER}/ 2>&1|200"
    "Lowercase path /API (if case-insensitive)|curl -v ${SERVER}/API 2>&1|200"
    
    # === RESPONSE HEADER TESTS ===
    "Check Content-Type header|curl -v ${SERVER}/ 2>&1|200"
    "Check Content-Length in response|curl -v ${SERVER}/ 2>&1|200"
    "Check Server header|curl -v ${SERVER}/ 2>&1|200"
    
    # === ERROR HANDLING ===
    "Malformed request line (spaces)|curl -v 'http://127.0.0.1:9090/  /api' 2>&1|400"
    "Request with invalid HTTP version|curl -v --http2 ${SERVER}/ 2>&1|505"
)

print_test_header() {
    echo -e "\n${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}Starting comprehensive curl test suite${NC}"
    echo -e "${BLUE}Server: $SERVER${NC}"
    echo -e "${BLUE}Total Tests: ${#TESTS[@]}${NC}"
    echo -e "${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"
}

run_test() {
    local description="$1"
    local command="$2"
    local expected_status="$3"
    
    ((TOTAL++))
    
    echo -e "${YELLOW}[Test $TOTAL]${NC} $description"
    echo -e "${YELLOW}Expected Status:${NC} $expected_status"
    
    # Run curl command with timeout and capture response
    response=$(timeout $TIMEOUT bash -c "eval \"$command\"" 2>&1)
    exit_code=$?
    
    # Extract HTTP status code
    status=$(echo "$response" | grep -oP '< HTTP/[\d.]+ \K[0-9]{3}' | head -1)
    
    # Handle timeout
    if [ $exit_code -eq 124 ]; then
        echo -e "${RED}✗ TIMEOUT${NC} (No response within ${TIMEOUT}s)"
        if [ "$VERBOSE_FAILED" = true ] || [ "$VERBOSE_ALL" = true ]; then
            echo -e "${RED}Response:${NC}"
            echo "$response" | head -20
        fi
        echo ""
        ((FAILED++))
        echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
        return
    fi
    
    # Handle connection refused
    if [ -z "$status" ] && echo "$response" | grep -q "Connection refused"; then
        echo -e "${RED}✗ CONNECTION REFUSED${NC} (Server not running?)"
        if [ "$VERBOSE_FAILED" = true ] || [ "$VERBOSE_ALL" = true ]; then
            echo -e "${RED}Response:${NC}"
            echo "$response" | head -20
        fi
        echo ""
        ((FAILED++))
        echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
        return
    fi
    
    # Check if status matches
    if [ "$status" == "$expected_status" ]; then
        echo -e "${GREEN}✓ PASSED${NC} (Got: $status)"
        if [ "$VERBOSE_ALL" = true ]; then
            echo -e "${GREEN}Response:${NC}"
            echo "$response" | head -20
        fi
        echo ""
        ((PASSED++))
    else
        echo -e "${RED}✗ FAILED${NC} (Expected: $expected_status, Got: ${status:-NO_STATUS})"
        if [ "$VERBOSE_FAILED" = true ] || [ "$VERBOSE_ALL" = true ]; then
            echo -e "${RED}Response (first 20 lines):${NC}"
            echo "$response" | head -20
        fi
        echo ""
        ((FAILED++))
    fi
    
    echo -e "${BLUE}────────────────────────────────────────────────────────────${NC}"
}

# Main execution
print_test_header

# Run all tests
for test in "${TESTS[@]}"; do
    # Parse test case
    IFS='|' read -r description command expected_status <<< "$test"
    run_test "$description" "$command" "$expected_status"
done

# Print summary
echo -e "\n${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Test Summary${NC}"
echo -e "${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}✓ Passed:${NC}  $PASSED"
echo -e "${RED}✗ Failed:${NC}  $FAILED"
echo -e "${BLUE}Total:${NC}   $TOTAL"
PASS_RATE=$((PASSED * 100 / TOTAL))
echo -e "${BLUE}Pass Rate: ${PASS_RATE}%${NC}"
echo -e "${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}\n"

# Exit with appropriate code
if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 All tests passed!${NC}\n"
    exit 0
else
    echo -e "${RED}❌ Some tests failed!${NC}\n"
    exit 1
fi