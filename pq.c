#include "common.h"

void pq_init(struct priority_queue *pq, size_t capacity, pq_less less, pq_setptr setptr)
{
    *pq = (struct priority_queue) {
        .data = calloc(capacity, sizeof(void *)),
        .size = 0,
        .capacity = capacity,
        .less = less,
        .setptr = setptr,
    };
}

void pq_free(struct priority_queue *pq)
{
    free(pq->data);
    pq->data = NULL;
}

static void pq_swap(struct priority_queue *pq, size_t a, size_t b)
{
    void *t = pq->data[a];
    pq->setptr(pq->data[a] = pq->data[b], a);
    pq->setptr(pq->data[b] = t, b);
}

void pq_deckey(struct priority_queue *pq, size_t item)
{
    while (item > 0) {
        if (pq->less(pq->data[item], pq->data[pq_fa(item)])) {
            pq_swap(pq, item, pq_fa(item));
            item = pq_fa(item);
        } else {
            break;
        }
    }
}

static void pq_heapify(struct priority_queue *pq, size_t item)
{
    while (1) {
        size_t t = item;
        if (pq_lch(item) < pq->size && pq->less(pq->data[pq_lch(item)], pq->data[t])) {
            t = pq_lch(item);
        }
        if (pq_rch(item) < pq->size && pq->less(pq->data[pq_rch(item)], pq->data[t])) {
            t = pq_rch(item);
        }
        if (t == item) break;
        pq_swap(pq, item, t);
        item = t;
    }
}

void *pq_top(struct priority_queue *pq)
{
    assert(pq->size > 0);
    return pq->data[0];
}

int pq_empty(struct priority_queue *pq)
{
    return pq->size == 0;
}

void pq_push(struct priority_queue *pq, void *item)
{
    assert(pq->size < pq->capacity);
    pq->data[pq->size++] = item;
    pq_deckey(pq, pq->size - 1);
}

void pq_pop(struct priority_queue *pq)
{
    assert(pq->size > 0);
    pq_swap(pq, 0, pq->size - 1);
    pq->size--;
    pq_heapify(pq, 0);
}

void pq_erase(struct priority_queue *pq, size_t item)
{
    while (item > 0) {
        pq_swap(pq, item, pq_fa(item));
        item = pq_fa(item);
    }
    pq_pop(pq);
}


/*
// simple self-test
//   should output: 1 2 4 5 6 7 8 

struct ival {
    int v;
    size_t idx;
};

int ival_less(const void *a, const void *b)
{
    return ((struct ival *)a)->v < ((struct ival *)b)->v;
}
void ival_setptr(void *p, size_t idx)
{
    ((struct ival *)p)->idx = idx;
}
void test()
{
    struct priority_queue pq;
    struct ival v[8];
    v[0].v = 5;
    v[1].v = 200;
    v[2].v = 7;
    v[3].v = 4;
    v[4].v = 1;
    v[5].v = 3;
    v[6].v = 8;
    v[7].v = 6;
    
    
    pq_init(&pq, 8, ival_less, ival_setptr);
    
    pq_push(&pq, &v[0]);
    pq_push(&pq, &v[1]);
    pq_push(&pq, &v[2]);
    pq_push(&pq, &v[3]);
    pq_push(&pq, &v[4]);
    pq_push(&pq, &v[5]);
    pq_push(&pq, &v[6]);
    pq_push(&pq, &v[7]);
    
    v[1].v = 2;
    pq_deckey(&pq, v[1].idx);
    
    pq_erase(&pq, v[5].idx);
    
    while (!pq_empty(&pq)) {
        printf("%d ", ((struct ival *)pq_top(&pq))->v);
        pq_pop(&pq);
    }
    
    printf("\n");
    
    pq_free(&pq);
}
*/
