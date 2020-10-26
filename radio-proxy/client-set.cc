#include "client-set.h"
#include <map>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#define ASSERT_SET_TYPE(x) if (x != CS_WAITING_SET && x != CS_CONNECTED_SET) return -1

struct client {
    in_addr_t s_addr;///< Sieciowa kolejność bajtów
    uint16_t  sin_port;///< Sieciowa kolejność bajtów
    client(struct sockaddr_in*);
};

struct client_cmp{
    bool operator()(const client &lhs, const client &rhs) const {
        return  lhs.s_addr < rhs.s_addr ||
            (lhs.s_addr == rhs.s_addr && lhs.sin_port < rhs.sin_port);
    }
};

struct client_set {
    static const uint16_t max_clients = 64;
    uint16_t current_clients;
    unsigned int timeout;
    std::map<client, time_t, client_cmp> connected;
    client_set(unsigned int timeout);
    ~client_set();
};

client::client(struct sockaddr_in *addr) :
    s_addr(addr->sin_addr.s_addr), sin_port(addr->sin_port) {}

client_set::client_set(unsigned int _timeout):
    current_clients(0), timeout(_timeout) {}

client_set::~client_set() {}

static bool expired(time_t _time, int timeout) {
    time_t current = time(NULL);

    return !(_time + timeout + 1 > current);
}

struct client_set *client_set_new(unsigned int timeout) {
    try {
        auto *result =  new struct client_set(timeout);
        return result;
    } catch (std::exception& e) {
        return NULL;
    }
}

void client_set_delete(struct client_set *client_set) {
    try {
        delete(client_set);
    }  catch (std::exception& e) {
        (void) 1;
    }
}

bool client_set_contains(struct client_set *client_set, struct sockaddr_in *addr,
                         enum client_set_info set_type) {

    ASSERT_SET_TYPE(set_type);
    bool result;
    auto &set =  client_set->connected;

    auto it = set.find({addr});
    if (it == set.end()) {
        result = false;
    } else if (expired(it->second, client_set->timeout)) {
        set.erase(it);
        result = false;
    } else {
        result = true;
    }

    return result;
}

int client_set_refresh_client(struct client_set *client_set, struct sockaddr_in *addr,
                              enum client_set_info set_type) {
    ASSERT_SET_TYPE(set_type);
    int result = 0;

    auto &set = client_set->connected;
    auto it = set.find({addr});

    if (it == set.end()) {
        result = -1;
    } else if (expired(it->second, client_set->timeout)) {
        set.erase(it);
        result = -1;
    } else {
        it->second = time(NULL);
        result = 0;
    }

    return result;
}

int client_set_add_client(struct client_set *client_set, struct sockaddr_in *addr) {
    int ret_code = 0;
    if (!client_set) return 0;

    try {
        client_set->connected.insert({{addr}, time(NULL)});
    } catch (std::exception &e) {
        ret_code = 1;
    }

    return ret_code;
}

int client_set_send_datagram(struct client_set *client_set, int udpsocket, char *buff, uint32_t len) {
    int cnt = 0;
    struct sockaddr_in udp_client;
    auto &set = client_set->connected;
    auto it = set.begin();

    bool remove_client = false;
    while (it != set.end()) {
        if (!expired(it->second, client_set->timeout)) {
            if (cnt < client_set->max_clients) {
                ++cnt;
                udp_client.sin_family = AF_INET;
                udp_client.sin_addr.s_addr = it->first.s_addr;
                udp_client.sin_port = it->first.sin_port;
                sendto(udpsocket, buff, len, 0, (struct sockaddr*) &udp_client, sizeof(udp_client));
                remove_client = false;
            } else {
                remove_client = true;
            }
        } else {
            remove_client = true;
        }

        if (remove_client) {
            it = set.erase(it);
            remove_client = false;
        } else {
            ++it;
        }
    }

    return 0;
}
