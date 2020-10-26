/** @file
 * @brief Implementacja pliku nagłówkowego class.h
 */
#include "class.h"
#include <stdlib.h>

void* new(const struct base_class *_class, ...){
    void * p = calloc(1, _class->size);

    if (p) {
        *(const struct base_class **) p = _class; //Zapisujemy klasę dp jakiej należy obiekt

        if (_class->ctor){
            va_list ap;

            va_start(ap, _class);
            void *tmp = _class->ctor(p, &ap);
            va_end(ap);

            if (tmp == NULL || tmp != p)
                free(p);
            p = tmp;
        }
    }

    return p;
}

void delete(void *object){
    const struct base_class** class = object;

    if (class && *class && (*class)->dtor){
        object = (*class)->dtor(object);
    }

    free(object);
}

void *clone(void *object) {
    const struct base_class* class = *((struct base_class**) object);

    void * p = calloc(1, class->size);

    if (p) {
        *(const struct base_class **) p = class; //Zapisujemy klasę dp jakiej należy obiekt

        if (class->clone){

            void *tmp = class->clone(p, object);

            if (tmp == NULL)
                free(p);

            p = tmp;
        }
    }

    return p;
}

void *move(void *object) {
    const struct base_class* class = *((struct base_class**) object);

    void * p = calloc(1, class->size);

    if (p) {
        *(const struct base_class **) p = class; //Zapisujemy klasę dp jakiej należy obiekt

        if (class->clone){

            void *tmp = class->move(p, object);

            if (tmp == NULL)
                free(p);

            p = tmp;
        }
    }

    return p;
}
