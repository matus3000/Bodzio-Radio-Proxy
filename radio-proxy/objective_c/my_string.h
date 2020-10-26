/**@file
 * @brief Plik z deklaracjami dla struktur @ref base_string, @ref resizable_string
 */
#ifndef _MY_STRING_H
#define _MY_STRING_H
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Klasa odpowiadająca za implementację napisów. Można używać na istancjach move i clone.
 */
struct base_string;

/**
 * @brief Klasa odpowiadająca za implementację napisów zmiennej długości.
 *        Można używać na istancjach move i clone.
 *        Klasa "dziedziczy" po stuct base_string
 */
struct resizable_string;

/**
 * @brief Funkcja tworzy instację struktury base_string przechowującą
 *         napis @p raw.
 * @param[in] raw Cstring, którego kopia będzie przechowywana w strukturze.
 * @pre @p raw nie jest NULLEM.
 * @return NULL jeśli nie udało się zaalokować pamięci, wpp. wskaźnik na
 *         nowoutworzoną instancję klasy base_string.
 */
struct base_string* base_string_new(char *raw);

/**
 * @brief Funkcja tworzy instację struktury resizable_string przechowującą
 *        kopię napisu @p raw.
 * @param[in] raw Cstring, którego kopia będzie przechowywana w strukturze.
 * @pre @p raw nie jest NULLEM.
 * @return NULL jeśli nie udało się zaalokować pamięci, wpp. wskaźnik na
 *         nowoutworzoną instancję struktury resizable_string.
 */
struct resizable_string* resizable_string_new(char *raw);

/**
 * @brief Funkcja przekazuje nowoutworzoną instację struktury resizable_string.
 *        Napis przechowywany w tej strukturze jest napisem pustym, jednak
 *        liczba komórek pamięci zaalokowana na napisa ma długość minimum
 *        @p init_len.
 *
 * @param[in]  init_len Dolne ograniczenie na długość bufora
 *
 * @return Wskaźnik na nową instancję struktury resizable_string, NULL jeśli
 *         wystąpił błąd alokacji pamięci.
 */
struct resizable_string* resizable_char_buffer(unsigned init_len);

/**
 * @brief Funkcja przekazuje wskaźnik na napis przechowywany w strukturze
 *        base_string.
 * @param[in] _self Wskaźnik na strukturę base_string lub resizable_string.
 *
 * @pre @p _self jest wskaźnikiem przekazanym przez @ref base_string_new,
 *      @ref resizable_string_new, lub @ref resizable_char_buffer.
 *
 * @return NULL jeśli @p _self jest NULLEM, wpp. wskaźnik na przechowywany
 *         napis.
 */
const char*    base_string_raw(void* _self);

/**
 * @brief Funkcja przekazuje wskaźnik na napis przechowywany w strukturze
 *        base_string. I usuwa strukturę za pomocą @ref delete.
 * @param[in] _self Wskaźnik na strukturę base_string lub resizable_string.
 *
 * @pre @p _self jest wskaźnikiem przekazanym przez @ref base_string_new,
 *      @ref resizable_string_new, lub @ref resizable_char_buffer.
 *
 * @return NULL jeśli @p _self jest NULLEM, wpp. wskaźnik na przechowywany
 *         napis.
 */
char*    base_string_raw_destructive(void *_self);

/**
 * @brief Funkcja przekazuje długość napisu przechowywanego w strukturze
 *        base_string.
 *
 * @param[in] _self Wskaźnik na strukturę base_string lub jej podklasę.
 *
 * @pre @p _self jest wskaźnikiem przekazanym przez @ref base_string_new,
 *      @ref resizable_string_new, lub @ref resizable_char_buffer.
 *
 * @return 0 jeśli @p _self jest NULLEM, wpp. wskaźnik na przechowywany
 *         napis.
 */
unsigned base_string_len(void *_self);

/**
 *  @brief Sprawia, że napis podany jako argument odpowiada napisowi pustemu
 *  @param _self Wskaźnik na napis do wyczyszczenia
 */
void     resizable_string_clear(void *_self);

/**
 * @brief Funkcja dopisuje znak do napisu przechowywanego w @p _self.
 *
 * @param[in] _self Wskaźnik na strukturę resizable_string lub jej podklasę.
 * @param[in] c Znak do dopisania.
 *
 * @pre @p _self jest wskaźnikiem przekazanym przez @ref resizable_string_new
 *      lub @ref resizable_char_buffer. @p _self nie jest NULLEM.
 *
 * @return 0 jeśli udało się dopisać znak @p c do napisu, -1 jeśli wystąpił
 *         błąd lub nie udało się zaalokować pamięci.
 */
int resizable_string_append_c(void *_self, char c);

/**
 * @brief Funkcja dopisuje napis @p str do końca napisu przechowywanego
 *        w @p _self.
 *
 * @param[in] _self Wskaźnik na strukturę resizable_string lub jej podklasę.
 * @param[in] str Napis do dopisania.
 * @pre @p _self jest wskaźnikiem przekazanym przez @ref resizable_string_new
 *      lub @ref resizable_char_buffer.
 *
 * @return 0 jeśli operacja przebiegła poprawnie, -1 jeśli wystąpił
 *         błąd lub nie udało się zaalokować pamięci.
 */
int      resizable_string_append_str(void *_self, char *str);

void base_string_delete(void *_self);
#endif
