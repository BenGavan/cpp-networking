#include <string>

#ifndef URL_H
#define URL_H

struct URL {
    std::string raw;
    std::string proto;
    std::string base; 
    std::string path; 
    int port;

    URL(const std::string &raw) : raw{raw} { parseURL(raw, proto, base, path, port); };  // parsed and constructs a raw url string
    std::string toString() const;
private:
    void parseURL(const std::string &url, std::string &proto, std::string &base, std::string &path, int &port);
};

#endif //URL_H
