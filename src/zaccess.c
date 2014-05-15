#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zaccess.h"
#include "zlog.h"

static int zimg_atoi(u_char *line, size_t n);
static u_char * zimg_strlchr(u_char *p, u_char *last, u_char c);
static in_addr_t zimg_inet_addr(u_char *text, size_t len);
static int stor(u_char *text, size_t text_len, zimg_rule_t *rule);
zimg_access_conf_t * conf_get_rules(const char *acc_str);
int zimg_access_inet(zimg_access_conf_t *cf, in_addr_t addr);
void free_access_conf(zimg_access_conf_t *cf);

static int zimg_atoi(u_char *line, size_t n)
{
    int value;

    if (n == 0) {
        return ZIMG_ERROR;
    }

    for (value = 0; n--; line++) {
        if (*line < '0' || *line > '9') {
            return ZIMG_ERROR;
        }

        value = value * 10 + (*line - '0');
    }

    if (value < 0) {
        return ZIMG_ERROR;

    } else {
        return value;
    }
}

static u_char * zimg_strlchr(u_char *p, u_char *last, u_char c)
{
    while (p < last) {

        if (*p == c) {
            return p;
        }

        p++;
    }

    return NULL;
}

//transfer "10.77.121.137" to in_addr_t
static in_addr_t zimg_inet_addr(u_char *text, size_t len)
{
    u_char *p, c;
    in_addr_t addr;
    uint octet, n;

    addr = 0;
    octet = 0;
    n = 0;

    for (p = text; p < text + len; p++) {

        c = *p;

        if (c >= '0' && c <= '9') {
            octet = octet * 10 + (c - '0');
            continue;
        }
        if (c == '.' && octet < 256) {
            addr = (addr << 8) + octet;
            octet = 0;
            n++;
            continue;
        }
        return INADDR_NONE;
    }

    if (n == 3 && octet < 256) {
        addr = (addr << 8) + octet;
        return htonl(addr);
    }
    return INADDR_NONE;
}

static int stor(u_char *text, size_t text_len, zimg_rule_t *rule)
{
    u_char *addr, *mask, *last;
    size_t len;
    int shift;
    addr = text;
    last = addr + text_len;

    mask = zimg_strlchr(addr, last, '/');
    len = (mask ? mask : last) - addr;

    rule->addr = zimg_inet_addr(addr, len);

    if (rule->addr != INADDR_NONE) {

        if (mask == NULL) {
            rule->mask = 0xffffffff;
            return ZIMG_OK;
        }
    } else {
        return ZIMG_ERROR;
    }

    mask++;

    shift = zimg_atoi(mask, last - mask);
    if (shift == ZIMG_ERROR) {
        return ZIMG_ERROR;
    }
    if (shift > 32) {
        return ZIMG_ERROR;
    }
    if (shift) {
        rule->mask = htonl((uint32_t) (0xffffffffu << (32 - shift)));
    } else {
        /*
        * x86 compilers use a shl instruction that
        * shifts by modulo 32
        */
        rule->mask = 0;
    }

    if (rule->addr == (rule->addr & rule->mask)) {
        return ZIMG_OK;
    }
    rule->addr &= rule->mask;

    return ZIMG_OK;
}

zimg_access_conf_t * conf_get_rules(const char *acc_str)
{
    if(acc_str == NULL)
        return NULL;
    zimg_access_conf_t *acconf = (zimg_access_conf_t *)malloc(sizeof(zimg_access_conf_t));
    if(acconf == NULL)
        return NULL;
    acconf->n = 0;
    acconf->rules = NULL;
    size_t acc_len = strlen(acc_str); 
    char *acc = (char *)malloc(acc_len);
    if(acc == NULL)
    {
        return NULL;
    }
    strcpy(acc, acc_str);
    char *start = acc;
    char *end = strchr(start, ';');
    end = (end) ? end : start+acc_len;
    while(end)
    {
        char *mode = start;
        char *range = strchr(mode, ' ');
        if (range)
        {
            zimg_rule_t *this_rule = (zimg_rule_t *)malloc(sizeof(zimg_rule_t));
            if (this_rule == NULL) {
                start = end + 1;
                end = strchr(start, ';');
                continue;
            }
            (void) memset(this_rule, 0, sizeof(zimg_rule_t));
            this_rule->deny = (mode[0] == 'd') ? 1 : 0;
            size_t range_len;
            range++;
            range_len = end - range;

            uint all = (range_len == 3 && strstr(range, "all") == range);

            if (!all) {
                int rc;
                rc = stor((unsigned char *)range, range_len, this_rule);
                if (rc == ZIMG_ERROR) {
                    start = end + 1;
                    end = strchr(start, ';');
                    continue;
                }
            }

            zimg_rules_t *rules = (zimg_rules_t *)malloc(sizeof(zimg_rules_t));
            if (rules == NULL) {
                start = end + 1;
                end = strchr(start, ';');
                continue;
            }

            rules->value= this_rule;
            rules->next = acconf->rules;
            acconf->rules = rules;
            acconf->n++;
        }
        start = end + 1;
        end = strchr(start, ';');
    }
    free(acc);
    return acconf;
}

//judge request by conf
int zimg_access_inet(zimg_access_conf_t *cf, in_addr_t addr)
{
    zimg_rules_t *rules = cf->rules;
    LOG_PRINT(LOG_DEBUG, "rules: %p", rules);

    while(rules)
    {
        LOG_PRINT(LOG_DEBUG, "addr: %d", addr);
        LOG_PRINT(LOG_DEBUG, "rules->value->addr: %d", rules->value->addr);
        LOG_PRINT(LOG_DEBUG, "rules->value->mask: %d", rules->value->mask);
        if ((addr & rules->value->mask) == rules->value->addr) {
            if (rules->value->deny) {
                return ZIMG_FORBIDDEN;
            }
            return ZIMG_OK;
        }
        rules = rules->next;
    }

    return ZIMG_FORBIDDEN;
}


void free_access_conf(zimg_access_conf_t *cf)
{
    if(cf == NULL)
        return;
    zimg_rules_t *rules = cf->rules;
    while(rules)
    {
        cf->rules = rules->next;
        free(rules->value);
        free(rules);
        rules = cf->rules;
    }
    free(cf);
}

