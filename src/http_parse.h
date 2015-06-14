#ifndef __HTTP_PARSE_H__
#define __HTTP_PARSE_H__

#define CR '\r'
#define LF '\n'
#define CRLFCRLF "\r\n\r\n"

int fv_http_parse_request_line(fv_http_request_t *r);
int fv_http_parse_request_body(fv_http_request_t *r);

#endif
