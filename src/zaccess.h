#ifndef ZACCESS_H
#define ZACCESS_H

#include <arpa/inet.h>
#include <netinet/in.h>

#define  ZIMG_OK          0
#define  ZIMG_ERROR      -1
#define  ZIMG_FORBIDDEN  -2

#ifndef INADDR_NONE  /* Solaris */
#define INADDR_NONE  ((unsigned int) -1)
#endif


typedef struct zimg_rules_s zimg_rules_t;

typedef struct {
    in_addr_t mask;
    in_addr_t addr;
    uint deny;	/* unsigned  deny:1; */
} zimg_rule_t;

struct zimg_rules_s {
    zimg_rule_t *value;
    zimg_rules_t *next;
};

typedef struct {
    uint n;
    zimg_rules_t *rules;
} zimg_access_conf_t;

zimg_access_conf_t * conf_get_rules(const char *acc_str);
int zimg_access_inet(zimg_access_conf_t *cf, in_addr_t addr);
void free_access_conf(zimg_access_conf_t *cf);

#endif
