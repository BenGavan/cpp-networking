#include <iostream>
#include <string>
#include "request.h"


int main() {
    std::cout << "Example clean version of http-client" << std::endl;

    std::string url_str = "localhost:8000/";
    Request req = Request(url_str, GET);
    std::cout << req.toString() << std::endl;

    Response resp = req.sendRequest();
    if (!resp.did_connect) {  
        std::cout << "Failed to connect to: " << req.url.raw << std::endl;
        return 0;
    }
    std::cout << resp.toString() << std::endl;

    return 0;
}