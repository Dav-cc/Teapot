#include <stddef.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include "http_parser.h"
#include "../core/log.h"
#include "../http/server.h"
#include <string.h>
#include <stdlib.h>

int slice_eq_string_ci(http_slice *slice, char *string)
{
    if (slice == NULL || string == NULL || slice->ptr == NULL)
        return -1;

    size_t str_len = strlen(string);

    if (slice->len != str_len)
        return -1;

    for (size_t i = 0; i < slice->len; i++) {
      unsigned char a = (unsigned char)slice->ptr[i];
      unsigned char b = (unsigned char)string[i];

      if (tolower(a) != tolower(b))
        return (int)tolower(a) - (int)tolower(b);
    }

    return 0;
}

int slice_eq_string(http_slice* slice, char* string){
    if (slice == NULL || string == NULL || slice->ptr == NULL)
        return -1;

    size_t str_len = strlen(string);

    if(slice->len != str_len)
        return -1;
    int res = memcmp(slice->ptr, string, str_len);
    return res;
}

char* find_slice(char* src, char* dest, size_t dest_size, size_t len){
    char* d = (char*)memmem(src, len,dest , dest_size);
    if(!d){
        return NULL;
    }
    return d;
}

http_header* header_lookup(http_header* headers, char* name, size_t headers_count){
    for(size_t i = 0; i <headers_count; i++ ){
        if (slice_eq_string_ci(&headers[i].name, name) == 0)
            return &headers[i];
        }
    return NULL;
}

int slice_lookup(http_slice* slice, const char* name){
    int j = strncmp(slice->ptr, name, strlen(name));
    if(j == 0)
        return 0;
    return -1;
}

ssize_t slice_to_ul(http_header* con_len){
    char tmp[32];
    ssize_t u_len;
    if(con_len->value.len >= sizeof(tmp) - 1)
        return -1;

    // char* ptr = memcpy(tmp, con_len->value.ptr, con_len->value.len);

    tmp[con_len->value.len] = '\0';
    u_len = strtoul(tmp, NULL, 10);
    return u_len;
}

ssize_t convert_con_len(http_header *h){
    if (h == NULL)
        return -1;

    return convert_length(h->value.ptr, h->value.len);
}

ssize_t convert_length(char* value, size_t len){
  if (value == NULL || len == 0)
        return -1;

    ssize_t result = 0;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)value[i];

        if (c < '0' || c > '9')
            return -1;

        int digit = c - '0';

        if (result > (SSIZE_MAX - digit) / 10)
            return -1;

        result = result * 10 + digit;
    }

    return result;

}

static int is_header_token_char(unsigned char c)
{
    if ((c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z')) {
        return 1;
    }

    switch (c) {
        case '!': case '#': case '$': case '%': case '&':
        case '\'': case '*': case '+': case '-': case '.':
        case '^': case '_': case '`': case '|': case '~':
            return 1;
        default:
            return 0;
    }
}

static int valid_header_name(const char *name, size_t len)
{
    if (name == NULL || len == 0)
        return 0;

    for (size_t i = 0; i < len; i++) {
        if (!is_header_token_char((unsigned char)name[i]))
            return 0;
    }

    return 1;
}

static int valid_header_value(const char *value, size_t len)
{
    if (value == NULL && len != 0)
        return 0;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)value[i];

        /* HTAB is permitted; other C0 controls and DEL are not. */
        if ((c < 0x20 && c != '\t') || c == 0x7f)
            return 0;
    }

    return 1;
}

int has_duplicate_content_length(http_request *req)
{
    size_t count = 0;
    for (size_t i = 0; i < req->headers_count; i++) {
        if (slice_eq_string_ci(&req->headers[i].name,"Content-Length") == 0) {
            count++;
            if (count > 1)
                return 1;
        }
    }
    return 0;
}

http_parser_result http_parser_parse(void* conn,dbuffer* buf){
    http_parser_error http_err; 
    Connection* c = conn;
    http_request* req = c->request;
    if(!req){
        return PARSER_ERROR;
    }

    switch (req->state) {
        case PARSER_STATE_REQLINE: 
            http_err = http_parser_request_line(req, buf);
                if(http_err.any_err){
                if(http_err.type == ERR_REQUEST_NOT_COMPELETE){
                    return PARSER_NEED_MORE;
                }
                    return PARSER_ERROR;
                }
                /* fall through */
        case PARSER_STATE_HEADERS:
            http_err = http_parser_headers(req, buf);
                if(http_err.any_err){
                if(http_err.type == ERR_REQUEST_NOT_COMPELETE){
                    return PARSER_NEED_MORE;
                }
                    return PARSER_ERROR;
                    }

            if (req->state == PARSER_STATE_DONE)
              return PARSER_OK;
            /* fall through */

        case PARSER_STATE_BODY:
            http_err = http_parser_body(req, buf);
                if(http_err.any_err){
                if(http_err.type == ERR_REQUEST_NOT_COMPELETE){
                    return PARSER_NEED_MORE;
                    }
                    return PARSER_ERROR;
                }
                /* fall through */
        case PARSER_STATE_DONE:
            return PARSER_OK;

        case PARSER_STATE_ERROR:
            log_message(LOG_LEVEL_ERROR,"Error in Parsing Http Request");
            return PARSER_ERROR;

    }
    return PARSER_ERROR;
}


http_parser_error http_parser_request_line(http_request* req,dbuffer* read_buf){
    http_parser_error err = {0};

    if (req == NULL || read_buf == NULL || read_buf->data == NULL) {
      err.any_err = 1;
      err.type = ERR_INVALID_REQUEST_LINE;
      return err;
    }

    if (read_buf->offset > read_buf->len)
      return (http_parser_error){.any_err = 1, .type = ERR_INVALID_REQUEST_LINE};

    char* start = read_buf->data + read_buf->offset;
    size_t available = read_buf->len - read_buf->offset;

    char *check = find_slice(start, "\r\n", 2, available);
    if(!check){
        err.any_err = 1;
        err.type = ERR_REQUEST_NOT_COMPELETE;
        return err;
    }

    size_t line_len = (size_t)(check - start);
    char* space1 = find_slice(start," ", 1, line_len);
    if(!space1){
        err.any_err = 1;
        err.type = ERR_REQUEST_NOT_COMPELETE;
        return err;
    }
    if (memchr(start, '\0', line_len) != NULL) {
    return (http_parser_error){
        .any_err = 1,
        .type = ERR_INVALID_REQUEST_LINE
     };
     }

    size_t remain = line_len - (size_t)(space1 + 1 - start);
    char* space2 = find_slice(space1+ 1, " ", 1, remain);
    if(!space2|| space2 == space1 + 1 || space2 + 1 == check){
        err.any_err = 1;
        err.type = ERR_REQUEST_NOT_COMPELETE;
        return err;
    }
    if (find_slice(space2 + 1, " ", 1,
                   (size_t)(check - (space2 + 1))) != NULL) {
        err.any_err = 1;
        err.type = ERR_INVALID_REQUEST_LINE;
        return err;
    }

    req->method.ptr = start;
    req->method.len = (size_t)(space1 - start);

    req->path.ptr = space1 + 1;
    req->path.len = (size_t)(space2 - (space1 + 1));

    req->version.ptr = space2 + 1;
    req->version.len = (size_t)(check - (space2 + 1));

    req->mtd = HTTP_UNKNOWN;

    if((slice_eq_string(&req->method, "GET")) == 0){
        req->mtd = HTTP_GET;
    }
    if((slice_eq_string(&req->method, "POST")) == 0){
        req->mtd = HTTP_POST;
    }
    if (req->mtd == HTTP_UNKNOWN ||
        (slice_eq_string(&req->version, "HTTP/1.1") != 0 &&
         slice_eq_string(&req->version, "HTTP/1.0") != 0)) {
      err.any_err = 1;
      err.type = ERR_INVALID_REQUEST_LINE;
      return err;
    }

    req->state = PARSER_STATE_HEADERS;
    read_buf->offset += line_len +2;

    return err;
}

http_parser_error http_parser_headers(http_request* req, dbuffer* read_buf){
    http_parser_error err = {0};
    http_header* h;
    size_t i = 0;
    char* h_start = read_buf->data + read_buf->offset;
    char* h_end = find_slice(h_start, "\r\n\r\n", 4, read_buf->len- read_buf->offset);
    if (!h_end) {
        err.any_err = 1;
        err.type = ERR_REQUEST_NOT_COMPELETE;
        return err;
}
    char *cur = read_buf->data + read_buf->offset;
    req->headers_count = 0;
    while (cur < h_end) {
      if (i >= 32) {
        err.any_err = 1;
        err.type = ERR_INVALID_HEADER_COUNT;
        return err;
      }

      size_t remaining = read_buf->len - (size_t)(cur - read_buf->data);

      char *crlf = find_slice(cur, "\r\n", 2, remaining);
      char *col = find_slice(cur, ":", 1, remaining);

      // char *crlf = find_slice(cur, "\r\n", 2, read_buf->len - read_buf->offset);
      // char *crlf = find_slice(cur, "\r\n", 2, read_buf->len - ( cur - read_buf->data));
      // char *col = find_slice(cur, ":", 1, read_buf->len - read_buf->offset);

      if (!crlf || !col || col > crlf){
        log_message(LOG_LEVEL_ERROR, "not valid header formate");
        err.any_err = 1;
        err.type = ERR_INVALID_HEADER_CONTENT;
        return err;
      }

      req->headers[i].name.ptr = cur;
      req->headers[i].name.len = col - cur;

      char *value_start = col + 1;

      while (value_start < crlf &&
             (*value_start == ' ' || *value_start == '\t')) {
        value_start++;
      }

      req->headers[i].value.ptr = value_start;
      req->headers[i].value.len = crlf - value_start;

      if (!valid_header_name(req->headers[i].name.ptr,
                             req->headers[i].name.len) ||
          !valid_header_value(req->headers[i].value.ptr,
                              req->headers[i].value.len)) {
        err.any_err = 1;
        err.type = ERR_INVALID_HEADER_CONTENT;
        return err;
      }

      if(req->headers[i].name.len ==0 ){
          err.any_err = 1;
          err.type = ERR_INVALID_HEADER_CONTENT;
          return err;
      }

      cur = crlf + 2;
      i++;
      req->headers_count++;
    }
    read_buf->offset = (h_end - read_buf->data) + 4;

    req->keep_alive = 1;

    if((h = header_lookup(req->headers, "Connection", req->headers_count)) != NULL){
        if(slice_eq_string_ci(&h->value, "close") == 0)
            req->keep_alive = 0;

    }
    if(has_duplicate_content_length(req)){
        err.any_err =1;
        err.type =ERR_INVALLID_CONTENT_LENGTH;
        return err;
    }

    switch (req->mtd){
        case HTTP_GET: req->state = PARSER_STATE_DONE; break;
        case HTTP_POST: req->state = PARSER_STATE_BODY; break;
        case HTTP_UNKNOWN: req->state = PARSER_STATE_ERROR; break;
    }

    return err;
}

http_parser_error http_parser_body(http_request* req, dbuffer* read_buf){
    http_parser_error err;
    char* start_body = read_buf->data + read_buf->offset;
    ssize_t recived_body = read_buf->len - read_buf->offset;

    http_header *con_len = header_lookup(req->headers, "Content-Length", req->headers_count);
    if (!con_len && convert_con_len(con_len) < 0) {
      log_message(LOG_LEVEL_ERROR, "req is POST but no content length");
        err.any_err = 1;
        err.type = ERR_INVALLID_CONTENT_LENGTH;
        return err;
    }
    ssize_t body_len = convert_con_len(con_len);
    if(body_len == -1){
        err.any_err = 1;
        err.type = ERR_INVALLID_CONTENT_LENGTH;
        return err;
    }

    if(body_len > recived_body){
      err.any_err = 1;
      err.type = ERR_REQUEST_NOT_COMPELETE;
      return err;
    }

    if(body_len < recived_body){
      err.any_err = 1;
      err.type = ERR_INVALLID_CONTENT_LENGTH;
      return err;
    }

    req->body.ptr = start_body;
    req->body.len = recived_body;

    err.any_err = 0;

    read_buf->offset += req->body.len;

    return err;
}

void http_parser_reset(http_request* req){
    req->mtd = HTTP_UNKNOWN;
    req->method.len = 0;
    req->path.len = 0;
    req->version.len = 0;
    req->body.len = 0;
    req->headers_count = 0;
    req->state =PARSER_STATE_REQLINE;
}

void http_request_destroy(http_request* req){
    free(req);
}
