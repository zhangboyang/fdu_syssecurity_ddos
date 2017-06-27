#include "common.h"

size_t bytevector_size(const struct bytevector *v)
{
    return v->end - v->begin;
}
size_t bytevector_capacity(const struct bytevector *v)
{
    return v->cap - v->begin;
}
void *bytevector_data(const struct bytevector *v)
{
    return v->begin;
}
void bytevector_realloc(struct bytevector *v, size_t new_capacity)
{
    size_t size = bytevector_size(v);
    assert(size <= new_capacity);
    v->begin = realloc(v->begin, new_capacity);
    v->end = v->begin + size;
    v->cap = v->begin + new_capacity;
}
void bytevector_init(struct bytevector *v)
{
    size_t default_capacity = 16;
    v->begin = v->end = malloc(default_capacity);
    v->cap = v->begin + default_capacity;
}
void bytevector_append(struct bytevector *v, const void *data, size_t data_len)
{
    size_t new_len = bytevector_size(v) + data_len;
    if (bytevector_capacity(v) < new_len) {
        bytevector_realloc(v, bytevector_capacity(v) * 2);
    }
    memcpy(v->end, data, data_len);
    v->end += data_len;
}
void bytevector_append_u8(struct bytevector *v, unsigned char val)
{
    bytevector_append(v, &val, sizeof(val));
}
void bytevector_append_u16(struct bytevector *v, unsigned short val)
{
    bytevector_append(v, &val, sizeof(val));
}
void bytevector_append_str(struct bytevector *v, const char *str)
{
    bytevector_append(v, str, strlen(str) + 1);
}
void bytevector_append_strnz(struct bytevector *v, const char *str)
{
    bytevector_append(v, str, strlen(str));
}
void bytevector_free(struct bytevector *v)
{
    free(v->begin);
}
void bytevector_dump(const struct bytevector *v)
{
    dump(bytevector_data(v), bytevector_size(v));
}
