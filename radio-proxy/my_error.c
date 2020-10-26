#include "my_error.h"
#include <string.h>
#include <netdb.h>

int gai_error;

int my_error_set_gai_error(int return_code) {
    gai_error = return_code;
    return 0;
}

void my_error_print_error(FILE *stream, int error_code) {
    switch(error_code) {
    case ERRNO_ERROR: fprintf(stream, "ERROR: %s\n", strerror(errno)); break;
    case GAI_ERROR: fprintf(stream, "ERROR: %s\n", gai_strerror(gai_error)); break;
    case TCP_CONNECT_FAIL: fputs("ERROR: All addresses exhausted, could not connect\n", stream); break;
    case READ_LINE_TOO_EARLY: fputs("ERROR: Connection ended before reading CRLF\n", stream); break;
    case ARGC_WRONG_VALUE: fputs("ERROR: Testhttp_raw accepts exatcly 3 arguments\n", stream); break;
    case REQUEST_UNIMPLEMENTED_HF: fputs("ERROR: HTTP_request - Unimplemented header field\n", stream); break;
    case RECEIVE_SQ_NO_NAME: fputs("ERROR: SetCookie header - empty cookiename\n", stream); break;
    case RECEIVE_TE_NOSUPP: fputs("ERROR: Transfer Encoding - support only for \"chunked\"\n", stream); break;
    case RECEIVE_SYNTAX_ERR: fputs("ERROR: Received message is not written according to standard\n", stream); break;
    case RECEIVE_STATUS_CODE: fputs("ERROR: Status Code - Not 200\n", stream); break;
    case RECEIVE_NO_HTTP: fputs("ERROR: HTTP version - syntax error or no support\n", stream); break;
    case RECEIVE_EMPTY_FIELD: fputs("ERROR: Header field value is empty\n", stream); break;
    case RECEIVE_UNSUPPORTED_HEADER: fputs("ERROR: Header not supported\n", stream); break;
    case UNSUPPORTED_HEADER: fputs("ERROR: Unsupported header?\n", stream); break;
    case ALREADY_SET: fputs("ERROR: Value already set\n", stream); break;
    case NOT_SET: fputs("ERROR: Value not set\n", stream); break;
    default: fputs("ERROR: Something went wrong\n", stream); break;
    }
}
