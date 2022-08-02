/*Example code for pub/sub
  run as publisher
  ./pubsub pub <channelName> <message>

  run as subscriber
   ./pubsub sub <channelName>*/


/*----------------------
Publish and Subscribe
------------------------*/

#include <csignal>
#include <iostream>
#include <cstdio>
#include <string>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include <redis.hpp>

namespace {
void subCallback(redisAsyncContext */*c*/, void *r, void *) {
    redisReply *reply = static_cast<redisReply *>(r);
    if (reply == NULL) {
        std::cout << "No reply\n";
        return;
    }
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3) {
        if (strcmp(reply->element[0]->str, "subscribe") != 0) {
            std::cout << "Message received on channel: \""
                      << reply->element[1]->str
                      << "\". Content: " << reply->element[2]->str << '\n';
        }
    }
}

void pubCallback(redisAsyncContext *c, void *r, void *) {
    redisReply *reply = static_cast<redisReply *>(r);
    if (reply == NULL) {
        std::cout << "No reply\n";
        return;
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        std::clog << reply->str << '\n';
    } else {
        std::cout << "message published\n";
    }
    redisAsyncDisconnect(c);
}
}

struct Redis_async {
    redisAsyncContext &m_ctx;
    event_base &m_event_base;

    Redis_async() : m_ctx(*redisAsyncConnect("127.0.0.1", 6379)), m_event_base(*event_base_new()) {}

    ~Redis_async() { redisAsyncFree(&m_ctx); }

    int32_t attach_Libevent() {
        return redisLibeventAttach(&m_ctx, &m_event_base);
    }

    int32_t dispatch() {
        return event_base_dispatch(&m_event_base);
    }

    int32_t command(redisCallbackFn *fn, char *data, std::string_view cmd) {
        return redisAsyncCommand(&m_ctx, fn, data, cmd.data());
    }
};

int main(int argc, const char **argv) {
    if (argc < 3) {
        std::cout << "Usage: sub stuff, subscribes to channel stuff" << '\n';
        return 1;
    }
    std::vector<const char *> args(argv, argv + argc);

    std::string mode(args.at(1));
    std::string channel(args.at(2));

    signal(SIGPIPE, SIG_IGN);

    Redis_async context;
    if (context.m_ctx.err) {
        std::cout << "Error: " << context.m_ctx.errstr << '\n';
        return 1;
    }

    context.attach_Libevent();

    if (mode == "pub") {
        // Publisher
        const std::string message = args.size() >= 3 ? args.at(3) : "";

        std::string command("publish ");
        command += channel;
        command += " '";
        command += message;
        command += "'";

        int status = context.command(pubCallback, NULL, command);
        if (status != 0) {
            std::clog << "status: " << status << '\n';
        }
    } else if (mode == "sub") {
        // Subscriber
        std::string command("subscribe ");
        command += channel;

        int status = context.command(subCallback, NULL, command);
        if (status != 0) {
            std::clog << "status: " << status << '\n';
        }
    } else
        std::cout << "Try pub or sub\n";

    context.dispatch();
    return 0;
}
