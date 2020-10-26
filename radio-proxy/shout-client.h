#ifndef __SHOUT_CLIENT__
#define __SHOUT_CLIENT__
#include <stdbool.h>
#include <stddef.h>
#include "net_utils.h"
#include "shout_response.h"
#include "shout_request.h"

/**
 * @brief Funkcja korzystając z pola deskryptora struktury @p stream wysyła zapytanie GET do
 *        serwera radia internetowego zgodnie z polami struktury @p rq. Następnie pobiera
 *        odpowiedź serwera i zapisuje ją w zainicjalizowanej struktrze HTTP_response.
 *
 * @param[in] stream Struktura opakowująca gniazdo TCP.
 * @param[in] rq Struktura zawierająca informacje potrzebne do stworzenia rządania HTTP.
 * @param[out] repsone Sparsowana odpowiedź serwera.
 *
 * @return 0 jeśli wszystko było w porządku, liczba różna od 0 jeśli nie udało się zaalokować pamięci,
 *         otrzymana odpowiedź nie była poprawnie sformatowaną odpowiedzią HTTP, nie otzymano pola
 *         ICY-METAINT pomimo wysłania prośby, przekroczono czas oczekiwania lub nie udało się zaalokować
 *         pamięci.
 */
int shout_client_server_negotiation(struct my_stream *stream, struct shout_request_message *rq,
                              struct HTTP_response_message *response);

/**
 * @brief Funkcja uruchamia pętlę, w której program pobiera audio z @p stream
 *        i wypisuje je do deskryptora @p STDOUT_FILENO, jeśli metaint jest różne od 0
 *        pobiera metadane i wypisuje je na STDERR_FILENO. Funkcja powraca albo, gdy
 *        proces otrzymał sygnał SIGKILL lub gdy przekroczono czas oczekiwania.
 *
 * @param[in,out] stream Struktura opakowująca gniazdo TCP.
 * @param[in] metaint Parameter metaint.
 * @param[in] proxy Informacja o tym czy program ma działać w trybie proxy.
 *
 * @attention Jeśli program działa w trybie proxy, to przed wypisaniem odebranych
 *            bajtów na STDOUT/STDERR_FILENO podaje on liczbę wypisywanych bajtów,
 *            która ma rozmiar 1-bajta.
 *
 * @return
 */
int shout_read_write_loop(struct my_stream *stream, size_t metaint, int outfd, int errfd, bool proxy);

#endif
