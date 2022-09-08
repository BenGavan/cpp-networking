#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstdlib>
#include <cstdio>

#include <netdb.h>

// C++ includes
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>

#include <fcntl.h> // for open
#include <unistd.h> // for close

class Header {
    std::map<std::string, std::string> header; // string key-value map 
};

class Request {
    std::string url;
    std::string conentType;
    std::string method;
    std::string protocol;
    std::vector<int> body; 
    std::string hostUrl;
    int contentLength;  // length of the associated content.  -1 = length of content unknown, >=0 = Number of bytes available to read from the body
    Header header;
};

// Extract the base URL, path, and port number (if providied - deduce from protocol (i.e. http -> 80))
void parseURL(const std::string &url, std::string &base, std::string &path, int &port) {
    //  TODO: write a URL parser to extract base, url path, and port if provided
    // http://localhost:8000/
    // http://localhost:8000/home

    // split url into x:///y:n/x
    // split url into substrings split at ':'
    std::vector<std::string> strs;
    strs.push_back("");
    int n = 0;
    for (int i=0; i<url.length(); i++) {
        if (url[i] == ':') {
            strs.push_back("");
            n++;
            continue;
        }
        strs[n] += url[i];
    }

    base = "";
    path = "";
    port = -1;
    std::string proto;
    std::string port_str;
    switch (strs.size()) {
    case 1:  // just base and path provided (e.g. example.com/path)
        for (int i=0; i<strs[0].length(); i++) {
            if (strs[0][i] == '/') {
                path = strs[0].substr(i, strs[0].length());
                break;
            }
            base += strs[0][i];
        }
        break;
    case 2: 
        if (strs[1].length() < 1) {  // TODO: Come back to
            path = strs[1].substr(0, strs[1].length());
            break;
        }
        if (strs[1][0] == '/' && strs[1][1] == '/') {  // http://example.com/path
            proto = strs[0];
            for (int i=0; i<strs[1].length(); i++) {
                if (strs[1][i] == '/') {
                    if (base.length() == 0) {
                        continue;
                    } else {
                        path = strs[1].substr(i, strs[1].length());
                        break;
                    }
                } 
                base += strs[1][i];
            }
        } else {  // example:80/path
            base = strs[0];
            for (int i=0; i<strs[1].length(); i++) {
                if (strs[1][i] == '/') {
                    port = std::stoi(port_str);
                    path = strs[1].substr(i, strs[1].length());
                    break;
                }
                port_str += strs[1][i];
            }
        }
        break;
    case 3:
        proto = strs[0];
        base = "";
        port = -1;
        path = "";
        for (int i=0; i<strs[1].length(); i++) {
            if (base.length() == 0 && strs[1][i] == '/') {
                continue;
            }
            base += strs[1][i];
        }
        for (int i=0; i<strs[2].length(); i++) {
            if (strs[2][i] == '/') {
                path = strs[2].substr(i, strs[2].length());
                break;
            }
            port_str += strs[2][i];
        }       
        port = std::stoi(port_str);
        break;
    
    default:
        std::cout << "url not supported" << std::endl;
        return;
        break;
    }

    if (port == -1) {
        if (proto == "http") {
            port = 80;
        } else if (proto == "https") {
            port = 443;
        } else {
            port = 80;
        }
    }
}

std::string make_header_str(const std::string &method, const std::string path, const std::string &host) {
    std::stringstream ss;
    ss << method;
    ss << " " << path;
    ss << " HTTP/1.1";
    ss << "\r\nHost: " << host;
    ss << "\r\n\r\n";
    return ss.str();
}

int connect_to_url(const std::string &host_str, const int &port) {
    struct sockaddr_in sa;
    struct hostent* he;

    int s = socket(PF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return -1;
    }

    bzero(&sa, sizeof sa);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);  // port on the server we are trying to connect to 

    const char* host = host_str.c_str();
    he = gethostbyname(host);
    if (he == NULL) {
        std::cout << "Invalid host name" << std::endl;
        herror(host);
        return -2;
    }

    bcopy(he->h_addr_list[0], &sa.sin_addr, he->h_length);

    if (connect(s, (struct sockaddr *) &sa, sizeof sa) < 0) {
        perror("connect");
        close(s);
        return -2;
    }
    return s;
}

std::pair<std::string, std::string> extractPairFromLine(const std::string &line) {
    std::pair<std::string, std::string> pair;
    bool is_first = true;
    for (int i=0; i<line.length(); i++) {
        if (line[i]==':') {  // TODO: need to handle keys like ":authority:"
            is_first = false;  // end of the key (first) now extracting the content (second)
            i++;  // ':' is always followed by a space -> can ignore
            continue;
        }
        if (is_first) {
            pair.first += line[i];
        } else {
            pair.second += line[i];
        }
    }
    return pair;
}

// Given the first line of the response, exctract the protocol, status, and status code
void parseFirstLineOfResponse(const std::string &line, std::string &proto, std::string &status, int &status_code) {
    proto = "";
    status = "";
    std::string code_str = "";
    
    // extract protocol
    int i = 0;
    for (; i<line.length(); i++) {
        if (line[i] == ' ') {
            i++; 
            break;
        }
        proto += line[i];
    }

    // extract status
    for (; i<line.length(); i++) {
        status += line[i];
    }

    // extract status code from status
    for (i=0; i<status.length(); i++) {
        if (status[i] == ' ') {
            break;
        }
        code_str += status[i];
    }

    status_code = std::stoi(code_str);
}

enum TransferEncodingType {
    chunked, none
};

struct Response {
    std::string proto;
    std::string status;
    int status_code = 0;
    std::map<std::string, std::string> headers;
    std::string body;

    bool is_header_complete = false;
    size_t chunk_length = -1;  // -1 = chunk length not specified

    std::string toString() const;
    TransferEncodingType getTransferEncodingType() const;
    int getContentLength() const;
};

std::string Response::toString() const {
    std::stringstream ss;
    ss << "Response:\n";
    ss <<   "resp status        = " << this->status;
    ss << "\nresp status code   = " << this->status_code;
    ss << "\nresp proto         = " << proto;
    ss << "\nis_header_complete = " << is_header_complete;
    for (auto iter=headers.begin(); iter!=headers.end(); ++iter) {
        ss << "\n|" << iter->first << "| = |" << iter->second << "|";
    }
    ss << "\nBody:\n";
    ss << "--------------------\n";
    ss << body;
    ss << "\n--------------------\n";
    ss << std::flush;
    return ss.str();
}

TransferEncodingType Response::getTransferEncodingType() const {
    std::map<std::string, std::string>::const_iterator it = headers.find("Transfer-Encoding");
    if (it == headers.end()) {
        return none;
    } else {
        if (it->second == "chunked") {
            return chunked;
        } else {
            return none;
        }
    }
}

int Response::getContentLength() const {
    std::map<std::string, std::string>::const_iterator it = headers.find("Content-Length");
    if (it == headers.end()) {  // not found
        return -1;
    } else {
        return std::stoi(it->second);  // TODO: error check (for failed string to int conversion)
    }
    return -1;
}

// Build the current complete parts of the header given the:
//  - newly read bytes from buffer (as std::string)
//  - response so far
//  - left over bytes that could not be parse from the previous attempt (as std::string)
// given back the remaining bytes (as std::string) that could not be parsed due to:
//  - the buffer cut a header pair in half, or the header is finished
//  - or, the remaining bytes are actually the body
void parseResponseHeader(const std::string &fresh_bytes, Response &response_so_far, std::string &remaining_bytes) {
    
    // combine remaining bytes from last time with the newly read bytes 
    const std::string combined_str = remaining_bytes + fresh_bytes;
    std::string current_line = "";

    for (int i=0; i<combined_str.length(); i++) {
        current_line += combined_str[i];
        size_t cl_len = current_line.length();

        if (current_line[cl_len-2] == '\r' && current_line[cl_len-1] == '\n') {
            if (current_line.length() == 2) {  // empty line = "\r\n"
                // found end of header, i.e empty line at end of Header
                remaining_bytes = ""; 
                if (i+1 < combined_str.length()) { // check safety of taking substring (+1 since if i = len: remaining="")
                    remaining_bytes = combined_str.substr(i+1, combined_str.length());  
                }
                
                response_so_far.is_header_complete = true;
                return;  
            }

            std::string trimmed_current_line = current_line.substr(0, current_line.length()-2);

            if (response_so_far.headers.size() == 0 && response_so_far.status_code == 0) {  // parse first header line
                parseFirstLineOfResponse(trimmed_current_line, response_so_far.proto, response_so_far.status, response_so_far.status_code);
                current_line =  "";
                continue;
            }

            response_so_far.headers.insert(extractPairFromLine(trimmed_current_line));

            current_line = "";
            continue;
        }
        
    }
    remaining_bytes = current_line;
}

bool parseResponseBody(const std::string &fresh_bytes, Response &response_so_far, std::string &remaining_bytes) {
    // combine remaining bytes from last time with the newly read bytes 
    const std::string combined_str = remaining_bytes + fresh_bytes;

    // string of the parsed bytes so far (combined_str - current_line = remaining_bytes)
    std::string current_line = "";

    for (int i=0; i<combined_str.length(); i++) {
        current_line += combined_str[i];
        size_t cl_len = current_line.length();

        if (response_so_far.getContentLength() > 0) {  
            if (cl_len == response_so_far.getContentLength()) {
                response_so_far.body = current_line;
                current_line = "";
                remaining_bytes = "";  
                if (i+1 < combined_str.length()) {
                    remaining_bytes = combined_str.substr(i+1, combined_str.length());
                }
                return true;
            }

        } else if (response_so_far.getTransferEncodingType() == chunked) {
            if (response_so_far.chunk_length == -1 && cl_len > 2) {  // check chunk length is specified
                if (current_line[cl_len-2] == '\r' && current_line[cl_len-1] == '\n') {
                    response_so_far.chunk_length = std::stoi(current_line.substr(0, cl_len-2), 0, 16);
                    current_line = "";
                    
                    if (response_so_far.chunk_length == 0) {  // EOF
                        remaining_bytes = "";  
                        if (i+1 < combined_str.length()) {
                            remaining_bytes = combined_str.substr(i+1, combined_str.length());
                        }
                        return true;
                    }
                    continue;
                }        
            }

            if (current_line.length() == response_so_far.chunk_length) {
                response_so_far.body += current_line;
                current_line = "";
                response_so_far.chunk_length = -1;
                continue;
            }
        }   
    }
    remaining_bytes = current_line;
    return false;
}


// Build part of the header or body given the:
//  - newly read bytes (as std::string)
//  - header so far (as the actual std::map<string, string>)
//  - left over bytes that could not be parse from the previous attempt
// given back the remaining bytes (as std::string) that could not be parsed due to:
//  - the buffer cut a header pair in half, or the header is finished
//  - or, the remaining bytes are actually the body
// return whether end of response has been reached (true = reached end of response)
bool parseResponse(const std::string &fresh_bytes, Response &response_so_far, std::string &remaining_bytes) {
    
    if (response_so_far.is_header_complete) {
        return parseResponseBody(fresh_bytes, response_so_far, remaining_bytes);
    } else {
        parseResponseHeader(fresh_bytes, response_so_far, remaining_bytes);
        if (response_so_far.is_header_complete) {
            return parseResponseBody("", response_so_far, remaining_bytes);
        }
    }

    return false;
}

Response read_data_from_socket(const int &socket) {
    int nbytes_total = 0;
    char buffer[BUFSIZ];

    Response response;
    std::string remaining_bytes;
    bool is_end_of_response = false;

    while (!is_end_of_response) {
        
        nbytes_total = recv(socket, buffer, sizeof(buffer),0);

        // parse response
        std::string h_str(buffer, nbytes_total);
        is_end_of_response = parseResponse(h_str, response, remaining_bytes);
    }

    std::cout << "Finished\n";
    return response;
}

Response makeRequest(const std::string &method, const std::string &url) {
    std::string baseURL;  // host/host_url
    std::string path;
    int port;

    parseURL(url, baseURL, path, port);

    // connect
    int socket = connect_to_url(baseURL, port);
    if (socket < 0) {
        perror("failed to connect");
        return Response();
    }

    // send data
    std::string header_str = make_header_str(method, path, baseURL);
    const char* header_chars = header_str.c_str();
    send(socket, header_chars, strlen(header_chars), 0);

    // recieve data
    Response response = read_data_from_socket(socket);
    return response;
}

void substrtest() {
    std::string str = "ABC";
    for (int i=0; i<10; i++) {
        if (i > str.length()) {
            std::cout << "i = " << i << ", str.len = " << str.length() << std::endl;
            return;
        }
        std::string sub_str = str.substr(i, str.length());
        std::cout << '|' << str << "|\n";
        std::cout << '|' << sub_str << '|' << std::endl;
    }
}

void parseurltest() {
    // std::string url = "http://localhost:8000/home";
    // std::string url = "http://localhost.com:8000/home/here/now";

    // std::string url = "http://localhost.com/home/here/now";
    // std::string url = "localhost:80/home/here/now";

    // std::string url = "localhost.com/home/here/now";

    std::string url = "http://google.co.uk/";

    std::string base;
    std::string path;
    int port;

    parseURL(url, base, path, port);

    std::cout << "url:  " << url;
    std::cout << "\nbase: " << base;
    std::cout << "\npath: " << path;
    std::cout << "\nport: " << port << std::endl;
}

int main() {
    std::cout << "A simple HTTP Client" << std::endl;

    std::string method = "GET";
    // std::string url = "https://www.rapidtables.com/convert/number/hex-to-decimal.html";
    std::string url = "http://localhost:8000/";
    Response resp = makeRequest(method, url);
    std::cout << "*****************\n" << resp.toString() << "\n*****************\n" << std::endl;

    return 0;
}