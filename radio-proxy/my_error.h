#ifndef __MY_ERROR__
#define __MY_ERROR__
#include <errno.h>
#include <stdio.h>

enum error_codes {
    ALREADY_SET = 1,
    NOT_SET = 2,
    ERRNO_ERROR = -1, ///< Przekazujemy, że kod błędu należy odzyskać z wartośi errno
    GAI_ERROR = -2,
    TCP_CONNECT_FAIL = -3,
    READ_LINE_TOO_EARLY = -4,
    ARGC_WRONG_VALUE = -5,
    RECEIVED_SIGNAL = -6,
    TIMEOUT = -7,
    READ_ZERO = -8,
    REQUEST_UNIMPLEMENTED_HF = -16,
    RECEIVE_SQ_NO_NAME = -32,
    RECEIVE_TE_NOSUPP = -33,
    RECEIVE_SYNTAX_ERR = -34,
    RECEIVE_STATUS_CODE = -35,
    RECEIVE_REASON_PHRASE = -36,
    RECEIVE_NO_HTTP = -37,
    RECEIVE_EMPTY_FIELD = -38,
    RECEIVE_UNSUPPORTED_HEADER = -39,
    UNSUPPORTED_HEADER = -100,
    UNKNOWN_ERROR = -128,
    HANDLED_ERROR = -127,

};

int my_error_set_gai_error(int return_code);
void my_error_print_error(FILE *stream, int error_code);

#endif
