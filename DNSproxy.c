#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <netdb.h>

#include <sys/socket.h>

#include <event2/dns.h>
#include <event2/dns_struct.h>
#include <event2/util.h>
#include <event2/event.h>

#define LISTEN_PORT 53
#define TTL 4000

//KLDP IP address
const ev_uint8_t REDIRECT[] = { 211, 237, 1, 231 };
ev_uint8_t dnsip[20]= {0,};

struct event_base *base;
struct evdns_base *dnsbase;

void server_callback(struct evdns_server_request *request, void *data)
{	
	int error=DNS_ERR_NONE;
	struct hostent *host_entry;

	char str_uri[200];
	char* dnsipname[10];

	char *tmp;
	int i;

	for (i=0; i < request->nquestions; ++i) {
		const struct evdns_server_question *q = request->questions[i];
		int ok=-1;
		//Redirection 시킬 URI를 입력
		if (0 == evutil_ascii_strcasecmp(q->name, "nid.naver.com")) {
			if (q->type == EVDNS_TYPE_A)
				ok = evdns_server_request_add_a_reply(request, q->name, 1, REDIRECT, TTL);
		} else {
			//질의한 URI를 응답 (A record 만 해당)
			if (q->type == EVDNS_TYPE_A){   
				printf("A url: %s\n", q->name);
				sprintf(str_uri, "%s", q->name);
				
				host_entry = gethostbyname(str_uri);
				if (!host_entry)
				{
					printf( "gethostbyname() Error\n");
					error = DNS_ERR_SERVERFAILED;
				}else{  //IP addr parser
					strcpy((char *)dnsipname, (const char *)inet_ntoa( *(struct in_addr*)host_entry->h_addr_list[0]));					
					tmp = strtok((char *)dnsipname, ".");

					dnsip[0] = atoi(tmp);					
					tmp = strtok(NULL, ".");
					dnsip[1] = atoi(tmp);
					tmp = strtok(NULL, ".");
					dnsip[2] = atoi(tmp);					
					tmp = strtok(NULL, ".");
					dnsip[3] = atoi(tmp);

					ok = evdns_server_request_add_a_reply(request, str_uri, 1, dnsip, TTL);
				}
			}//다른 TYPE의 DNS요청을 확인
			else if(q->type == EVDNS_TYPE_NS ){
				printf("EVDNS_TYPE_NS url: %s\n", q->name);
			}
			else if(q->type == EVDNS_TYPE_CNAME ){
				printf("EVDNS_TYPE_CNAME url: %s\n", q->name);
			}
			else if(q->type == EVDNS_TYPE_SOA ){
				printf("EVDNS_TYPE_SOA url: %s\n", q->name);
			}
			else if(q->type == EVDNS_TYPE_PTR ){
				printf("EVDNS_TYPE_PTR url: %s\n", q->name);
			}
			else if(q->type == EVDNS_TYPE_MX ){
				printf("EVDNS_TYPE_MX url: %s\n", q->name);
			}
			else if(q->type == EVDNS_TYPE_TXT ){
				printf("EVDNS_TYPE_TXT url: %s\n", q->name);
			}
			else if(q->type == EVDNS_TYPE_AAAA ){
				printf("EVDNS_TYPE_AAAA url: %s\n", q->name);
			}
			else if(q->type == EVDNS_QTYPE_AXFR  ){
				printf("EVDNS_QTYPE_AXFR  url: %s\n", q->name);
			}
			else if(q->type == EVDNS_QTYPE_ALL ){
				printf("EVDNS_QTYPE_ALL url: %s\n", q->name);
			}
		}

		if (ok<0 && error==DNS_ERR_NONE)
			error = DNS_ERR_SERVERFAILED;
	}
	evdns_server_request_respond(request, error);
}

int main(int argc, char **argv)
{
	struct evdns_server_port *server;
	evutil_socket_t server_fd;
	struct sockaddr_in listenaddr;

	base = event_base_new();
	if (!base)
		return 1;

	dnsbase = evdns_base_new(base, 1);

        //DNS Server address Set
	server_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_fd < 0)
		return 1;
	memset(&listenaddr, 0, sizeof(listenaddr));
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_port = htons(LISTEN_PORT);
	listenaddr.sin_addr.s_addr = INADDR_ANY;	
	
	if (bind(server_fd, (struct sockaddr*)&listenaddr, sizeof(listenaddr))<0)
		return 1;
	
	//Start DNS Server
	server = evdns_add_server_port_with_base(base, server_fd, 0, server_callback, NULL);
	
	event_base_dispatch(base);

	evdns_close_server_port(server);
	evdns_base_free(dnsbase, 0);
	event_base_free(base);

	return 0;
}
