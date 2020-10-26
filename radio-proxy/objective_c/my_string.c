/**@file
 * @brief Implementacjia pliku nagłóœkowego my_string.h
 */
#include "my_string.h"
#include "../my_error.h"
#include "class.h"
#include <string.h>
#include <stdlib.h>
#define MINIMAL_SIZE 16///< Minimalna długość alokowanego napisu.

/**
 * @brief Podstawowa klasa do napisów. Napisy są stałego rozmiaru.
 */
struct base_string {
    const struct base_class * class;///< Wskaźnik na klasę
    char *raw;///< Włąściwy c-string, zakończony /0.
    unsigned size;///< Liczba bajtów zaalokowana na potrzeby raw.
};

/**
 * @brief Napis zmiennego rozmiaru.
 */
struct resizable_string {
    struct base_string super;///< Nadklasa
    unsigned len;///< Długość napisu bez znaku końca linii.
};

static void* base_string_ctor(void *_self, va_list * args);
static void* base_string_dtor(void* _self);
static void* base_string_clone(void *_self, void *copied);
static void* base_string_move(void *_self, void* moved);
static void* resizable_string_ctor(void *_self, va_list *ap);
static void* resizable_string_move(void *_self, void* _moved);
static void* resizable_string_clone(void *_self, void* _cloned);

/// Klasa dla base_string
static struct base_class BaseString = {
    sizeof(struct base_string),
    base_string_ctor, base_string_dtor,
    base_string_clone, base_string_move,
};

/// Klasa dla resizable_string
static struct base_class ResizableString = {
    sizeof(struct resizable_string),
    resizable_string_ctor, base_string_dtor,
    resizable_string_clone, resizable_string_move
};

/**
 * @brief Funkcja przekazuje długość napisu przechowywanego w instancji base_string
 * @param[in] self Wskaźnik na instację struktury base_string.
 * @pre @p self został otrzymany na skutek wywołania New z parametrem &BaseString i
 *      napisem, który miał być przechowany. Nie jest on NULLEM.
 * @return Długość napisu.
 */
static unsigned __base_string_len(struct base_string *self) {
    if (self->size == MINIMAL_SIZE) {
        return strlen(base_string_raw(self));
    }
    else {
        return self->size - 1;
    }
}

/**
 * @brief Konstruktor instacji struktury base_string.
 * @param[in] _self
 * @param[in] args
 * @warning Funkcja powinna być wywoływana jedynie przez funkcję new.
 * @pre _self nie jest NULLEM. @p args ma jako pierwszy element c-stringa.
 * @return _self jeśli inicjalizacjia się udała; NULL wpp.
 */
static void* base_string_ctor(void *_self, va_list * args){
    struct base_string *self = _self;
    const char *text = va_arg(*args, char*);

    unsigned size = strlen(text) + 1;
    if (size < MINIMAL_SIZE)
        size = MINIMAL_SIZE;
    self->raw = malloc(size * sizeof(char));
    if (self->raw) {
        self->size = size;
        strcpy(self->raw, text);
    } else {
        return NULL;
    }

    return self;
}

/**
 * @brief Destruktor struktury base_string. Usuwa pamięć
 *        zaalokowaną na potrzeby tworzenia napisu.
 * @param[in] _self Wskaźnik na strukturę do usunięcia.
 * @return _self;
 */
static void* base_string_dtor(void* _self){
    struct base_string *self = _self;

    free(self->raw);

    return self;
}

/**
 * @brief "Konstruktor" kopiujący.
 * @param[in] _self Wskaźnik na pamięć do, której kopiujemy obiekt.
 * @param[in] _cloned Obiekt kopiowany.
 * @return _self jeśli sukces, NULL wpp.
 */
static void *base_string_clone(void *_self, void *_cloned) {
    struct base_string *self = _self, *cloned = _cloned;
    char *tmp = malloc(cloned->size);

    if (tmp != NULL) {
        self->raw = tmp;
        strcpy(tmp, cloned->raw);
        self->size = cloned->size;
    } else {
        self = NULL;
    }

    return self;
}

/**
 * @brief "Konsktruktor" przenoszący.
 * @param[in] _self Wskaźnik na pamięć do, której przenosimy rekord.
 * @param[in] _moved Obiekt przenoszony
 * @return _self jeśli sukces, NULL wpp.
 */
static void* base_string_move(void *_self, void* _moved) {
    struct base_string *self = _self, *moved = _moved;

    self->raw = moved->raw;
    self->size = moved->size;
    moved->raw = NULL;

    return self;
}

/**
 * @brief "Konsktruktor" struktury resizable_string.
 * @param[in] _self Wskaźnik na pamięć zaalokaną na nową instancję struktury.
 * @param[in] ap va_list zawierająca wskaźnik na napis.
 * @return _self jeśli sukces, NULL wpp.
 */
static void *resizable_string_ctor(void *_self, va_list *ap){
    struct resizable_string *self = _self;

    self = base_string_ctor(self, ap);
    if (self)
        self->len = __base_string_len((struct base_string*) self);

    return self;
}

/**
 * @brief "Konsktruktor" kopiujący.
 * @param[in] _self Wskaźnik na pamięć do, której kopiujemy obiekt.
 * @param[in] _cloned Obiekt kopiowany.
 * @return _self jeśli sukces, NULL wpp.
 */
static void* resizable_string_clone(void *_self, void* _cloned) {
    struct resizable_string *self = BaseString.clone(_self, _cloned);

    if (self) {
        struct resizable_string *cloned = _cloned;
        self->len = cloned->len;
    }

    return self;
}

/**
 * @brief Move konsktruktor
 * @param[in] _self Wskaźnik na pamięć do, której przenosimy rekord.
 * @param[in] _moved Obiekt przenoszony
 * @return _self jeśli sukces, NULL wpp.
 */
static void* resizable_string_move(void *_self, void* _moved) {
    struct resizable_string *moved = _moved;

    struct resizable_string *self = BaseString.move(_self, _moved);

    if (self) {
        self->len = moved->len;
    }

    return self;
}

/**
 * @brief Funkjca pomocniczna służąca do zmiany liczby zaalokowanych
 *        bajtów na napis w strukturze base_string.
 * @param[in] _self Wskaźnik na instację struktury base_class lub jej
 *                  podklasy.
 * @param[in] new_len Nowa długość.
 * @return 0 jeśli udało się zmienić liczbę zaalokwanych bajtów na @p new_len,
 *         -1 wpp.
 */
static int base_string_resize(void *_self, size_t new_len){
    int result = 0;
    struct base_string *self = _self;

    char *tmp = realloc(self->raw, new_len * sizeof(char));
    if (tmp) {
        self->raw = tmp;
        self->size = new_len * sizeof(char);
    }
    else {
        result = ERRNO_ERROR;
    }

    return result;
}

struct base_string* base_string_new(char *raw) {
    return new(&BaseString, raw);
}

struct resizable_string* resizable_string_new(char *raw) {
    return new(&ResizableString, raw);
}

struct resizable_string* resizable_char_buffer(unsigned init_len) {
    char tmp[init_len + 1];

    for (unsigned i = 0; i < init_len; ++i)
        tmp[i] = 'B';
    tmp[init_len] = '\0';

    struct resizable_string *result = resizable_string_new(tmp);

    if (result)
        resizable_string_clear(result);

    return result;
}

const char *base_string_raw(void* _self) {
    struct base_string *self = _self;

    return self->raw;
}

unsigned base_string_len(void *_self) {
    struct base_class *class = *((struct base_class **) _self);

    if (class == &BaseString) {
        struct base_string *self = _self;
        return __base_string_len(self);
    }
    else {
        struct resizable_string *self = _self;

        return self->len;
    }
}

void resizable_string_clear(void *_self) {
    struct resizable_string *self = _self;
    self->len = 0;
    self->super.raw[0] = '\0';
}

int resizable_string_append_c(void *_self, char c)
{
    int result = 0;
    struct resizable_string *self = _self;

    if (self->len >= self->super.size - 1  && c != '\0')
        result = base_string_resize(self, (self->super.size > 0) ? self->super.size * 2 : 10);

    if (result == 0) {
        self->super.raw[self->len++] = c;
        self->super.raw[self->len] = '\0';
    }

    return result;
}

int resizable_string_append_str(void *_self, char *str){
    int result = 0;
    struct resizable_string *self = _self;
    size_t old_len = self->len;

    for (int i = 0; str[i] != '\0'
             && (result = resizable_string_append_c(self, str[i])) == 0; ++i);

    if (result != 0) {
        self->len = old_len;
        self->super.raw[old_len] = '\0';
    }

    return result;
}


char* base_string_raw_destructive(void *_self) {
    struct base_string *self = _self;
    char *result = self->raw;

    self->raw = NULL;
    self->size = 0;
    delete(_self);

    return result;
}

void base_string_delete(void *self) {
    delete(self);
}
