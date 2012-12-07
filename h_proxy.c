#include <evhttp.h>
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

#define MAX_PACKET 1024*512


int port = 9999;
struct event_base *base;
struct evhttp *http;
struct evhttp_bound_socket *handle;



void read_callback(struct bufferevent *bev, void *ctx)
{
     struct bufferevent *p = ctx;
     struct evbuffer *src, *dst;

     //output, input 버퍼를 얻는다
     src = bufferevent_get_input(bev);
     dst = bufferevent_get_output(p);

     //한쪽 버퍼에 값들을 전부 추가(받은 값 그대로 전송)
     evbuffer_add_buffer(dst, src);

     //모든 메시지를 읽을때까지 돈다(한번에 최대로 보낼 수 있는 양이 있음)
     if(evbuffer_get_length(dst) >= MAX_PACKET)
     {
        bufferevent_setcb(p, read_callback,  NULL, NULL, bev);
        bufferevent_disable(bev, EV_READ);
     }
}



void static global_callback(struct evhttp_request *req, void *arg)
{
    struct evkeyvalq *headers;
    char * src_address;
    char * dst_address;
    ev_uint16_t dst_port;
    ev_uint16_t src_port;

    struct evhttp_connection * srcreq;

    struct sockaddr_storage connect_addr;
    int socklen = sizeof(struct sockaddr_storage);
    memset(&connect_addr, 0x00, socklen);

    struct bufferevent *bufferOut;
    struct bufferevent *bufferIn;
    char *buff;

    //evhttp_request구조체 안에 들어있는 evhttp_connection 구조체를 얻음
    srcreq = evhttp_request_get_connection(req);

    //input 버퍼의 HTTP Header를 얻는다
    headers = evhttp_request_get_input_headers(req);

    //얻어온 HTTP Header에서 목적지 IP를 얻는다
    dst_address = evhttp_find_header(headers, "Host");
    if(strchr(dst_address, ':') == NULL)
        sprintf(dst_address, "%s:80", dst_address);
    printf("dst : %s\n", dst_address);

    //evhttp_connection 구조체에서 출발지 IP를 얻는다
    evhttp_connection_get_peer(srcreq, &src_address, &src_port);
    if(strchr(src_address, ':') == NULL)
        sprintf(src_address, "%s:%d", src_address,src_port);
    printf("src : %s\n",src_address);

    //bound 구조체를 이용해서 소켓 디스크립터를 얻음(근대 사용 안함)
    evutil_socket_t fd;
    fd = evhttp_bound_socket_get_fd(handle);

    //bufferIn은 이미 연결된 것을 사용, BufferOut는 새로 만듬
    bufferIn = srcreq->bufev;
    bufferOut  = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);

    if(!(bufferOut && bufferIn))
    {
       printf("error bufferevent_socket_new()");
    }

    //HTTP Header에서 얻어온 IP를 bufferOut에 할당하기 위해 IP를 파싱한다(HTTP Header에서 얻어온 건 단순 문자열)
    if(evutil_parse_sockaddr_port(dst_address, (struct sockaddr*)&connect_addr, &socklen)<0)
    {
        printf("dst sockaddr parse error!\n");
    }

     //bufferOut의 새로운 연결을 만든다
     if(bufferevent_socket_connect(bufferOut, (struct sockaddr *)&connect_addr, sizeof(struct sockaddr))<0)
     {
        bufferevent_free(bufferIn);
        bufferevent_free(bufferOut);
        exit(1);
     }

     //각각의 bufferevent에 Callback함수를 설정한다
     bufferevent_setcb(bufferIn , read_callback, NULL, NULL, bufferOut);
     bufferevent_setcb(bufferOut, read_callback, NULL, NULL, bufferIn);

     bufferevent_enable(bufferIn , EV_READ|EV_WRITE);
     bufferevent_enable(bufferOut, EV_READ|EV_WRITE);


}



int main(int argc, char **argv)
{

    int param;

    //인자값 처리
    if(argc == 1)
    {
        printf("usage : %s -p<port>\n",argv[0]);
        return 1;
    }
    // -p (port) 옵션 검사
    while( (param = getopt(argc, argv, ":p:")) != -1 ){
        switch(param){
            case 'p':
                port = atoi(optarg);
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

    //base 생성
    base = event_base_new();
    if(!base)
    {
        fprintf(stderr, "Error, event_base_new()\n");
        return 1;
    }

    //http 생성
    http = evhttp_new(base);
    if (!http) {
         fprintf(stderr, "Error, evhttp_new()\n");
         return 1;
     }

    //callback 세팅
    evhttp_set_gencb(http, global_callback, NULL);

    //bind (0.0.0.0으로 설정하고, Client에서 port만 맞게 들어오면 모두 받아들임)
    handle = evhttp_bind_socket_with_handle(http, "0.0.0.0", port);
    if(!handle)
    {
        fprintf(stderr, "Error, evhttp_bind_socket_with_handle().. couldn't bind to port %d.\n", port);
        return 1;
    }
    
    //dispatch가 이루어 져야 동작
    event_base_dispatch(base);

    return 0;
}
