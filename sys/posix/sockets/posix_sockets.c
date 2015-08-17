/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @file
 * @brief   Providing implementation for POSIX socket wrapper.
 * @author  Martine Lenders <mlenders@inf.fu-berlin.de>
 * @todo
 */

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include "fd.h"
#include "mutex.h"
#include "net/conn.h"
#include "sys/socket.h"
#include "netinet/in.h"

#define SOCKET_POOL_SIZE    (4)
#define CONN_IPV4   defined(MODULE_CONN_IP4) || defined(MODULE_CONN_UDP4) || \
                    defined(MODULE_CONN_TCP4)
#define CONN_IPV6   defined(MODULE_CONN_IP6) || defined(MODULE_CONN_UDP6) || \
                    defined(MODULE_CONN_TCP6)

/**
 * @brief   Unitfied connection type.
 */
typedef union {
    /* is not supposed to be used */
    /* cppcheck-suppress unusedStructMember */
    int undef;                  /**< for case that no connection module is present */
#ifdef  MODULE_CONN_IP4
    conn_ip4_t raw4;            /**< raw IPv4 connection */
#endif  /* MODULE_CONN_IP4 */
#ifdef  MODULE_CONN_IP6
    conn_ip6_t raw6;            /**< raw IPv6 connection */
#endif  /* MODULE_CONN_IP6 */
#ifdef  MODULE_CONN_TCP4
    conn_tcp4_t tcp4;           /**< TCP over IPv4 connection */
#endif  /* MODULE_CONN_TCP4 */
#ifdef  MODULE_CONN_TCP6
    conn_tcp6_t tcp6;           /**< TCP over IPv6 connection */
#endif  /* MODULE_CONN_TCP6 */
#ifdef  MODULE_CONN_UDP4
    conn_udp4_t udp4;           /**< UDP over IPv4 connection */
#endif  /* MODULE_CONN_UDP4 */
#ifdef  MODULE_CONN_UDP6
    conn_udp6_t udp6;           /**< UDP over IPv6 connection */
#endif  /* MODULE_CONN_UDP6 */
} socket_conn_t;

typedef struct {
    int fd;
    int domain;
    int type;
    int protocol;
    socket_conn_t conn;
} socket_t;

socket_t _pool[SOCKET_POOL_SIZE];
mutex_t _pool_mutex = MUTEX_INIT;

const struct sockaddr_in6 in6addr_any = {AF_INET6, 0, 0, IN6ADDR_ANY_INIT, 0};
const struct sockaddr_in6 in6addr_loopback = {
    AF_INET6, 0, 0, IN6ADDR_LOOPBACK_INIT, 0
};

static socket_t *_get_free_socket(void)
{
    for (int i = 0; i < SOCKET_POOL_SIZE; i++) {
        if (_pool[i].domain == AF_UNSPEC) {
            return &_pool[i];
        }
    }
    return NULL;
}

static socket_t *_get_socket(int fd)
{
    for (int i = 0; i < SOCKET_POOL_SIZE; i++) {
        if (_pool[i].fd == fd) {
            return &_pool[i];
        }
    }
    return NULL;
}

static inline int _choose_ipproto(int type, int protocol)
{
    switch (type) {
#if defined(MODULE_CONN_TCP4) || defined(MODULE_CONN_TCP6)
        case SOCK_STREAM:
            if ((protocol == 0) || (protocol == IPPROTO_TCP)) {
                return protocol;
            }
            else {
                errno = EPROTOTYPE;
            }
            break;
#endif
#if defined(MODULE_CONN_UDP4) || defined(MODULE_CONN_UDP6)
        case SOCK_DGRAM:
            if ((protocol == 0) || (protocol == IPPROTO_UDP)) {
                return protocol;
            }
            else {
                errno = EPROTOTYPE;
            }
            break;
#endif
#if defined(MODULE_CONN_IP4) || defined(MODULE_CONN_IP6)
        case SOCK_RAW:
            return protocol;
#endif
        default:
            (void)protocol;
            break;
    }
    errno = EPROTONOSUPPORT;
    return -1;
}

#if CONN_IPV4
static inline ipv4_addr_t *_in_addr_ptr(struct sockaddr_storage *addr)
{
    return (ipv4_addr_t *)(&((struct sockaddr_in *)addr)->sin_addr);
}

static inline uint16_t *_in_port_ptr(struct sockaddr_storage *addr)
{
    return &((struct sockaddr_in *)addr)->sin_port;
}

static inline void _in_htons_port(struct sockaddr_storage *addr)
{
    struct sockaddr_in *tmp = (struct sockaddr_in *)addr;
    tmp->sin_port = htons(tmp->sin_port);
}
#endif

#if CONN_IPV6
static inline ipv6_addr_t *_in6_addr_ptr(struct sockaddr_storage *addr)
{
    return (ipv6_addr_t *)(&((struct sockaddr_in6 *)addr)->sin6_addr);
}

static inline uint16_t *_in6_port_ptr(struct sockaddr_storage *addr)
{
    return &((struct sockaddr_in6 *)addr)->sin6_port;
}

static inline void _in6_htons_port(struct sockaddr_storage *addr)
{
    struct sockaddr_in6 *tmp = (struct sockaddr_in6 *)addr;
    tmp->sin6_port = htons(tmp->sin6_port);
}
#endif

#if CONN_IPV4 || CONN_IPV6
static inline socklen_t _addr_truncate(struct sockaddr *out, socklen_t out_len,
                                       struct sockaddr_storage *in, socklen_t target_size)
{
    out_len = (out_len < target_size) ? out_len : target_size;
    memcpy(out, in, out_len);
    return out_len;
}
#endif

static int socket_close(int socket)
{
    socket_t *s;
    int res = 0;
    if ((unsigned)(socket - 1) > (SOCKET_POOL_SIZE - 1)) {
        return -1;
    }
    mutex_lock(&_pool_mutex);
    s = &_pool[socket];
    s->domain = AF_INET;
    switch (s->domain) {
#if CONN_IPV4
        case AF_INET:
            switch (s->type) {
#ifdef MODULE_CONN_UDP4
                case SOCK_DGRAM:
                    conn_udp4_close(&s->conn.udp4);
                    break;
#endif
#ifdef MODULE_CONN_IP4
                case SOCK_RAW:
                    conn_ip4_close(&s->conn.raw4);
                    break;
#endif
#ifdef MODULE_CONN_TCP4
                case SOCK_STREAM:
                    conn_tcp4_close(&s->conn.tcp4);
                    break;
#endif
                default:
                    res = -1;
                    break;
            }
#endif
#if CONN_IPV6
        case AF_INET6:
            switch (s->type) {
#ifdef MODULE_CONN_UDP6
                case SOCK_DGRAM:
                    conn_udp6_close(&s->conn.udp6);
                    break;
#endif
#ifdef MODULE_CONN_IP6
                case SOCK_RAW:
                    conn_ip6_close(&s->conn.raw6);
                    break;
#endif
#ifdef MODULE_CONN_TCP6
                case SOCK_STREAM:
                    conn_tcp6_close(&s->conn.tcp6);
                    break;
#endif
                default:
                    res = -1;
                    break;
            }
#endif
        default:
            res = -1;
            break;
    }
    mutex_unlock(&_pool_mutex);
    return res;
}

static inline ssize_t socket_read(int socket, void *buf, size_t n)
{
    return recv(socket, buf, n, 0);
}

static inline ssize_t socket_write(int socket, const void *buf, size_t n)
{
    return send(socket, buf, n, 0);
}

int socket(int domain, int type, int protocol)
{
    int res = 0;
    socket_t *s;
    mutex_lock(&_pool_mutex);
    s = _get_free_socket();
    if (s == NULL) {
        errno = ENFILE;
        mutex_unlock(&_pool_mutex);
        return -1;
    }
    switch (domain) {
#if CONN_IPV4
        case AF_INET:
#endif
#if CONN_IPV6
        case AF_INET6:
#endif
#if CONN_IPV4 || CONN_IPV6
            s->domain = domain;
            s->type = type;
            if ((s->protocol = _choose_ipproto(type, protocol)) < 0) {
                res = -1;
            }
            break;
#endif
        default:
            (void)type;
            (void)protocol;
            errno = EAFNOSUPPORT;
            res = -1;
    }
    if (res == 0) {
        /* TODO: add read and write */
        int fd = fd_new(s - _pool, socket_read, socket_write, socket_close);
        if (fd < 0) {
            errno = ENFILE;
            res = -1;
        }
        else {
            s->fd = res = fd;
        }
    }
    mutex_unlock(&_pool_mutex);
    return res;
}


int accept(int socket, struct sockaddr *restrict address,
           socklen_t *restrict address_len)
{
    socket_t *s, *new_s = NULL;
    int res = 0;
    /* May be kept unassigned if no conn module is available */
    /* cppcheck-suppress unassignedVariable */
    struct sockaddr_storage tmp;
    mutex_lock(&_pool_mutex);
    s = _get_socket(socket);
    if (s == NULL) {
        mutex_unlock(&_pool_mutex);
        errno = ENOTSOCK;
        return -1;
    }
    switch (s->domain) {
#if CONN_IPV4
        case AF_INET:
            switch (s->type) {
#ifdef MODULE_CONN_TCP4
                case SOCK_STREAM:
                    new_s = _get_free_socket();
                    if (new_s == NULL) {
                        errno = ENFILE;
                        res = -1;
                        break;
                    }
                    if ((res = conn_tcp4_accept(&s->conn.tcp4, &new_s->conn.tcp4)) < 0) {
                        errno = -res;
                        res = -1;
                        break;
                    }
                    else if ((address != NULL) && (address_len != NULL)) {
                        /* TODO: add read and write */
                        int fd = fd_new(new_s - _pool, NULL, NULL, socket_close);
                        if (fd < 0) {
                            errno = ENFILE;
                            res = -1;
                            break;
                        }
                        else {
                            new_s->fd = res = fd;
                        }
                        new_s->domain = s->domain;
                        new_s->type = s->type;
                        new_s->protocol = s->protocol;
                        tmp.ss_family = AF_INET;
                        if ((res = conn_tcp4_getpeeraddr(&s->conn.tcp4, _in_addr_ptr(&tmp),
                                                         _in_port_ptr(&tmp))) < 0) {
                            errno = -res;
                            res = -1;
                            break;
                        }
                        _in_htons_port(&tmp); /* XXX: sin_port is supposed to be network byte
                                               *      order */
                        *address_len = _addr_truncate(address, *address_len, &tmp,
                                                      sizeof(struct sockaddr_in));
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    res = -1;
                    break;
            }
            break;
#endif
#if CONN_IPV6
        case AF_INET6:
            if ((address_len != NULL) && (*address_len < sizeof(struct sockaddr_in6))) {
                errno = ENOMEM;
                res = -1;
                break;
            }
            switch (s->type) {
#ifdef MODULE_CONN_TCP6
                case SOCK_STREAM:
                    new_s = _get_free_socket();
                    if (new_s == NULL) {
                        errno = ENFILE;
                        res = -1;
                        break;
                    }
                    if ((res = conn_tcp6_accept(&s->conn.tcp6, &new_s->conn.tcp6)) < 0) {
                        errno = -res;
                        res = -1;
                        break;
                    }
                    else if ((address != NULL) && (address_len != NULL)) {
                        /* TODO: add read and write */
                        int fd = fd_new(new_s - _pool, NULL, NULL, socket_close);
                        if (fd < 0) {
                            errno = ENFILE;
                            res = -1;
                            break;
                        }
                        else {
                            new_s->fd = res = fd;
                        }
                        new_s->domain = s->domain;
                        new_s->type = s->type;
                        new_s->protocol = s->protocol;
                        *address_len = sizeof(struct sockaddr_in6);
                        tmp.ss_family = AF_INET6;
                        if ((res = conn_tcp6_getpeeraddr(&s->conn.tcp6, _in6_addr_ptr(&tmp),
                                                         _in6_port_ptr(&tmp))) < 0) {
                            errno = -res;
                            res = -1;
                            break;
                        }
                        _in6_htons_port(&tmp);  /* XXX: sin_port6 is supposed to be network byte
                                                 *      order */
                        *address_len = _addr_truncate(address, *address_len, &tmp,
                                                      sizeof(struct sockaddr_in6));
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    res = -1;
                    break;
            }
            break;
#endif
        default:
            (void)address;
            (void)address_len;
            (void)new_s;
            (void)tmp;
            errno = EPROTO;
            res = -1;
            break;
    }
    mutex_unlock(&_pool_mutex);
    return res;
}

int bind(int socket, const struct sockaddr *address, socklen_t address_len)
{
    socket_t *s;
    int res = 0;
    mutex_lock(&_pool_mutex);
    s = _get_socket(socket);
    mutex_unlock(&_pool_mutex);
    if (s == NULL) {
        errno = ENOTSOCK;
        return -1;
    }
    switch (s->domain) {
#if CONN_IPV4
        case AF_INET:
            if (address->sa_family != AF_INET) {
                errno = EAFNOSUPPORT;
                return -1;
            }
            if (address_len < sizeof(struct sockaddr_in)) {
                errno = EINVAL;
                return -1;
            }
            struct sockaddr_in *in_addr = (struct sockaddr_in *)address;
            switch (s->type) {
#ifdef MODULE_CONN_IP4
                case SOCK_RAW:
                    if ((res = conn_ip4_create(&s->conn.raw4, (ipv4_addr_t *)&in_addr->sin_addr,
                                               s->protocol)) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_TCP4
                case SOCK_STREAM:
                    /* XXX sin_port is in network byteorder */
                    if ((res = conn_tcp4_create(&s->conn.tcp4, (ipv4_addr_t *)&in_addr->sin_addr,
                                                ntohs(in_addr->sin_port))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_UDP4
                case SOCK_DGRAM:
                    /* XXX sin_port is in network byteorder */
                    if ((res = conn_udp4_create(&s->conn.udp4, (ipv4_addr_t *)&in_addr->sin_addr,
                                                ntohs(in_addr->sin_port))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    (void)in_addr;
                    errno = EOPNOTSUPP;
                    return -1;
            }
            break;
#endif
#if CONN_IPV6
        case AF_INET6:
            if (address->sa_family != AF_INET6) {
                errno = EAFNOSUPPORT;
                return -1;
            }
            if (address_len < sizeof(struct sockaddr_in6)) {
                errno = EINVAL;
                return -1;
            }
            struct sockaddr_in6 *in6_addr = (struct sockaddr_in6 *)address;
            switch (s->type) {
#ifdef MODULE_CONN_IP6
                case SOCK_RAW:
                    if ((res = conn_ip6_create(&s->conn.raw6, (ipv6_addr_t *)&in6_addr->sin6_addr,
                                               s->protocol)) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_TCP6
                case SOCK_STREAM:
                    /* XXX sin6_port is in network byteorder */
                    if ((res = conn_tcp6_create(&s->conn.tcp6, (ipv6_addr_t *)&in6_addr->sin6_addr,
                                                ntohs(in6_addr->sin6_port))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_UDP6
                case SOCK_DGRAM:
                    /* XXX sin6_port is in network byteorder */
                    if ((res = conn_udp6_create(&s->conn.udp6, (ipv6_addr_t *)&in6_addr->sin6_addr,
                                                ntohs(in6_addr->sin6_port))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    (void)in6_addr;
                    errno = EOPNOTSUPP;
                    return -1;
            }
            break;
#endif
        default:
            (void)address;
            (void)address_len;
            (void)res;
            errno = EAFNOSUPPORT;
            return -1;
    }
    return 0;
}

int connect(int socket, const struct sockaddr *address, socklen_t address_len)
{
    socket_t *s;
    int res = 0;
    mutex_lock(&_pool_mutex);
    s = _get_socket(socket);
    mutex_unlock(&_pool_mutex);
    if (s == NULL) {
        errno = ENOTSOCK;
        return -1;
    }
    switch (s->domain) {
#if CONN_IPV4
        case AF_INET:
            if (address->sa_family != AF_INET) {
                errno = EAFNOSUPPORT;
                return -1;
            }
            if (address_len < sizeof(struct sockaddr_in)) {
                errno = EINVAL;
                return -1;
            }
            struct sockaddr_in *in_addr = (struct sockaddr_in *)address;
            switch (s->type) {
#ifdef MODULE_CONN_TCP4
                case SOCK_STREAM:
                    /* XXX sin_port is in network byteorder */
                    if ((res = conn_tcp4_connect(&s->conn.tcp4, (ipv4_addr_t *)&in_addr->sin_addr,
                                                 ntohs(in_addr->sin_port))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    (void)in_addr;
                    errno = EPROTOTYPE;
                    return -1;
            }
            break;
#endif
#if CONN_IPV6
        case AF_INET6:
            if (address->sa_family != AF_INET6) {
                errno = EAFNOSUPPORT;
                return -1;
            }
            if (address_len < sizeof(struct sockaddr_in6)) {
                errno = EINVAL;
                return -1;
            }
            struct sockaddr_in6 *in6_addr = (struct sockaddr_in6 *)address;
            switch (s->type) {
#ifdef MODULE_CONN_TCP6
                case SOCK_STREAM:
                    /* XXX sin6_port is in network byteorder */
                    if ((res = conn_tcp6_connect(&s->conn.tcp6, (ipv6_addr_t *)&in6_addr->sin6_addr,
                                                 ntohs(in6_addr->sin6_port))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    (void)in6_addr;
                    errno = EPROTOTYPE;
                    return -1;
            }
            break;
#endif
        default:
            (void)address;
            (void)address_len;
            (void)res;
            errno = EAFNOSUPPORT;
            return -1;
    }
    return 0;
}

int getpeername(int socket, struct sockaddr *__restrict address,
                socklen_t *__restrict address_len)
{
    socket_t *s;
    int res = 0;
    /* May be kept unassigned if no conn module is available */
    /* cppcheck-suppress unassignedVariable */
    struct sockaddr_storage tmp;
    mutex_lock(&_pool_mutex);
    s = _get_socket(socket);
    mutex_unlock(&_pool_mutex);
    if (s == NULL) {
        errno = ENOTSOCK;
        return -1;
    }
    switch (s->domain) {
#if CONN_IPV4
        case AF_INET:
            switch (s->type) {
#ifdef MODULE_CONN_TCP4
                case SOCK_STREAM:
                    if ((res = conn_tcp4_getpeeraddr(&s->conn.tcp4, _in_addr_ptr(&tmp),
                                                     _in_port_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    return -1;
            }
            tmp.ss_family = AF_INET;
            _in_htons_port(&tmp); /* XXX: sin_port is supposed to be network byte
                                   *      order */
            *address_len = _addr_truncate(address, *address_len, &tmp, sizeof(struct sockaddr_in));
            break;
#endif
#if CONN_IPV6
        case AF_INET6:
            switch (s->type) {
#ifdef MODULE_CONN_TCP6
                case SOCK_STREAM:
                    if ((res = conn_tcp6_getpeeraddr(&s->conn.tcp6, _in6_addr_ptr(&tmp),
                                                     _in6_port_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    return -1;
            }
            tmp.ss_family = AF_INET6;
            _in6_htons_port(&tmp); /* XXX: sin_port6 is supposed to be network byte
                                    *      order */
            *address_len = _addr_truncate(address, *address_len, &tmp,
                                          sizeof(struct sockaddr_in6));
            break;
#endif
        default:
            (void)address;
            (void)address_len;
            (void)tmp;
            (void)res;
            errno = EAFNOSUPPORT;
            return -1;
    }
    return 0;
}

int getsockname(int socket, struct sockaddr *__restrict address,
                socklen_t *__restrict address_len)
{
    socket_t *s;
    int res = 0;
    /* May be kept unassigned if no conn module is available */
    /* cppcheck-suppress unassignedVariable */
    struct sockaddr_storage tmp;
    mutex_lock(&_pool_mutex);
    s = _get_socket(socket);
    mutex_unlock(&_pool_mutex);
    if (s == NULL) {
        errno = ENOTSOCK;
        return -1;
    }
    switch (s->domain) {
#if CONN_IPV4
        case AF_INET:
            switch (s->type) {
#ifdef MODULE_CONN_UDP4
                case SOCK_DGRAM:
                    if ((res = conn_udp4_getlocaladdr(&s->conn.udp4, _in_addr_ptr(&tmp),
                                                      _in_port_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_IP4
                case SOCK_RAW:
                    if ((res = conn_ip4_getlocaladdr(&s->conn.raw4, _in_addr_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_TCP4
                case SOCK_STREAM:
                    if ((res = conn_tcp4_getlocaladdr(&s->conn.tcp4, _in_addr_ptr(&tmp),
                                                      _in_port_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    return -1;
            }
            tmp.ss_family = AF_INET;
            _in_htons_port(&tmp); /* XXX: sin_port is supposed to be network byte
                                   *      order */
            *address_len = _addr_truncate(address, *address_len, &tmp, sizeof(struct sockaddr_in));
            break;
#endif
#if CONN_IPV6
        case AF_INET6:
            switch (s->type) {
#ifdef MODULE_CONN_UDP6
                case SOCK_DGRAM:
                    if ((res = conn_udp6_getlocaladdr(&s->conn.udp6, _in6_addr_ptr(&tmp),
                                                      _in6_port_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_IP6
                case SOCK_RAW:
                    if ((res = conn_ip6_getlocaladdr(&s->conn.raw6, _in6_addr_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_TCP6
                case SOCK_STREAM:
                    if ((res = conn_tcp6_getlocaladdr(&s->conn.tcp6, _in6_addr_ptr(&tmp),
                                                      _in6_port_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    return -1;
            }
            tmp.ss_family = AF_INET6;
            _in6_htons_port(&tmp); /* XXX: sin_port6 is supposed to be network byte
                                    *      order */
            *address_len = _addr_truncate(address, *address_len, &tmp,
                                          sizeof(struct sockaddr_in6));
            break;
#endif
        default:
            (void)address;
            (void)address_len;
            (void)tmp;
            (void)res;
            errno = EAFNOSUPPORT;
            return -1;
    }
    return 0;
}

int listen(int socket, int backlog)
{
    socket_t *s;
    int res = 0;
    mutex_lock(&_pool_mutex);
    s = _get_socket(socket);
    mutex_unlock(&_pool_mutex);
    switch (s->domain) {
#if CONN_IPV4
        case AF_INET:
            switch (s->type) {
#ifdef MODULE_CONN_TCP4
                case SOCK_STREAM:
                    if ((res = conn_tcp4_listen(&s->conn.tcp4, backlog)) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    return -1;
            }
            break;
#endif
#if CONN_IPV6
        case AF_INET6:
            switch (s->type) {
#ifdef MODULE_CONN_TCP6
                case SOCK_STREAM:
                    if ((res = conn_tcp6_listen(&s->conn.tcp6, backlog)) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    return -1;
            }
            break;
#endif
        default:
            (void)backlog;
            (void)res;
            errno = EAFNOSUPPORT;
            return -1;
    }
    return 0;
}

ssize_t recv(int socket, void *buffer, size_t length, int flags)
{
    return recvfrom(socket, buffer, length, flags, NULL, NULL);
}

ssize_t recvfrom(int socket, void *restrict buffer, size_t length, int flags,
                 struct sockaddr *restrict address,
                 socklen_t *restrict address_len)
{
    socket_t *s;
    int res = 0;
    /* May be kept unassigned if no conn module is available */
    /* cppcheck-suppress unassignedVariable */
    struct sockaddr_storage tmp;
    (void)flags;
    mutex_lock(&_pool_mutex);
    s = _get_socket(socket);
    mutex_unlock(&_pool_mutex);
    if (s == NULL) {
        errno = ENOTSOCK;
        return -1;
    }
    switch (s->domain) {
#if CONN_IPV4
        case AF_INET:
            switch (s->type) {
#ifdef MODULE_CONN_UDP4
                case SOCK_DGRAM:
                    if ((res = conn_udp4_recvfrom(&s->conn.udp4, buffer, length,
                                                  _in_addr_ptr(&tmp), _in_port_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_IP4
                case SOCK_RAW:
                    if ((res = conn_ip4_recvfrom(&s->conn.raw4, buffer, length,
                                                 _in_addr_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_TCP4
                case SOCK_STREAM:
                    if ((res = conn_tcp4_recv(&s->conn.udp4, buffer, length)) < 0) {
                        errno = -res;
                        return -1;
                    }
                    if ((res = conn_tcp4_getpeeraddr(&s->conn.tcp4, _in_addr_ptr(&tmp),
                                                     _in_port_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    return -1;
            }
            if ((address != NULL) && (address_len != NULL)) {
                tmp.ss_family = AF_INET;
                _in_htons_port(&tmp); /* XXX: sin_port is supposed to be network byte
                                       *      order */
                *address_len = _addr_truncate(address, *address_len, &tmp,
                                              sizeof(struct sockaddr_in));
            }
            break;
#endif
#if CONN_IPV6
        case AF_INET6:
            switch (s->type) {
#ifdef MODULE_CONN_UDP6
                case SOCK_DGRAM:
                    if ((res = conn_udp6_recvfrom(&s->conn.udp6, buffer, length,
                                                  _in6_addr_ptr(&tmp), _in6_port_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_IP6
                case SOCK_RAW:
                    if ((res = conn_ip6_recvfrom(&s->conn.raw6, buffer, length,
                                                 _in6_addr_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_TCP6
                case SOCK_STREAM:
                    if ((res = conn_tcp6_recv(&s->conn.udp6, buffer, length)) < 0) {
                        errno = -res;
                        return -1;
                    }
                    if ((res = conn_tcp6_getpeeraddr(&s->conn.tcp6, _in6_addr_ptr(&tmp),
                                                     _in6_port_ptr(&tmp))) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    return -1;
            }
            if ((address != NULL) && (address_len != NULL)) {
                tmp.ss_family = AF_INET6;
                _in6_htons_port(&tmp); /* XXX: sin6_port is supposed to be network byte
                                        *      order */
                *address_len = _addr_truncate(address, *address_len, &tmp,
                                              sizeof(struct sockaddr_in));
            }
            break;
#endif
        default:
            (void)buffer;
            (void)length;
            (void)address;
            (void)address_len;
            (void)tmp;
            errno = EAFNOSUPPORT;
            return -1;
    }
    return res;
}

ssize_t send(int socket, const void *buffer, size_t length, int flags)
{
    return sendto(socket, buffer, length, flags, NULL, 0);
}

ssize_t sendto(int socket, const void *buffer, size_t length, int flags,
               const struct sockaddr *address, socklen_t address_len)
{
    socket_t *s;
    int res = 0;
    (void)flags;
    mutex_lock(&_pool_mutex);
    s = _get_socket(socket);
    mutex_unlock(&_pool_mutex);
    if (s == NULL) {
        errno = ENOTSOCK;
        return -1;
    }
    switch (s->domain) {
#if CONN_IPV4
        case AF_INET:
            switch (s->type) {
#ifdef MODULE_CONN_IP4
                case SOCK_RAW:
                    if (address != NULL) {
                        struct sockaddr_in *in_addr = (struct sockaddr_in *)address;
                        if (address->sa_family != AF_INET) {
                            errno = EAFNOSUPPORT;
                            return -1;
                        }
                        if (address_len < sizeof(struct sockaddr_in)) {
                            errno = EINVAL;
                            return -1;
                        }
                        res = conn_ip4_sendto(&s->conn.raw4, buffer, length,
                                              (ipv4_addr_t *)&in_addr->sin_addr);
                    }
                    else {
                        res = conn_ip4_sendto(&s->conn.raw4, buffer, length, NULL);
                    }
                    if (res < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_TCP4
                case SOCK_STREAM:
                    /* XXX sin_port is in network byteorder */
                    if ((res = conn_tcp4_send(&s->conn.tcp4, buffer, length)) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_UDP4
                case SOCK_DGRAM:
                    if (address != NULL) {
                        struct sockaddr_in *in_addr = (struct sockaddr_in *)address;
                        if (address->sa_family != AF_INET) {
                            errno = EAFNOSUPPORT;
                            return -1;
                        }
                        if (address_len < sizeof(struct sockaddr_in)) {
                            errno = EINVAL;
                            return -1;
                        }
                        /* XXX sin_port is in network byteorder */
                        res = conn_udp4_sendto(&s->conn.udp4, buffer, length,
                                               (ipv4_addr_t *)&in_addr->sin_addr,
                                               ntohs(in_addr->sin_port));
                    }
                    else {
                        res = conn_udp4_sendto(&s->conn.udp4, buffer, length, NULL, 0);
                    }
                    if (res < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    return -1;
            }
            break;
#endif
#if CONN_IPV6
        case AF_INET6:
            switch (s->type) {
#ifdef MODULE_CONN_IP6
                case SOCK_RAW:
                    if (address != NULL) {
                        struct sockaddr_in6 *in6_addr = (struct sockaddr_in6 *)address;
                        if (address->sa_family != AF_INET6) {
                            errno = EAFNOSUPPORT;
                            return -1;
                        }
                        if (address_len < sizeof(struct sockaddr_in)) {
                            errno = EINVAL;
                            return -1;
                        }
                        res = conn_ip6_sendto(&s->conn.raw6, buffer, length,
                                              (ipv6_addr_t *)&in6_addr->sin6_addr);
                    }
                    else {
                        res = conn_ip6_sendto(&s->conn.raw6, buffer, length, NULL);
                    }
                    if (res < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_TCP6
                case SOCK_STREAM:
                    /* XXX sin6_port is in network byteorder */
                    if ((res = conn_tcp6_send(&s->conn.tcp6, buffer, length)) < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
#ifdef MODULE_CONN_UDP6
                case SOCK_DGRAM:
                    if (address != NULL) {
                        struct sockaddr_in6 *in6_addr = (struct sockaddr_in6 *)address;
                        if (address->sa_family != AF_INET6) {
                            errno = EAFNOSUPPORT;
                            return -1;
                        }
                        if (address_len < sizeof(struct sockaddr_in)) {
                            errno = EINVAL;
                            return -1;
                        }
                        /* XXX sin6_port is in network byteorder */
                        res = conn_udp6_sendto(&s->conn.udp6, buffer, length,
                                               (ipv6_addr_t *)&in6_addr->sin6_addr,
                                               ntohs(in6_addr->sin6_port));
                    }
                    else {
                        res = conn_udp6_sendto(&s->conn.udp6, buffer, length, NULL, 0);
                    }
                    if (res < 0) {
                        errno = -res;
                        return -1;
                    }
                    break;
#endif
                default:
                    errno = EOPNOTSUPP;
                    return -1;
            }
            break;
#endif
        default:
            (void)buffer;
            (void)length;
            (void)address;
            (void)address_len;
            errno = EAFNOSUPPORT;
            return -1;
    }
    return res;
}


/**
 * @}
 */
