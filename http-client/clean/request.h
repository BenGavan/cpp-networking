#ifndef REQUEST_H
#define REQUEST_H

#include <string>
#include <vector>
#include "url.h"
#include "response.h"

enum method_t {
    GET, POST
};

// class Header {
// public:
//     std::string conentType;
//     method_t method;
//     std::string protocol;
//     int contentLength;  // length of the associated content.  -1 = length of content unknown, >=0 = Number of bytes available to read from the body

//     Header(const method_t method, const URL &url) : method{url.method}, protocol{url.proto} {};

// };

class Request {
public:
    // URL being requested
    URL url;

    // Header
    std::string conentType;
    method_t method;
    std::string protocol;
    int contentLength;  // length of the associated content.  -1 = length of content unknown, >=0 = Number of bytes available to read from the body
    // Header header;

    // Body
    std::vector<int> body; 
    
    Request(const std::string &url_str, const method_t &method) : url{url_str}, method{method} {};

    Response sendRequest();
    std::string toString() const;
    std::string make_header_str(const method_t &method, const std::string path, const std::string &host) const;

    // TODO: move into own object/class for networking
    int connect_to_url(const std::string &host_str, const int &port) const;
    Response read_data_from_socket(const int &socket);
};


#endif //REQUEST_H
