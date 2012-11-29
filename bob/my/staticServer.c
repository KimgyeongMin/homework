
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/keyvalq_struct.h>


int port = 9999;

static void global_callback(struct evhttp_request *req, void *arg)
{
    struct evbuffer *sendMessage = NULL;
    const char *cmdtype;
    struct evkeyvalq *headers;
    struct evkeyval  *header;

    sendMessage = evbuffer_new();

    switch (evhttp_request_get_command(req)) {
    case EVHTTP_REQ_GET: cmdtype = "GET"; break;
    case EVHTTP_REQ_POST: cmdtype = "POST"; break;
    case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
    case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
    case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
    case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
    case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
    case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
    case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
    default: cmdtype = "unknown"; break;
    }

    evbuffer_add_printf(sendMessage,
	 "<html>\n<title>hello libevent Server</title>\n<body>\n<h3>get method : %s</h3>",
	 cmdtype);


    /*
    #ifdef _EVENT_DEFINED_TQENTRY
    #undef TAILQ_ENTRY
    struct event_list;
    struct evkeyvalq;
    #undef _EVENT_DEFINED_TQENTRY
    #else
    TAILQ_HEAD (event_list, event);
    TAILQ_HEAD (evkeyvalq, evkeyval); 
 
    *********************************

    struct evkeyval {
        TAILQ_ENTRY(evkeyval) next;

        char *key;
        char *value;
    };
    */


    headers = evhttp_request_get_input_headers(req);
    header  = headers->tqh_first;
    for (; header; header = header->next.tqe_next)
    {
	evbuffer_add_printf(sendMessage, "<b>key: %s</b>\n	  value: %s\n<br>",header->key, header->value);
	header = header->next.tqe_next;
    } 


    evbuffer_add_printf(sendMessage, "</body></heml>\n");

    evhttp_send_reply(req,200, "OK", sendMessage);


}

int main(int argc, char **argv)
{

    int param;

    struct event_base *base;
    struct evhttp *http;
    struct evhttp_bound_socket *handle;
 

    if(argc == 1)
    {
	printf("usage : %s -p<port>\n",argv[0]);
	return 1;   
    }

    while( (param = getopt(argc, argv, ":p:")) != -1 ){
        switch(param){		
            case 'p':
		port = atoi(optarg);
		if(port < 1024)
		{
			printf("Don't use well know port");
			return 1;
		}
		if(port > 65535)
		{
			printf("unavailable port");
			return 1;
		}
		printf("use port : %d\n",port);
		break;
            case '?':
                printf("Unknow param\n");
		printf("usage : %s -p<port>\n",argv[0]);
		return 1;
            case ':':
                printf("use default port : 9999\n");
		break; 
        }
    }

    base = event_base_new();
    if(!base)
    {
	fprintf(stderr, "Error, event_base_new()\n");
   	return 1;
    }

    http = evhttp_new(base);
    if (!http) {
         fprintf(stderr, "Error, evhttp_new()\n");
         return 1;
     }

    evhttp_set_gencb(http, global_callback, NULL);

    handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
    if(!handle)
    {
	fprintf(stderr, "Error, evhttp_bind_socket_with_handle().. couldn't bind to port %d.\n", port);
     	return 1;
    }


        {
		char uri_root[512];
                struct sockaddr_storage ss;
                evutil_socket_t fd;
                ev_socklen_t socklen = sizeof(ss);
                char addrbuf[128];
                void *inaddr;
                const char *addr;
                int got_port = -1;
                fd = evhttp_bound_socket_get_fd(handle);
                memset(&ss, 0, sizeof(ss));
                if (getsockname(fd, (struct sockaddr *)&ss, &socklen)) {
                        perror("getsockname() failed");
                        return 1;
                }
                if (ss.ss_family == AF_INET) {
                        got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
                        inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
                } else if (ss.ss_family == AF_INET6) {
                        got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
                        inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
                } else {
                        fprintf(stderr, "Weird address family %d\n",
                            ss.ss_family);
                        return 1;
                }
                addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
                    sizeof(addrbuf));
                if (addr) {
                        printf("Listening on %s:%d\n", addr, got_port);
                        evutil_snprintf(uri_root, sizeof(uri_root),
                            "http://%s:%d",addr,got_port);
                } else {
                        fprintf(stderr, "evutil_inet_ntop failed\n");
                        return 1;
                }
        }

   event_base_dispatch(base);

   return 0;

}
