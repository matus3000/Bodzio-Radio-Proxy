#include "http.h"
#include "shout_response.h"
#include "objective_c/my_string.h"
#include "my_error.h"
#include "net_utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 *
 * @return 0 jeśli podany napis jest interpretowany jako liczba nieujemna,
 *         RECEIVE_SYNTAX_ERR w przeciwnym przypadku.
 */
int fvalue_parse_metaint(char *field_value, size_t *metaint) {
    /* if (content_lenght_wrong(field_value)) return RECEIVE_SYNTAX_ERR; */

    if (sscanf(field_value, "%zu", metaint) != 1) {
        return RECEIVE_SYNTAX_ERR;
    }

    return 0;
}

int receive_SP(struct resizable_string *line, size_t *position) {
    if (base_string_raw(line)[*position] != ' ') {
        return RECEIVE_SYNTAX_ERR;
    } else {
        ++*position;
        return 0;
    }
}


/**
 * @brief Funkcja odbiera z napisu @p line znaki CR i LF.
 * @param[in] line Wskaźnik na napis, z którego mają być pobrane znaki
 * @param[in,out] position Pozycja, na której ma znajdować się znak CR.
 *                         Jeśli uda się pobrać znaki CR i LF zapisana
 *                         zostanie pod adresem następna nieprzeczytana
 *                         pozycja.
 * @return 0, jeśli udało się pobrać znaki; RECEIVE_SYTNAX_ERR w przeciwnym przypadku.
 */
int receive_CRLF(struct resizable_string *line, size_t *position) {
    const char *tmp = base_string_raw(line);

    if (tmp[*position] == '\r' && tmp[*position + 1] == '\n') {
        *position += 2;
        return 0;
    } else {
        return RECEIVE_SYNTAX_ERR;
    }
}


/**
 * @brief Funkcja pobiera znak zdefiniowane w standardzie HTTP jako OWS.
 * @param[in] buff Bufor, w którym ma znajdować się znak.
 * @param[in, out] position Pozycja pierwszego nieprzeczytanego znaku. Jeżeli
 *                     pod tą pozycją w buforze znajduje się OWS, zapisana
 *                     zostaje pod tym adresem pozycja o 1 większa.
 */
void receive_OWS(struct resizable_string *buff, size_t *position) {
    char c = base_string_raw(buff)[*position];

    if (c == ' ' || c == '\t') {
        ++*position;
    }
}


int receive_colon(struct resizable_string *buff, size_t *position) {
    if (base_string_raw(buff)[*position] == ':') {
        ++(*position);
        return 0;
    } else {
        return RECEIVE_SYNTAX_ERR;
    }
}

int receive_HTTP_version(struct resizable_string *status_line, size_t *position) {
    int result = 0;
    const char *line = (base_string_raw(status_line));
    char http_version[] = "HTTP/1.";
    char icy[] = "ICY";

    if (strncmp(http_version, &line[*position], 7) == 0 && isdigit(line[*position + 7])) {
        *position += 8;
    } else if (strncmp(icy, &line[*position], sizeof(icy) - 1) == 0) {
        *position += 3;
    } else {
        result = RECEIVE_NO_HTTP;
    }

    return result;
}


/**
 * @brief Funkcja pobiera kod statusu z napisu @p line od pozycji @p *position.
 *        Jeśli kod statusu jest inny niż "200" zwraca -1;
 *
 * @return 0, jeśli kod statusu wynosi 200
 */
int receive_status_code(struct resizable_string *line, struct HTTP_response_message *response,
                        size_t *position) {
    (void) response;
    size_t i = *position;
    const char *str = &base_string_raw(line)[i];


    if (strncmp("200", str, 3) == 0) {
        *position += 3;
        return 0;
    } else {
        return RECEIVE_REASON_PHRASE;
    }
}


/**
 * @brief Funkcja pobiera reason_phares z napisu @p line od pozycji @p *position.
 *        Jeśli ma on wartość inną niż "OK", zwracane jest nie 0.
 *
 * @return 0, jeśli pobrano "OK", -1 wpp.
 */
int receive_reason_phrase(struct resizable_string *line,
                          size_t *position) {
    size_t reason_begin = *position;
    int return_code;

    char tmp1[3];
    char tmp2[2];
    if (sscanf(&base_string_raw(line)[reason_begin], "%2s%1s", tmp1, tmp2) != 1) {
       return_code = RECEIVE_STATUS_CODE;
       return return_code;
    }

    if (strcmp("OK", tmp1) == 0) {
        *position = base_string_len(line) - 2;
        return 0;
    } else {
        return RECEIVE_REASON_PHRASE;
    }
}


int receive_status_line(struct my_stream *stream, struct HTTP_response_message *response,
                        struct resizable_string *buff) {
    int return_code = 0;
    resizable_string_clear(buff);
    return_code = read_line(stream, buff);


    if (return_code <= 0) return (return_code == 0) ? READ_ZERO : return_code;

    size_t i = 0;
    if ((return_code = receive_HTTP_version(buff, &i)) != 0) {
        goto clean;
    }
    if (receive_SP(buff, &i) != 0) {
        return_code = RECEIVE_NO_HTTP;
        goto clean;
    }
    if ((return_code = receive_status_code(buff, response, &i)) != 0) {
        goto clean;
    }
    if ((return_code = receive_SP(buff, &i)) != 0) {
        goto clean;
    }
    if ((return_code = receive_reason_phrase(buff, &i)) != 0) {
        goto clean;
    }
    if ((return_code = receive_CRLF(buff, &i)) != 0) {
        goto clean;
    } if (base_string_len(buff) != i) {
        return_code = RECEIVE_SYNTAX_ERR;
        goto clean;
    }

clean:
    return return_code;
}


enum header_field receive_field_name(struct resizable_string *line, size_t *position) {
    size_t i = 0;
    char *tmp = (char *) base_string_raw(line);

    while (is_tchar(tmp[i])) {
        tmp[i] = toupper(tmp[i]);
        ++i;
    }

    *position = i;

    if (strncmp(tmp, get_header_field_name(ICY_METAINT),i) == 0) {
        return ICY_METAINT;
    } else if (strncmp(tmp, get_header_field_name(ICY_NAME), i) == 0) {
        return ICY_NAME;
    } else {
        return UNKNOWN;
    }
}



int save_field_value_icy_metaint(char *field_value,
                                    struct HTTP_response_message *response) {
    size_t metaint;
    int return_code;

    if ((return_code = fvalue_parse_metaint(field_value, &metaint)) != 0) {
        return return_code;
    } else {
        return HTTP_response_set_icy_metaint(response, metaint);
    }
}

int save_field_value_icy_name(char *field_value,
                              struct HTTP_response_message *response) {
    return HTTP_response_set_icy_name(response, field_value);
}

int save_field_value(enum header_field field_name, char * field_value,
                     struct HTTP_response_message *response) {
    if (field_name == ICY_NAME) {
        return save_field_value_icy_name(field_value, response);
    } else if (field_name == ICY_METAINT) {
        return save_field_value_icy_metaint(field_value, response);
    }

    return RECEIVE_UNSUPPORTED_HEADER;
}

int receive_field_value(enum header_field field_name,
                        struct resizable_string *buff,
                        struct HTTP_response_message *response,
                        size_t *position) {
    int return_code = 0;
    size_t i = *position;
    const char *tmp = base_string_raw(buff);

    while (is_vchar(tmp[i])){
        ++i;
        size_t j = i;
        while (tmp[j] == ' ' || tmp[j] == '\t') {
            ++j;
        }
        if (i != j && is_vchar(tmp[j])) {
            i = j;
        }
    };

    if (i - *position > 0) {
        size_t len = i - *position;
        char * field_value = malloc((len + 1 > 16) ? len + 1 : 16);
        if (field_value) {
            strncpy(field_value, &tmp[*position], len);
            field_value[len] = 0;
            *position = i;
            return_code = save_field_value(field_name, field_value, response);
            free(field_value);
        } else {
            return_code = ERRNO_ERROR;
        }
        return return_code;
    } else {
        return RECEIVE_EMPTY_FIELD;
    }
}



int receive_header_fields_CRLF(struct my_stream *stream,
                               struct HTTP_response_message *response,
                               struct resizable_string *buff) {
    int return_code = 0;
    resizable_string_clear(buff);

    while ((return_code = read_line(stream, buff)) > 0 &&
           base_string_len(buff) > 2) {
        size_t i = 0;
        enum header_field field_name;

        if ((field_name = receive_field_name(buff, &i)) == WRONG) {
            return RECEIVE_UNSUPPORTED_HEADER;
        } else if (field_name == UNKNOWN) {
            resizable_string_clear(buff);
            continue;
        }

        if ((return_code = receive_colon(buff, &i))) return return_code;
        receive_OWS(buff, &i);
        if ((return_code = receive_field_value(field_name, buff, response, &i))) return return_code;
        receive_OWS(buff, &i);
        if ((return_code = receive_CRLF(buff, &i))) return return_code;

        resizable_string_clear(buff);
    }

    return 0;
}

int receive_HTTP_response_message(struct my_stream *stream, struct HTTP_response_message *response) {
    int return_code = 0;
    /* if (set_syntax_checker()) return -1; */
    struct resizable_string *buff = resizable_char_buffer(20);

    if ((return_code = receive_status_line(stream, response, buff)) != 0) {
        goto clean;
    }
    if ((return_code = receive_header_fields_CRLF(stream, response, buff)) != 0) {
        goto clean;
    }

clean:
    /* delete_syntax_checker(); */
    base_string_delete(buff);
    return return_code;
}
