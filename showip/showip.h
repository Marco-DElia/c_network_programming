#ifndef SHOWIP_H
#define SHOWIP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>





#define ERR_VERB_LEN 129

struct error {

    int   err_value;
    char  err_verb[ERR_VERB_LEN];
};

void set_err(struct error *err, int value, const char *verb)
{
    u_int8_t len;

    if(err == NULL)
    {
        fprintf(stderr, "Fatal error: error struct pointer is null. LINE: %d\n", __LINE__);
        return;
    }

    err->err_value = value;

    if(verb == NULL)
    {
        memcpy(err->err_verb, "No error msg", 13);
        return;
    }

    if( (len = strlen(verb)) >= ERR_VERB_LEN )
        len = ERR_VERB_LEN - 1;

    memcpy(err->err_verb, verb, len);
    err->err_verb[len] = '\0';
}

void print_err(struct error err)
{
    printf("@Error: ");
    printf("value- > %d, ", err.err_value);
    printf("verb -> %s\n" , err.err_verb);
}













struct ip_unspec {

    char  iu_version[5];
    char *iu_addr;
};

struct ip_pool {

    u_int8_t          ip_nv4;
    u_int8_t          ip_nv6;
    struct ip_unspec *ip_iu;
};



#define free_ip_pool(pool, c_pool) pool == NULL ? free(c_pool) : free(c_pool->ip_iu)



struct ip_pool *fill_ip_pool(struct ip_pool *pool, char *node, struct error *err)
{
    if(node == NULL)
    {
        set_err(err, 1, "Null node name");
        return NULL;
    }

    char ipstr[INET6_ADDRSTRLEN];
    struct addrinfo hints, *res, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if(getaddrinfo(node, NULL, &hints, &res) != 0)
    {
        set_err(err, 2, "getaddrinfo failed");
        return NULL;
    }

    //count bytes to allocate
    u_int32_t allbytes = 0;

    {

        void               *addr;
        struct sockadd_in  *ipv4;
        struct sockadd_in6 *ipv6;

        for(p = res; p != NULL; p = p->ai_next)
        {
            if(p->ai_family == AF_INET)
            {
                ipv4 = (struct sockadd_in *)p->ai_addr;
                addr = &(ipv4->sin_addr);
            }
            else
            {
                ipv6 = (struct sockadd_in6 *)p->ai_addr;
                addr = &(ipv6->sin6_addr);
            }

            if(inet_ntop(p->ai_family, addr, ipstr, INET6_ADDRSTRLEN) == NULL)
            {
                set_err(err, 3, "inet_ntop returned null pointer");
                freeaddrinfo(res);

                return NULL;
            }

            allbytes += 5 + strlen(ipstr);
        }

    }
    //got allbytes


    /*
        If is not provided a ip_pool allocate full ip_pool,
        otherwire allocare just ip_iu.

        NOTE:
        The caller must care to free the correct memory,
        depending on the provided parameter.
    */

   struct ip_pool *c_pool = pool;

   {

        if(c_pool == NULL)
        {
            c_pool = (struct ip_pool *)malloc(sizeof c_pool + allbytes);

            if(c_pool == NULL)
            {
                set_err(err, 4, "pool allocation failed");
                freeaddrinfo(res);

                return NULL;
            }

            c_pool->ip_iu = (struct ip_unspec *)(c_pool + 1);
        }
        else
        {
            c_pool->ip_iu = (struct ip_unspec *)malloc(allbytes);

            if(c_pool->ip_iu == NULL)
            {
                set_err(err, 5, "pool->ip_iu allocation failed");
                freeaddrinfo(res);

                return NULL;
            }
        }

   }

    //fill the pool
    {
        void               *addr;
        struct ip_unspec   *temp = c_pool->ip_iu;
        struct sockadd_in  *ipv4;
        struct sockadd_in6 *ipv6;
        
        for(p = res; p != NULL; p = p->ai_next)
        {
            if(p->ai_family == AF_INET)
            {
                c_pool->ip_nv4++;
                memcpy(temp->iu_version, "IPv4", 5);

                ipv4 = (struct sockadd_in *)p->ai_addr;
                addr = &(ipv4->sin_addr);
            }
            else
            {
                c_pool->ip_nv6++;
                memcpy(temp->iu_version, "IPv6", 5);

                ipv6 = (struct sockadd_in6 *)p->ai_addr;
                addr = &(ipv6->sin6_addr);
            }
            
            temp = (struct ip_unspec *)((char *)temp + 5);

            if(inet_ntop(p->ai_family, addr, temp->iu_addr, INET6_ADDRSTRLEN) == NULL)
            {

                free_ip_pool(pool, c_pool);

                set_err(err, 6, "inet_ntop returned null pointer");
                freeaddrinfo(res);

                return NULL;   
            }

            temp = (struct ip_unspec *)( (char *)temp + strlen(temp->iu_addr) );
        }

    }

    freeaddrinfo(res);


    pool = c_pool;

    return pool;
}

#endif