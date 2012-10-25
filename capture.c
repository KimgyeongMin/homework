#include <sys/time.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <pcap/pcap.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>



void pcap_error(const char *failed, const char *errbuf)
{
        printf("error %s: %s\n",failed, errbuf);
        exit(0);
}

void progressPacket(u_char *param, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{

    struct ether_header *ep;
    struct ip *ip_packet;
    struct tcphdr *tcp_packet;

    unsigned short ether_type;    
    int count =0;
    int length=pkthdr->len;

    ep = (struct ether_header *)packet;

    packet += sizeof(struct ether_header);

    ether_type = ntohs(ep->ether_type);

    if (ether_type == ETHERTYPE_IP)
    { 
        ip_packet = (struct ip *)packet;
        printf("IP header\n");
        printf("Src Address : %s\n", inet_ntoa(ip_packet->ip_src));
        printf("Dst Address : %s\n", inet_ntoa(ip_packet->ip_dst));

        if (ip_packet->ip_p == IPPROTO_TCP)
        {
            tcp_packet = (struct tcp *)(packet + ip_packet->ip_hl * 4);
	    printf("TCP header\n");
            printf("Src Port : %d\n" , ntohs(tcp_packet->source));
            printf("Dst Port : %d\n" , ntohs(tcp_packet->dest));
        }

        while(length--)
        {
            printf("%02x", *(packet++)); 
            if ((++count % 20) == 0) 
                printf("\n");
        }
    }

    printf("\n\n");
}    

int main()
{
        struct pcap_pkthdr header;
        const u_char *packet;  
        char errbuf[PCAP_ERRBUF_SIZE];
        char *device;
        pcap_t *pcap_handle;
 
        device = pcap_lookupdev(errbuf);

        if(device == NULL)
                pcap_error("pcap_lookupdev",errbuf);


        pcap_handle = pcap_open_live(device,4096,1,0,errbuf);

        if(pcap_handle == NULL)
                pcap_error("pcap_open_live",errbuf);

        pcap_loop(pcap_handle ,0 ,progressPacket , NULL);

        pcap_close(pcap_handle);

        return 0;
}


