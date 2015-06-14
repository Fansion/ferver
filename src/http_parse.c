#include "http.h"

int fv_http_parse_request_line(fv_http_request_t *r)
{
    u_char ch, *p, *m;
    enum {
        sw_start = 0,
        sw_method,
        sw_spaces_before_uri,
        sw_after_slash_in_uri,
        sw_http,
        sw_http_H,
        sw_http_HT,
        sw_http_HTT,
        sw_http_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_spaces_after_digit,
        sw_almost_done
    } state;
    state = r->state;
    check(state == 0, "state should be 0");

    for (p = r->pos; p < (u_char *)r->last; ++p)
    {
        ch = *p;
        switch (state)
        {
        case sw_start:  /* HTTP methods: GET, HEAD, POST */
            r->request_start = p;
            if (ch == CR || ch == LF)
                break;
            if ((ch < 'A' || ch > 'Z') && ch != '_')
            {
                return FV_HTTP_PARSE_INVALID_METHOD;
            }
            state = sw_method;
            break;
        case sw_method:
            if (ch == ' ') {
                r->method_end = p;
                m = r->request_start;
                switch (p - m)
                {
                case 3:
                    if (fv_str3_cmp(m, 'G', 'E', 'T', ' '))
                    {
                        r->method = FV_HTTP_GET;
                        break;
                    }
                    break;
                case 4:
                    if (fv_str30cmp(m, 'P', 'O', 'S', 'T'))
                    {
                        r->method = FV_HTTP_POST;
                        break;
                    }
                    if (fv_str4cmp(m, 'H', 'E', 'A', 'D'))
                    {
                        r->method = FV_HTTP_HEAD;
                        break;
                    }
                    break;
                default:
                    r->method = FV_HTTP_UNKNOWN;
                    break;
                }
                state = sw_spaces_before_uri;
                break;
            }
            if ((ch < 'A' || ch > 'Z') && ch != '_')
            {
                return FV_HTTP_PARSE_INVALID_METHOD;
            }
            break;
        case sw_spaces_before_uri:  /* space* before URI */
            if (ch == '/')
            {
                r->uri_start = p;
                state = sw_after_slash_in_uri;
                break;
            }
            switch (ch) {
            case ' ':
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_after_slash_in_uri:
            switch (ch) {
            case ' ':
                r->uri_end = p;
                state = sw_http;
                break;
            default:
                break;
            }
            break;
        case sw_http:
            switch (ch) {
            case ' ':
                break;
            case 'H':
                state = sw_http_H;
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_http_H:
            switch (ch) {
            case 'T':
                state = sw_http_HT;
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_http_HT:
            switch (ch) {
            case 'T':
                state = sw_http_HTT;
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_http_HTT:
            switch (ch) {
            case 'P':
                state = sw_http_HTTP;
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_http_HTTP:
            switch (ch) {
            case '/':
                state = sw_first_major_digit;
                break;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_first_major_digit: /* first digit of major HTTP version */
            if (ch < '1' || ch > '9')
            {
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            r->http_major = ch - '0';
            state = sw_major_digit;
            break;
        case sw_major_digit: /* major HTTP version or not */
            if (ch == '.')
            {
                state = sw_first_minor_digit;
                break;
            }
            if (ch < '0' || ch > '9')
            {
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            r->http_major = r->http_major * 10 + ch - '0';
            break;
        case sw_first_minor_digit:  /* first digit of minor HTTP version */
            if (ch < '0' || ch > '9')
            {
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            r->http_minor = ch - '0';
            state = sw_minor_digit;
            break;
        case sw_minor_digit:
            if (ch == CR) {
                state = sw_almost_done;
                break;
            }
            if (ch == LF)
            {
                goto done;
            }
            if (ch == ' ')
            {
                state = sw_spaces_after_digit;
                break;
            }
            if (ch < '0' || ch > '9')
            {
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            r->http_minor = r->http_minor * 10 + ch - '0';
            break;
        case sw_spaces_after_digit:
            switch (ch) {
            case ' ':
                break;
            case CR:
                state = sw_almost_done;
                break;
            case LF:
                goto done;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;
        case sw_almost_done:
            r->request_end = p - 1;
            switch (ch) {
            case LF:
                goto done;
            default:
                return FV_HTTP_PARSE_INVALID_REQUEST;
            }
        }
    }
    r->pos = p;
    r->state = state;
    return FV_AGAIN;
done:
    r->pos = p + 1;
    if (r->request_end == NULL)
    {
        r->request_end = p;
    }
    r->state = sw_start;
    return FV_OK;
}

int fv_http_parse_request_body(fv_http_request_t *r)
{
    u_char ch, *p;
    enum {
        sw_start = 0,
        sw_key, // deal with line other than request line in the header
        sw_spaces_before_colon,
        sw_spaces_after_colon,
        sw_value,
        sw_cr,
        sw_crlf,
        sw_crlfcr
    } state;
    state = r->state;
    check(state == 0, "state should be 0");
    if (r->pos == r->last)      // empty body return directly
        return FV_OK;

    fv_http_header_t *hd;       // save http header
    for (p = r->pos; p < (u_char *)r->last; ++p)
    {
        ch = *p;
        switch (state)
        {
        case sw_start:
            r->cur_header_key_start = p;
            if (ch == CR || ch == LF) {
                break;
            }
            state = sw_key;
            break;
        case sw_key:
            if (ch == ' ')
            {
                r->cur_header_key_end = p;
                state = sw_spaces_before_colon;
                break;
            }
            if (ch == ':')
            {
                r->cur_header_key_end = p;
                state = sw_spaces_after_colon;
                break;
            }
            break;
        case sw_spaces_before_colon:
            if (ch == ' ') {
                break;
            } else if(ch == ':') {
                state = sw_spaces_after_colon;
                break;
            } else {
                return FV_HTTP_PARSE_INVALID_HEADER;
            }
            break;
        case sw_spaces_after_colon:
            if (ch == ' ')
                break;
            state = sw_value;
            r->cur_header_value_start = p;
            break;
        case sw_value:
            if (ch == CR)
            {
                r->cur_header_value_end = p;
                state = sw_cr;
            }
            if (ch == LF)
            {
                r->cur_header_value_end = p;
                state = sw_crlf;
            }
            break;
        case sw_cr:
            if (ch == LF) {
                state = sw_crlf;
                // save the current http header line
                hd = (fv_http_header_t *)malloc(sizeof(fv_http_header_t));
                hd->key_start    = r->cur_header_key_start;
                hd->key_end      = r->cur_header_key_end;
                hd->value_start  = r->cur_header_value_start;
                hd->value_end    = r->cur_header_value_end;
                list_add(&hd->list, &r->list);
                break;
            } else {
                return FV_HTTP_PARSE_INVALID_HEADER;
            }
            break;
        case sw_crlf:
            if (ch == CR) {
                state = sw_crlfcr;
            } else {
                r->cur_header_key_start = p;
                state = sw_key;     // find more header lines
            }
            break;
        case sw_crlfcr:
            switch (ch)
            {
            case LF:
                goto done;
            default:
                return FV_HTTP_PARSE_INVALID_HEADER;
            }
            break;
        }
    }
    r->pos = p;
    r->state = state;

    return FV_AGAIN;
done:
    r->pos = p+1;
    r->state = sw_start;
    return FV_OK;
}