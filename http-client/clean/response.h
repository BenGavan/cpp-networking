

#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>
#include <map>
// #include "request.h"

enum TransferEncodingType {
    chunked, none
};

struct Response {
    // Request request;

    std::string proto;
    std::string status;
    int status_code = 0;
    std::map<std::string, std::string> headers;
    std::string body;

    bool is_header_complete = false;
    size_t chunk_length = -1;  // -1 = chunk length not specified

    bool did_connect = false;

    // Response(const Request &req) : request{req} {};

    std::string toString() const;
    TransferEncodingType getTransferEncodingType() const;
    int getContentLength() const;
    void parseResponseHeader(const std::string &fresh_bytes, Response &response_so_far, std::string &remaining_bytes);
    bool parseResponseBody(const std::string &fresh_bytes, Response &response_so_far, std::string &remaining_bytes);
    bool parseResponse(const std::string &fresh_bytes, Response &response_so_far, std::string &remaining_bytes);
    std::pair<std::string, std::string> extractPairFromLine(const std::string &line);
    void parseFirstLineOfResponse(const std::string &line, std::string &proto, std::string &status, int &status_code);
};

#endif // RESPONSE_H
