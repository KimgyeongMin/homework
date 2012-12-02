
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/keyvalq_struct.h>
#include <event2/http_compat.h>
#include <event2/listener.h>

#define MAX_PACKET 1024*512


int port = 9999;

struct event_base *base;
struct evhttp_connection *evhttp_connection;
struct evhttp_request* evhttp_request;

const char *global_uri;
struct evhttp_request * global_req;


static struct sockaddr_storage listen_addr;
static struct sockaddr_storage connect_addr;

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


void read_callback(struct bufferevent *bev, void *ctx)
{
     struct bufferevent *p = ctx;
     struct evbuffer *src, *dst;
printf("in read callback");
     src = bufferevent_get_input(bev);
     dst = bufferevent_get_output(p);

     evbuffer_add_buffer(dst, src);

     if(evbuffer_get_length(dst) >= MAX_PACKET)
     {
        bufferevent_setcb(p, read_callback,  NULL, NULL, bev);
        bufferevent_disable(bev, EV_READ);
     }
printf("pass last\n");
}



void accept_callback(struct evconnlistener *listener, evutil_socket_t fd,
         struct sockaddr *a, int slen, void *p)
{

     struct bufferevent *bufferOut;
     struct bufferevent *bufferIn;

     printf("accept pass\n");

     bufferIn = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
     bufferOut  = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);

     if(!(bufferOut && bufferIn))
     {
        printf("error bufferevent_socket_new()");
     }


     if(bufferevent_socket_connect(bufferOut, (struct sockaddr *)&connect_addr, sizeof(struct sockaddr))<0)
     {
        bufferevent_free(bufferIn);
        bufferevent_free(bufferOut);
        exit(1);
     }

     printf("buffevent pass!\n");

     bufferevent_setcb(bufferIn , read_callback, NULL, NULL, bufferOut);
     bufferevent_setcb(bufferOut, read_callback, NULL, NULL, bufferIn);

     bufferevent_enable(bufferIn , EV_READ|EV_WRITE);
     bufferevent_enable(bufferOut, EV_READ|EV_WRITE);

}


void static global_callback(struct evhttp_request *req, void *arg)
{

    const char *uri = evhttp_request_get_uri(req);
    struct evbuffer *sendMessage = NULL;
    char * serveraddr;
    short port; 
    struct evkeyvalq *headers;
    struct evkeyval  *header;


     char * src_address;
     char * dst_address;
     ev_uint16_t src_port;
     struct evhttp_connect * srcreq;
     srcreq = evhttp_request_get_connection(req);



    int socklen;
    struct evconnlistener *listener;

    /* get dst address */
    sendMessage = evbuffer_new();

    headers = evhttp_request_get_input_headers(req);
    header  = headers->tqh_first;

    dst_address = evhttp_find_header(headers, "Host");
   
    evhttp_connection_get_peer(srcreq, &src_address, &src_port);

    if(strchr(src_address, ':') == NULL)	
    	sprintf(src_address, "%s:%d", src_address,src_port);


    printf("src address : %s\n",src_address);
    printf("dst address : %s\n",dst_address);


    socklen = sizeof(struct sockaddr_storage);

    memset(&listen_addr,  0, socklen);
    memset(&connect_addr, 0, socklen);

    if(evutil_parse_sockaddr_port(src_address, (struct sockaddr*)&listen_addr, &socklen)<0)
    {
        printf("fail listen");
	exit(1);
    }

    if(evutil_parse_sockaddr_port("211.237.1.231:80", (struct sockaddr*)&connect_addr, &socklen)<0)
    {
        printf("fail connect");
	exit(1);
    }


    listener = evconnlistener_new_bind(base, accept_callback, NULL,
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC|LEV_OPT_REUSEABLE,
            -1, (struct sockaddr*)&listen_addr, socklen);

 
   // evhttp_send_reply(req, 200, "OK", sendMessage);    
}




int main(int argc, char **argv)
{

    int param;
    int result;
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

   result = event_base_dispatch(base);
   printf("event_base_dispatch : %s", result);
   evhttp_free(http);
   event_base_free(base);
   
   return 0;

}
