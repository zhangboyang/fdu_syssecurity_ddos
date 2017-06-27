#include "common.h"

int ip_rawsocket()
{
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (fd < 0) fail("can't create socket");
    int v = 1;
    int r;
    r = setsockopt(fd, IPPROTO_IP, IP_HDRINCL, (void *) &v, sizeof(v));
    if (r < 0) fail("can't set HDRINCL");
    
    return fd;
}

void ip_init(struct ip *ip, u_int8_t p)
{
    *ip = (struct ip) {
        .ip_v = 4,
        .ip_hl = DDOS_IPHDR_LEN / 4,
        .ip_tos = 0,
        .ip_len = htons(DDOS_IPHDR_LEN),
        .ip_id = 0,
        .ip_off = htons(IP_DF),
        .ip_ttl = 64,
        .ip_p = p,
        .ip_sum = 0,
        .ip_src.s_addr = 0,
        .ip_dst.s_addr = 0,
    };
}

void ip_setsrc(struct ip *ip, struct in_addr src)
{
    ip->ip_src = src;
}
void ip_setdst(struct ip *ip, struct in_addr dst)
{
    ip->ip_dst = dst;
}

void ip_setid(struct ip *ip, u_short id)
{
    ip->ip_id = htons(id);
}

void ip_resize(struct ip *ip, ushort datalen)
{
    ip->ip_len = htons(DDOS_IPHDR_LEN + datalen);
}

void ip_cksum(struct ip *ip)
{
    ip->ip_sum = 0;
    // no need to calc checksum, kernel will calc it for us
    //ip->ip_sum = in_cksum((u_short *) ip, sizeof(*ip));
}

ushort ip_size(struct ip *ip)
{
    return ntohs(ip->ip_len);
}

void ip_mksaddr(struct sockaddr_in *saddr, struct in_addr addr)
{
    *saddr = (struct sockaddr_in) {
        .sin_family = AF_INET,
        .sin_port = 0,
        .sin_addr = addr,
    };
}

