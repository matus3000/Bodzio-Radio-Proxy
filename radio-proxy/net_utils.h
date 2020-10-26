#ifndef __NET_UTILS__
#define __NET_UTILS__
#include <stddef.h>
#include <inttypes.h>
#include "objective_c/my_string.h"
#include <sys/time.h>

struct my_stream {
    int fd;///< Deskryptor z którego czytamy.
    int signalfd;///< Deskryptor na sygnał.
    char buff[256];///< Bufor.
    int16_t read_cnt;///< Ile znaków pozostało do przeczytania.
    int16_t position;///< Pozycja ostatniego przeczytanego znaku w buforze.
    bool eot;///< Czy read zwrócił 0.
    struct timeval *time;///< Maksymalny czas oczekiwania funkcji read.
};

int tcp_connect(char *host, char *service, int sigfd);

/**
 * Funkcja opakowuje deskryptor @p fd w strukturę my_stream.
 * @param[in] fd Deskryptor na którym będą wykonywane operacje czytania.
 *               Liczba nieujemna
 * @param[in] signalfd Deskryptor otrzymany przez wywołanie funkcji signalfd.
 *                     Jeżeli będzie gotowy do odczytu, to funkcja czytania zwróci błąd
 *                     ESIGNAL. Wartość ujemna sprawia, że signalfd jest pomijany.
 * @param[in] time Wskaźnik na sturkturę timeval. Jeśli NULL to oczekiwanie na read jest
 *                 zwykłym blokującym oczekiwanie. Jeśli 0 to polling. Wpp. jest to maksymalny
 *                 czas oczekiwania na dotarcie danych do odczytu do deskryptora @p fd.
 * @reutrn NULL jeśli fd jest mniejszy od 0 lub nie udało się zaalokować pamięci. Wpp. niezerowy
 *         wskaźnik.
 */
struct my_stream *my_stream_new(int fd, int signalfd, const struct timeval *time);

/**
 * @brief Funkcja dealokowuje pamięć zaalokowaną przez wywołanie my_stream_new.
 * @param[in,out] my_stream Struktura do dealokacji
 * @param[in] close_fd Czy należy zamknąć deskryptor
 * @param[in] close_signal Czy należy zamknąć deskryptor sygnałowy.
 */
int my_stream_delete(struct my_stream *my_stream, bool close_fd, bool close_signal);

/**
 * @brief Funkcja blokująca.
 */
int64_t writen(int fd, int sigfd, void *buff, size_t n);

/**
 * @brief Funkcja pobiera ze strumienia linię tekstę. Znak końca lini to
 *        kombinacja znaków CRLF.
 *        Implementacja zapewnia, że w napisie str, zostaną zapisany
 *        kandydat na linię, nawt jeśli połączenie zakończy się przed
 *        wczytaniem CRLF.
 *
 * @return Liczba pobranych linii, ie. 1; 0 jeśli read zwrócił 0 przed
 *         pobraniem znaków CRLF; ujemna liczba jeśli wystąpił błąd.
 */
int read_line(struct my_stream *stream, struct resizable_string *str);

/**
 * @brief Funkcja pobiera @p len bajtów ze strumienia @p stream i
 *        zapisuje je w buforze @p buff. Powrót nastąpi gdy pobrane
 *        zostanie @len bajtów lub read zwróci 0 lub wystąpi błąd.
 *
 * @return Liczba pobranych bajtów, ujemna liczba jeśłi wystąpił błąd.
 */
int64_t readn(struct my_stream *stream, char *buff, int64_t len);

/**
 * @breif Funkcja pobiera @p len bajtów ze strumienia @p stream i wpisuje
 *        je do deskryptora @p targetfd.
 * @param[in,out] stream
 * @param[out] targetfd
 * @param[in] len
 * @return Liczba przesłanych bajtów. Liczba ujemna jeśli wystąpił błąd.
 */
int64_t readto(struct my_stream *stream, int targetfd, int64_t len);

size_t find_character(char *w, char c);

size_t skip_leading_spaces(char *w, size_t beginning);

size_t skip_trailing_spaces(char *w, size_t end);

#endif
