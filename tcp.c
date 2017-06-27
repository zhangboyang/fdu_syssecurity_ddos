#include "common.h"

static int tcpconn_less(const void *a, const void *b)
{
    return ts_less(&((struct tcpconn *)a)->nextsche, &((struct tcpconn *)b)->nextsche);
}
static void tcpconn_setptr(void *c, size_t p)
{
    ((struct tcpconn *)c)->heap_ptr = p;
}

void tcppool_init(struct tcppool *p, struct in_addr dstaddr, ushort dstport, int maxconn, void *pool_userdata, void (*new_conn)(struct tcpconn *conn, struct tcppool *pool), void (*del_conn)(struct tcpconn *conn, struct tcppool *pool), int (*conn_onevent)(struct tcpconn *conn, struct tcppool *pool))
{
    assert(maxconn > 0);
    
    // init structure
    *p = (struct tcppool) {
        .dstaddr = (struct sockaddr_in) {
            .sin_family = AF_INET,
            .sin_port = htons(dstport),
            .sin_addr = dstaddr,
        },
        .maxconn = maxconn,
        .pool_userdata = pool_userdata,
        
        .new_conn = new_conn,
        .del_conn = del_conn,
        .conn_onevent = conn_onevent,
        
        .epfd = epoll_create(maxconn),
        .conn = malloc(maxconn * sizeof(struct tcpconn)),
    };
    
    // init priority queue
    pq_init(&p->pq, maxconn, tcpconn_less, tcpconn_setptr);
    
    // init free list
    int i;
    for (i = 0; i < maxconn; i++) {
        p->conn[i].fd = -1;
        p->conn[i].conn_userdata = i < maxconn - 1 ? &p->conn[i + 1] : NULL;
    }
    p->flist = &p->conn[0];    
}


static struct tcpconn *tcppool_newconn(struct tcppool *p)
{
    // allocate new node from free list
    struct tcpconn *c = p->flist;
    if (!c) return NULL;
    assert(c->fd == -1);
    p->flist = c->conn_userdata;
    
    // init structure
    c->conn_userdata = NULL;
    c->inpq = 0;
    clock_gettime(CLOCK_REALTIME, &c->nextsche);
    
    // create socket and connect
    c->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (c->fd < 0) fail_sys("socket() failed.");
    int r = connect(c->fd, (struct sockaddr *) &p->dstaddr, sizeof(p->dstaddr));
    if (r < 0 && errno != EINPROGRESS) fail_sys("connect() failed.");
    
    // add to epfd
    struct epoll_event e = {
        .events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET,
        .data.ptr = c,
    };
    epoll_ctl(p->epfd, EPOLL_CTL_ADD, c->fd, &e);
    
    // call user new callback
    if (p->new_conn) p->new_conn(c, p);
    
    return c;
}

static void tcppool_delconn(struct tcppool *p, struct tcpconn *c)
{
    assert(c->fd >= 0);
    
    // call user delete callback
    if (p->del_conn) p->del_conn(c, p);
    
    // remove from priority queue
    if (c->inpq) {
        c->inpq = 0, pq_erase(&p->pq, c->heap_ptr);
    }
    
    // remove from epfd
    epoll_ctl(p->epfd, EPOLL_CTL_DEL, c->fd, NULL);
    
    // close socket and add to free list
    close(c->fd);
    c->fd = -1;
    c->conn_userdata = p->flist;
    p->flist = c;
}

void tcppool_oneloop(struct tcppool *p)
{
    int i;
    struct epoll_event e[TCPPOLL_MAXEV];
    struct timespec nts;
    struct tcpconn *qt;
    int timeout;
    
    // create new connections
    while (tcppool_newconn(p));
    
    // determine wait dead line
    clock_gettime(CLOCK_REALTIME, &nts);
    if (pq_empty(&p->pq)) {
        timeout = -1;
    } else if (ts_less(&(qt = pq_top(&p->pq))->nextsche, &nts)) {
        timeout = 0;
    } else {
        timeout = ts_minus(&qt->nextsche, &nts); 
    }
    //printf("timeout is %d\n", timeout);
    
    // process epoll events
    int ecnt = epoll_wait(p->epfd, e, TCPPOLL_MAXEV, timeout);
    for (i = 0; i < ecnt; i++) {
        uint32_t ev = e[i].events;
        struct tcpconn *c = e[i].data.ptr;
        if ((ev & EPOLLHUP) || (ev & EPOLLERR) || (ev & EPOLLRDHUP)) {
            //printf("remote closed.\n");
            tcppool_delconn(p, c);
        } else {
            if (!c->inpq) {
                c->inpq = 1, pq_push(&p->pq, c);
            }
        }
    }
    
    // call onevent callbacks
    clock_gettime(CLOCK_REALTIME, &nts);
    while (!pq_empty(&p->pq) && ts_less(&(qt = pq_top(&p->pq))->nextsche, &nts)) {
        qt->inpq = 0, pq_pop(&p->pq);
        int r = p->conn_onevent(qt, p);
        if (r > 0) {
            qt->inpq = 1, pq_push(&p->pq, qt);
        } else if (r == 0) {
            tcppool_delconn(p, qt);
        }
    }
}

void tcppool_free(struct tcppool *p)
{
    // free all connections
    int i;
    for (i = 0; i < p->maxconn; i++) {
        if (p->conn[i].fd < 0) {
            tcppool_delconn(p, &p->conn[i]);
        }
    }
    free(p->conn);
    
    // free priority queue
    pq_free(&p->pq);
    
    // close epfd
    close(p->epfd);
}

void tcppool_delaysche(struct tcpconn *c, long long ms)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts_plus(&c->nextsche, &ts, ms);
}
