#include "http_parser.h"
#include "../http/server.h"
#include "../core/log.h"
#include <stdlib.h>

http_error err = {
    .any_error = 0,
    .type = NO_ERROR,
};

http_parser_result http_parser_parse(dbuffer* buf){
    http_request* req = calloc(1, sizeof(http_request));
    req = http_parser_request_line(req, buf);
    if(err.any_error){
        log_message(LOG_LEVEL_ERROR, "error in parsing reqline : %s", err.txt);
        return PARSER_ERROR;
    }
    req = http_parser_headers(req, buf);
    if(err.any_error){
        log_message(LOG_LEVEL_ERROR, "error in parsing headers : %s", err.txt);
        return PARSER_ERROR;
    }
    req = http_parser_body(req, buf);
    if(err.any_error){
        log_message(LOG_LEVEL_ERROR, "error in parsing body : %s", err.txt);
        return PARSER_ERROR;
    }
}


http_request* http_parser_request_line(http_request* req,dbuffer* read_buf);

http_request* http_parser_headers(http_request* req, dbuffer* buf);

http_request* http_parser_body(http_request* req, dbuffer* buf);
