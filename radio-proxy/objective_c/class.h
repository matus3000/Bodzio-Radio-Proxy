/**@file
 * @brief Definicja prostego interfejsu obiektowego.
 * Pozwala on na korzystanie z funkcji @ref new, @ref move, @ref clone, @ref delete.
 */
#ifndef _CLASS_H_
#define _CLASS_H_
#include <stddef.h>
#include <stdarg.h>

/**
 * @brief Podstawowy interfejs do implementowania namiastki obiektowości w języku C
 */
struct base_class {
    /// Liczba bajtów potrzebna do alokacji pamięci dla instacji klasy.
    size_t size;
    /// Konstruktor
    void* (* ctor) (void *_self, va_list * ap);
    /// Destruktor
    void* (* dtor) (void * _self);
    /// Konstruktor kopiujący
    void* (* clone) (void *_self, void* copied);
    /// Konstruktor przenoszący
    void* (* move) (void * _self, void* moved);
};

/**
 * @brief Tworzy nową instancję klasy, której interfejs wskazuje @p _class
 *
 * Jeśli _class->ctor jest równy NULL funkcja zwraca zainicjalizowaną 0
 * strukturę. Do allokacji pamięci używany jest calloc, więc nie trzeba specyfikować
 * wartości domyślnych, które mają interpretację składającą sie z samych bajtów zerowych.
 *
 * Jeśli wartość zwrócona przez _class->ctor będzie równa NULL, pamięć zaalokowana na
 * nową instancję struktury jest zwalniania i zwrócony zostaje NULL.
 *
 * @param _class Wskaźnik na interfejs danej klay. NOT NULLABLE
 *
 * @return Wskaźnik na nową instancję klasy (NOT NULL), NULL jeśli nie udało się zaalokować pamięci lub
 *         wywołanie _class->ctor zwróciło NULL.
 */
void *new(const struct base_class *_class, ...);

/**
 * @brief Inicjuje instancję klasy, której interfejs wskazuje @p _class
 *        Działa podobnie jak new, ale nie alokuje pamięci na instancję
 *        struktury.
 *
 * @param[in] _class Wskaźnik na interfejs danej klay.
 * @param[in] place Wskaźnik na obiekt do inicjalizacji.
 * @pre @p _class i @p place nie sa NULLAMI.
 * @return Wskaźnik na zaincjalizowaną instancję klasy @p _class lub NULL, gdy
 *         wywołanie _class->ctor zwróciło NULL.
 */
void *init(const struct base_class *_class, void *place, ...);

/**
 * @brief Usuwa @p object z pamięci.
 *
 * @param[in] object Wskaźnik na obiekt do usunięcia. NULLABLE. Usuwa tylko obiekty
 *                   otrzymane za pomocą wywołań new, clone lub move.
 */
void delete(void *object);

/**
 * @brief Niszczy @p object. Uruchumia destruktor obiektu, ale nie zwalnia pamięci
 *        wskazywanej przez @p object.
 *
 * @param[in] object Wskaźnik na obiekt do usunięcia. NULLABLE
 */
void destroy(void *object);

/**
 * @brief Zwraca kopię objektu *object.
 *
 * @param object  Wskaźnik na obiekt do skopiowania, należy zapewnić, że klasa,
 *                której obiekt jest instancją, implementuje funkcję clone.
 *
 * @return Wskaźnik na nową kopię obiektu objekt, który należy usunąć za pomocą funkcji delete.
 */
void *clone(void *object);

/**
 * @brief Zwraca nowy obiekt, do którego przeniesiono zawartość object.
 *
 * @param object Wskaźnik na obiekt do przeniesienia, należy zapewnić, że klasa,
 *              której obiekt jest instancją, implementuje funkcję move.
 *
 * @attention Po wywołaniu funkcji należy usunąć @p object przy pomocy delete. Wszystekie
 *            operacja na tym obiekcie poza delete są błędne niezależnie od tego, czy
 *            funkcja zakończyła się sukcesem.
 *
 * @return Wskaźnik na nowy objekt, który należy usunąć za pomocą funkcji delete,
 *         NULL jeśli nie udało się zaalokować pamięci lub wywołanie _self->move
 *         przekazało NULL.
 */
void *move(void *object);

#endif
