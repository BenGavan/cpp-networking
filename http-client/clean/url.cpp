#include "url.h"
#include <vector>
#include <iostream>
#include <sstream>

std::string URL::toString() const {
    std::stringstream ss;
    ss <<  "Raw:       " << raw;
    ss << "\nprotocol: " << proto;
    ss << "\nbase:     " << base;
    ss << "\nport:     " << port;
    ss << "\npath:     " << path << std::endl;
    return ss.str();
}


// Extract the base URL, path, and port number (if providied - deduce from protocol (i.e. http -> 80))
void URL::parseURL(const std::string &url, std::string &proto, std::string &base, std::string &path, int &port) {
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