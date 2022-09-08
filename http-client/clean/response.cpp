#include "response.h"
#include <sstream>

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
void Response::parseResponseHeader(const std::string &fresh_bytes, Response &response_so_far, std::string &remaining_bytes) {
    
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


bool Response::parseResponseBody(const std::string &fresh_bytes, Response &response_so_far, std::string &remaining_bytes) {
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
bool Response::parseResponse(const std::string &fresh_bytes, Response &response_so_far, std::string &remaining_bytes) {
    
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

std::pair<std::string, std::string> Response::extractPairFromLine(const std::string &line) {
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
void Response::parseFirstLineOfResponse(const std::string &line, std::string &proto, std::string &status, int &status_code) {
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
