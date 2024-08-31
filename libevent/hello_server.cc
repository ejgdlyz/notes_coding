#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

/**
 * 基于信号的 Hello World
 * 
*/

static const char MESSAGE[] = "Hello World!\n";
static const int PORT = 9725;

static void g_conn_writecb(struct bufferevent *bev, void *user_data) 
{   
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0) 
    {   
        puts("flushed answer\n");
        bufferevent_free(bev);
    }
}

static void g_conn_eventcb(struct bufferevent *bev, short events, void *user_data) 
{
    if (events & BEV_EVENT_EOF)
    {
        puts("Connected closed.\n");
    }
    else if (events & BEV_EVENT_ERROR)
    {
        printf("Got an error on the connection: %s\n", strerror(errno));
    }

    bufferevent_free(bev);
}

static void g_listener_cb(struct evconnlistener *listener, evutil_socket_t fd
    , struct sockaddr *sa, int socklen, void *user_data)
{
    struct event_base *base = reinterpret_cast<struct event_base*>(user_data);
    struct bufferevent *bev;

    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev)
    {
        fprintf(stderr, "Error constructing bufferevent\n");
        event_base_loopbreak(base);
        return ;
    }

    bufferevent_setcb(bev, nullptr, g_conn_writecb, g_conn_eventcb, nullptr);
    bufferevent_enable(bev, EV_WRITE);
    bufferevent_disable(bev, EV_READ);

    bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}

static void g_signal_cb(evutil_socket_t sig, short events, void *user_data) 
{
    struct event_base *base = reinterpret_cast<struct event_base*>(user_data);
    struct timeval delay = {2, 0};

    puts("Caught an interrupt signal; existing cleanly in tow seconds.\n");

    event_base_loopexit(base, &delay);
}

int main(int argc, char const *argv[])
{
    struct event_base *base;
    struct evconnlistener *listener;
    struct event *signal_event;

    struct sockaddr_in addr;

    base = event_base_new();
    if (!base) 
    {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    
    listener = evconnlistener_new_bind(base, g_listener_cb, reinterpret_cast<void *>(base)
        , LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    
    if (!listener)
    {
        fprintf(stderr, "Could not create a listener\n");
        return 1;
    }

    signal_event = evsignal_new(base, SIGINT, g_signal_cb, reinterpret_cast<void *>(base));

    if (!signal_event) 
    {
        fprintf(stderr, "Could not create/add a sginal event\n");
        return 1;
    }

    event_base_dispatch(base);

    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);

    puts("done\n");
    return 0;
}

