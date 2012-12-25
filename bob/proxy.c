
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <event2/bufferevent_ssl.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>

static struct event_base *base;
static struct sockaddr_storage server_addr;
static struct sockaddr_storage client_addr;
static int client_addrlen;

#define MAXBUF (512*1024)

static void readcb(struct bufferevent *bev, void *ctx)
{
        struct bufferevent *partner = ctx;
        struct evbuffer *src, *dst;
		char reader[MAXBUF];
        char *strtmp;

		src = bufferevent_get_input(bev);
        dst = bufferevent_get_output(partner);

		const size_t n = evbuffer_get_length(src);
		evbuffer_copyout(src, reader, n);

        printf("test : %s\n", reader);
		evbuffer_add_buffer(dst, src);
        
        if (evbuffer_get_length(dst) >= MAXBUF)
                bufferevent_setcb(partner, readcb, NULL, NULL, bev);
}

static void accept_cb(struct evconnlistener *listener, evutil_socket_t fd,
    struct sockaddr *a, int slen, void *p)
{
        struct bufferevent *b_out, *b_in;
        b_in = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
        b_out = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);

        if(!(b_in && b_out))
	    {
			printf("bufferevent_sock_new Error\n");
			exit(1);
		}
          

		if (bufferevent_socket_connect(b_out, (struct sockaddr*)&client_addr, client_addrlen)<0) {
                perror("bufferevent_socket_connect");
                bufferevent_free(b_out);
                bufferevent_free(b_in);
                return;
        }

        bufferevent_setcb(b_in, readcb, NULL, NULL, b_out);
        bufferevent_setcb(b_out, readcb, NULL, NULL, b_in);

        bufferevent_enable(b_in, EV_READ|EV_WRITE);
        bufferevent_enable(b_out, EV_READ|EV_WRITE);
}

void main(int argc, char **argv)
{
        int server_addrlen;
        struct evconnlistener *listener;

        base = event_base_new();
        if (!base) {
                perror("event_base_new()");
                exit(1);
        }

        memset(&client_addr, 0, sizeof(client_addr));
		memset(&server_addr, 0, sizeof(server_addr));
        client_addrlen = sizeof(client_addr);
        server_addrlen = sizeof(server_addr);

        evutil_parse_sockaddr_port(argv[1], (struct sockaddr*)&server_addr, &server_addrlen); 
        evutil_parse_sockaddr_port(argv[2], (struct sockaddr*)&client_addr, &client_addrlen);

        listener = evconnlistener_new_bind(base, accept_cb, NULL,
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC|LEV_OPT_REUSEABLE,
            -1, (struct sockaddr*)&server_addr, server_addrlen);

        event_base_dispatch(base);

        evconnlistener_free(listener);
        event_base_free(base);

}
                                                            
