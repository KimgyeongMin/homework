#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <err.h>
#include <event.h>
#include <evhttp.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>


#include <errno.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/keyvalq_struct.h>


int port = 9999;
struct event_base *base;

static void debug_callback(struct evhttp_request *req, void *arg)
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

struct evhttp_request *origin_req;

void chunk_callback(struct evhttp_request * req, void * arg)
{
     req = origin_req;
	printf("here chunk_callback\n");
}
 
// gets called when request completes
void request_callback(struct evhttp_request * req, void * arg)
{
	printf("here request_callback\n");
}
 
void static global_callback(struct evhttp_request *req, void *arg)
{
    struct evkeyvalq *headers;
	
     char * tmp;
     char * src_address;
     char * dst_address;
     short dst_port;
     ev_uint16_t src_port;
     struct evhttp_connect * srcreq;

     origin_req = req;
     srcreq = evhttp_request_get_connection(req);

    headers = evhttp_request_get_input_headers(req);


    tmp = evhttp_find_header(headers, "Host");
    printf("dstaddr: %s\n",tmp);
    dst_address = strtok(tmp,":");
    char *r = strtok(NULL,":");
    dst_port = atoi(r);


    evhttp_connection_get_peer(srcreq, &src_address, &src_port);
    if(strchr(src_address, ':') == NULL)
        sprintf(src_address, "%s:%d", src_address,src_port);

    struct evhttp *evhttp = evhttp_new(base);

    struct evhttp_request *evhttp_request;

    evhttp = evhttp_connection_new(dst_address, dst_port);
    evhttp_request = evhttp_request_new(request_callback, NULL);
    evhttp_request->chunk_cb = chunk_callback;
    evhttp_make_request(evhttp, evhttp_request, EVHTTP_REQ_GET, "/");
    
}

int main(int argc, char **argv)
{

    int param;

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
                printf("use default port : %d\n",port);
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

    evhttp_set_cb(http, "/debug", debug_callback, NULL);

    evhttp_set_gencb(http, global_callback, NULL);

    handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
    if(!handle)
    {
	fprintf(stderr, "Error, evhttp_bind_socket_with_handle().. couldn't bind to port %d.\n", port);
     	return 1;
    }

   event_base_dispatch(base);

   return 0;

}
