/**
 * \addtogroup uip6
 * @{
 */

/**
 * \file
 *         IPv6 data structures handling functions
 *         Comprises part of the Neighbor discovery (RFC 4861) 
 *         and auto configuration (RFC 4862 )state machines
 * \author Mathilde Durvy <mdurvy@cisco.com>
 * \author Julien Abeille <jabeille@cisco.com>
 */
/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <string.h>
#include <stdlib.h>
#include "lib/random.h"
#include "net/uip-nd6.h"
#include "net/uip-ds6.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",lladdr->addr[0], lladdr->addr[1], lladdr->addr[2], lladdr->addr[3],lladdr->addr[4], lladdr->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif


struct etimer uip_ds6_timer_periodic;                           /** \brief Timer for maintenance of data structures */
#if UIP_CONF_ROUTER
struct stimer uip_ds6_timer_ra;                                 /** \brief RA timer, to schedule RA sending */
static uint8_t racount;                                         /** \brief number of RA already sent */
static uint16_t rand_time;                                      /** \brief random time value for timers */
#else /* UIP_CONF_ROUTER */
struct etimer uip_ds6_timer_rs;                                 /** \brief RS timer, to schedule RS sending */
static uint8_t rscount;                                         /** \brief number of rs already sent */
#endif /* UIP_CONF_ROUTER */

/** \name "DS6" Data structures */
/** @{ */
uip_ds6_netif uip_ds6_if;                                       /** \brief The single interface */
uip_ds6_nbr uip_ds6_nbr_cache[UIP_DS6_NBR_NB];                  /** \brief Neighor cache */
uip_ds6_defrt uip_ds6_defrt_list[UIP_DS6_DEFRT_NB];             /** \brief Default rt list */
uip_ds6_prefix uip_ds6_prefix_list[UIP_DS6_PREFIX_NB];          /** \brief Prefix list */
uip_ds6_route uip_ds6_routing_table[UIP_DS6_ROUTE_NB];          /** \brief Routing table */
/** @} */

/* "full" (as opposed to pointer) ip address used in this file,  */
static uip_ipaddr_t     loc_fipaddr;  

/* Pointers used in this file */
static uip_ipaddr_t*    locipaddr;
static uip_ds6_addr*    locaddr;  
static uip_ds6_maddr*   locmaddr; 
static uip_ds6_aaddr*   locaaddr;  
static uip_ds6_prefix*  locprefix;
static uip_ds6_nbr*     locnbr;
static uip_ds6_defrt*   locdefrt;
static uip_ds6_route*   locroute;

/*---------------------------------------------------------------------------*/
void
uip_ds6_init(void) {
  PRINTF("Init of IPv6 data structures\n");
  PRINTF("%u neighbors\n%u default routers\n%u prefixes\n%u routes\n%u unicast addresses\n%u multicast addresses\n%u anycast addresses\n", UIP_DS6_NBR_NB, UIP_DS6_DEFRT_NB, UIP_DS6_PREFIX_NB, UIP_DS6_ROUTE_NB, UIP_DS6_ADDR_NB, UIP_DS6_MADDR_NB, UIP_DS6_AADDR_NB);
  for(locnbr = uip_ds6_nbr_cache; locnbr < uip_ds6_nbr_cache + UIP_DS6_NBR_NB; locnbr++) {
    locnbr->isused = 0;
  }
  for(locdefrt = uip_ds6_defrt_list; locdefrt < uip_ds6_defrt_list + UIP_DS6_DEFRT_NB; locdefrt++) {
    locdefrt->isused = 0;
  }
  for(locprefix = uip_ds6_prefix_list; locprefix < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB; locprefix++) {
    locprefix->isused = 0;
  }
  for(locaddr = uip_ds6_if.addr_list; locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; locaddr++) {
    locaddr->isused = 0;
  }
  for(locmaddr = uip_ds6_if.maddr_list; locmaddr < uip_ds6_if.maddr_list + UIP_DS6_MADDR_NB; locmaddr++) {
    locmaddr->isused = 0;
  }
  for(locaaddr = uip_ds6_if.aaddr_list; locaaddr < uip_ds6_if.aaddr_list + UIP_DS6_AADDR_NB; locaaddr++) {
    locaaddr->isused = 0;
  }
  for(locroute = uip_ds6_routing_table; locroute < uip_ds6_routing_table + UIP_DS6_ROUTE_NB; locroute++) {
    locroute->isused = 0;
  }
  
  /* Set interface parameters */ 
  uip_ds6_if.link_mtu = UIP_LINK_MTU;
  uip_ds6_if.cur_hop_limit = UIP_TTL;
  uip_ds6_if.base_reachable_time = UIP_ND6_REACHABLE_TIME;
  uip_ds6_if.reachable_time = uip_ds6_compute_reachable_time();
  uip_ds6_if.retrans_timer = UIP_ND6_RETRANS_TIMER;
  uip_ds6_if.maxdadns = UIP_ND6_DEF_MAXDADNS;

  /* Create link local address, prefix, multicast addresses, anycast addresses */
  uip_create_linklocal_prefix(&loc_fipaddr);
#if UIP_CONF_ROUTER
  uip_ds6_prefix_add(&loc_fipaddr, UIP_DEFAULT_PREFIX_LEN, 0, 0, 0, 0);
#else /* UIP_CONF_ROUTER */
  uip_ds6_prefix_add(&loc_fipaddr, UIP_DEFAULT_PREFIX_LEN, 0);
#endif /* UIP_CONF_ROUTER */
  uip_ds6_set_addr_iid(&loc_fipaddr, &uip_lladdr);
  uip_ds6_addr_add(&loc_fipaddr, 0, ADDR_AUTOCONF);

  uip_create_linklocal_allnodes_mcast(&loc_fipaddr);
  uip_ds6_maddr_add(&loc_fipaddr);
#if UIP_CONF_ROUTER
  uip_create_linklocal_allrouters_mcast(&loc_fipaddr);
  uip_ds6_maddr_add(&loc_fipaddr);
#if UIP_ND6_SEND_RA
  stimer_set(&uip_ds6_timer_ra, 2);      /* wait to have a link local IP address */    
#endif /* UIP_ND6_SEND_RA */
#else /* UIP_CONF_ROUTER */
  etimer_set(&uip_ds6_timer_rs, random_rand()%(UIP_ND6_MAX_RTR_SOLICITATION_DELAY * CLOCK_SECOND));
#endif /* UIP_CONF_ROUTER */
  etimer_set(&uip_ds6_timer_periodic, UIP_DS6_PERIOD);

  return;
}


/*---------------------------------------------------------------------------*/
void
uip_ds6_periodic(void) {
  /* Periodic processing on unicast addresses */
  for(locaddr = uip_ds6_if.addr_list; locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; locaddr++) {
    if(locaddr->isused) { 
      if((!locaddr->isinfinite) && (stimer_expired(&locaddr->vlifetime))) {
        uip_ds6_addr_rm(locaddr);
      } else if ((locaddr->state == ADDR_TENTATIVE) && (locaddr->dadnscount <= uip_ds6_if.maxdadns) && (timer_expired(&locaddr->dadtimer))) {
        uip_ds6_dad(locaddr);
      }
    }
  }

  /* Periodic processing on default routers */
  for(locdefrt = uip_ds6_defrt_list; locdefrt < uip_ds6_defrt_list + UIP_DS6_DEFRT_NB; locdefrt++) {
    if((locdefrt->isused) && (stimer_expired(&(locdefrt->lifetime)))) {
        uip_ds6_defrt_rm(locdefrt);
    }
  }

#if !UIP_CONF_ROUTER
  /* Periodic processing on prefixes */
  for(locprefix = uip_ds6_prefix_list; locprefix < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB; locprefix++) {
    if((locprefix->isused) && 
       (!locprefix->isinfinite) &&
       (stimer_expired(&(locprefix->vlifetime)))) {
      uip_ds6_prefix_rm(locprefix);
    }
  }
#endif /* !UIP_CONF_ROUTER */

  /* Periodic processing on neighbors */
  for(locnbr = uip_ds6_nbr_cache; locnbr < uip_ds6_nbr_cache + UIP_DS6_NBR_NB; locnbr++) {
    if(locnbr->isused) {
      switch (locnbr->state) {
        case NBR_INCOMPLETE:
          if(locnbr->nscount >= UIP_ND6_MAX_MULTICAST_SOLICIT) {
            uip_ds6_nbr_rm(locnbr);
          }
          else if(stimer_expired(&(locnbr->sendns))) {
            locnbr->nscount++;
            PRINTF("NBR_INCOMPLETE: NS %u\n",locnbr->nscount);
            uip_nd6_ns_output(NULL, NULL, &locnbr->ipaddr);
            stimer_set(&(locnbr->sendns), uip_ds6_if.retrans_timer / 1000);
          }
          break;
        case NBR_REACHABLE:
          if(stimer_expired(&(locnbr->reachable))) {
            PRINTF("REACHABLE: moving to STALE (");
            PRINT6ADDR(&locnbr->ipaddr);
            PRINTF(")\n");
            locnbr->state = NBR_STALE;
          }
          break;
        case NBR_DELAY:
          if(stimer_expired(&(locnbr->reachable))) {
            locnbr->state = NBR_PROBE;
            locnbr->nscount = 1;
            PRINTF("DELAY: moving to PROBE + NS %u\n", locnbr->nscount);
            uip_nd6_ns_output(NULL, &locnbr->ipaddr, &locnbr->ipaddr);
            stimer_set(&(locnbr->sendns),
                      uip_ds6_if.retrans_timer / 1000);
          }
          break;
        case NBR_PROBE:
          if(locnbr->nscount >= UIP_ND6_MAX_UNICAST_SOLICIT) {
            PRINTF("PROBE END \n");
            if((locdefrt = uip_ds6_defrt_lookup(&locnbr->ipaddr)) != NULL) {
              uip_ds6_defrt_rm(locdefrt);
            }
            uip_ds6_nbr_rm(locnbr);
          } else if(stimer_expired(&(locnbr->sendns))){
            locnbr->nscount++;
            PRINTF("PROBE: NS %u\n",locnbr->nscount);
            uip_nd6_ns_output(NULL, &locnbr->ipaddr, &locnbr->ipaddr);
            stimer_set(&(locnbr->sendns), uip_ds6_if.retrans_timer / 1000);
          }
          break;
        default:
          break;
      }
    }
  }

#if UIP_CONF_ROUTER & UIP_ND6_SEND_RA
  /* Periodic RA sending */
  if(stimer_expired(&uip_ds6_timer_ra)) {
    uip_ds6_send_ra_periodic();
  }
#endif /* UIP_CONF_ROUTER & UIP_ND6_SEND_RA */
  etimer_reset(&uip_ds6_timer_periodic);
  return;
}

/*---------------------------------------------------------------------------*/
uint8_t uip_ds6_list_loop(uip_ds6_element* list, uint8_t size, uint16_t elementsize, uip_ipaddr_t* ipaddr, uint8_t ipaddrlen, uip_ds6_element** out_element) {
  uip_ds6_element *element;
  *out_element = NULL;

  for(element = list; element < (uip_ds6_element*)((uint8_t*)list + (size * elementsize)); element = (uip_ds6_element*)((uint8_t*)element + elementsize)) {
    if(element->isused) {
      if(uip_ipaddr_prefixcmp(&(element->ipaddr), ipaddr, ipaddrlen)) {
        *out_element = element;
        return FOUND;
      }
    } else {
      *out_element = element;
    }
  }

  if(*out_element) {
    return FREESPACE;
  } else {
    return NOSPACE;
  }
}

/*---------------------------------------------------------------------------*/
uip_ds6_nbr*
uip_ds6_nbr_add(uip_ipaddr_t *ipaddr, uip_lladdr_t *lladdr, uint8_t isrouter, uint8_t state) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_nbr_cache, UIP_DS6_NBR_NB, sizeof(uip_ds6_nbr),ipaddr, 128, (uip_ds6_element**)&locnbr) == FREESPACE) {
    locnbr->isused = 1;
    uip_ipaddr_copy(&(locnbr->ipaddr), ipaddr);
    if(lladdr != NULL){
      memcpy(&(locnbr->lladdr), lladdr, UIP_LLADDR_LEN);
    } else {
      memset(&(locnbr->lladdr), 0, UIP_LLADDR_LEN);
    }
    locnbr->isrouter = isrouter;
    locnbr->state = state;
    /* timers are set separately, for now we put them in expired state */
    stimer_set(&(locnbr->reachable),0);
    stimer_set(&(locnbr->sendns),0);
    locnbr->nscount = 0;
    PRINTF("Adding neighbor with ip addr");
    PRINT6ADDR(ipaddr);
    PRINTF("link addr");
    PRINTLLADDR((&(locnbr->lladdr)));
    PRINTF("state %u\n", state);
    return locnbr;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_nbr_rm(uip_ds6_nbr* nbr) {
  if(nbr != NULL) { 
    nbr->isused = 0;
  }
  return;
}

/*---------------------------------------------------------------------------*/
uip_ds6_nbr*
uip_ds6_nbr_lookup(uip_ipaddr_t *ipaddr) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_nbr_cache, UIP_DS6_NBR_NB, sizeof(uip_ds6_nbr) ,ipaddr, 128, (uip_ds6_element**)&locnbr) == FOUND) {
    return locnbr;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
uip_ds6_defrt*
uip_ds6_defrt_add(uip_ipaddr_t *ipaddr, unsigned long interval) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_defrt_list, UIP_DS6_DEFRT_NB, sizeof(uip_ds6_defrt),ipaddr, 128, (uip_ds6_element**)&locdefrt) == FREESPACE) {
    locdefrt->isused = 1;
    uip_ipaddr_copy(&(locdefrt->ipaddr), ipaddr);
    stimer_set(&(locdefrt->lifetime), interval);
  
    PRINTF("Adding defrouter with ip addr");
    PRINT6ADDR(&locdefrt->ipaddr);
    PRINTF("\n");
    return locdefrt;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_defrt_rm(uip_ds6_defrt* defrt) {
  if(defrt != NULL) {
    defrt->isused = 0;
  }
  return;
}

/*---------------------------------------------------------------------------*/
uip_ds6_defrt*
uip_ds6_defrt_lookup(uip_ipaddr_t *ipaddr) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_defrt_list, UIP_DS6_DEFRT_NB, sizeof(uip_ds6_defrt),ipaddr, 128, (uip_ds6_element**)&locdefrt) == FOUND) {
    return locdefrt;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
uip_ipaddr_t*
uip_ds6_defrt_choose() {
  uip_ds6_nbr *bestnbr;

  locipaddr = NULL;
  for(locdefrt = uip_ds6_defrt_list; locdefrt < uip_ds6_defrt_list + UIP_DS6_DEFRT_NB; locdefrt++) {
    if(locdefrt->isused) {
      PRINTF("Defrt, IP address ");
      PRINT6ADDR(&locdefrt->ipaddr);
      PRINTF("\n");
      bestnbr = uip_ds6_nbr_lookup(&locdefrt->ipaddr);
      if((bestnbr != NULL) && (bestnbr->state != NBR_INCOMPLETE)) {
        PRINTF("Defrt found, IP address ");
        PRINT6ADDR(&locdefrt->ipaddr);
        PRINTF("\n");
        return &locdefrt->ipaddr;
      } else {
        locipaddr = &locdefrt->ipaddr;
        PRINTF("Defrt INCOMPLETE found, IP address ");
        PRINT6ADDR(&locdefrt->ipaddr);
        PRINTF("\n");
      }
    }   
  }   
  return locipaddr;
}

#if UIP_CONF_ROUTER
/*---------------------------------------------------------------------------*/
uip_ds6_prefix*
uip_ds6_prefix_add(uip_ipaddr_t *ipaddr, uint8_t ipaddrlen, uint8_t advertise, uint8_t flags, unsigned long vtime, unsigned long ptime) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_prefix_list, UIP_DS6_PREFIX_NB, sizeof(uip_ds6_prefix), ipaddr, ipaddrlen, (uip_ds6_element**)&locprefix) == FREESPACE) {
    locprefix->isused = 1;
    uip_ipaddr_copy(&(locprefix->ipaddr), ipaddr);
    locprefix->length = ipaddrlen;
    locprefix->advertise = advertise;
    locprefix->l_a_reserved = flags;
    locprefix->vlifetime = vtime;
    locprefix->plifetime = ptime;
    PRINTF("Adding prefix ");
    PRINT6ADDR(&locprefix->ipaddr);
    PRINTF("length %u, flags %x, Valid lifetime %lx, Preffered lifetime %lx\n", ipaddrlen, flags, vtime, ptime);
    return locprefix;
  } else {
    PRINTF("No more space in Prefix list\n");
  }
  return NULL;
}


#else /* UIP_CONF_ROUTER */
uip_ds6_prefix*
uip_ds6_prefix_add(uip_ipaddr_t *ipaddr, uint8_t ipaddrlen, unsigned long interval){
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_prefix_list, UIP_DS6_PREFIX_NB, sizeof(uip_ds6_prefix), ipaddr, ipaddrlen, (uip_ds6_element**)&locprefix) == FREESPACE) {
    locprefix->isused = 1;
    uip_ipaddr_copy(&(locprefix->ipaddr), ipaddr);
    locprefix->length = ipaddrlen;
    if(interval != 0){
      stimer_set(&(locprefix->vlifetime),interval);
      locprefix->isinfinite = 0;
    } else {
      locprefix->isinfinite = 1;
    }
    PRINTF("Adding prefix ");
    PRINT6ADDR(&locprefix->ipaddr);
    PRINTF("length %u, vlifetime%lu\n", ipaddrlen, interval);
  }
  return NULL;
}
#endif /* UIP_CONF_ROUTER */

/*---------------------------------------------------------------------------*/

void
uip_ds6_prefix_rm( uip_ds6_prefix *prefix) {
  if(prefix != NULL) {
    prefix->isused = 0;
  }
  return;
}

/*---------------------------------------------------------------------------*/
uip_ds6_prefix*
uip_ds6_prefix_lookup(uip_ipaddr_t *ipaddr, uint8_t ipaddrlen) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_prefix_list, UIP_DS6_PREFIX_NB, sizeof(uip_ds6_prefix), ipaddr, ipaddrlen, (uip_ds6_element**)&locprefix) == FOUND) {
    return locprefix;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
uint8_t uip_ds6_is_addr_onlink(uip_ipaddr_t *ipaddr) {
  for(locprefix = uip_ds6_prefix_list; locprefix < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB; locprefix++) {
    if((locprefix->isused) &&
       (uip_ipaddr_prefixcmp(&locprefix->ipaddr, ipaddr, locprefix->length)) ){ 
      return 1;
    }   
  }
  return 0;
}

/*---------------------------------------------------------------------------*/
uip_ds6_addr* 
uip_ds6_addr_add(uip_ipaddr_t *ipaddr, unsigned long vlifetime, uint8_t type) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_if.addr_list, UIP_DS6_ADDR_NB, sizeof(uip_ds6_addr),ipaddr, 128, (uip_ds6_element**)&locaddr) == FREESPACE) {
    locaddr->isused = 1;
    uip_ipaddr_copy(&locaddr->ipaddr, ipaddr);
    locaddr->state = ADDR_TENTATIVE;
    locaddr->type = type;
    if(vlifetime == 0) {
      locaddr->isinfinite = 1;
    } else {
      locaddr->isinfinite = 0;
      stimer_set(&(locaddr->vlifetime), vlifetime);
    }
    timer_set(&locaddr->dadtimer, random_rand()%(UIP_ND6_MAX_RTR_SOLICITATION_DELAY * CLOCK_SECOND));
    locaddr->dadnscount = 0;
    uip_create_solicited_node(ipaddr, &loc_fipaddr);
    uip_ds6_maddr_add(&loc_fipaddr);
    return locaddr;
  }
  return NULL; 
}

/*---------------------------------------------------------------------------*/
void 
uip_ds6_addr_rm(uip_ds6_addr *addr) {
  if(addr != NULL) { 
    uip_create_solicited_node(&addr->ipaddr, &loc_fipaddr);
    if((locmaddr = uip_ds6_maddr_lookup(&loc_fipaddr)) != NULL) {
      uip_ds6_maddr_rm(locmaddr);
    }
    addr->isused = 0;
  }
  return;
}

/*---------------------------------------------------------------------------*/
uip_ds6_addr*
uip_ds6_addr_lookup(uip_ipaddr_t* ipaddr) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_if.addr_list, UIP_DS6_ADDR_NB, sizeof(uip_ds6_addr),ipaddr, 128, (uip_ds6_element**)&locaddr) == FOUND) {
    return locaddr;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
uip_ds6_maddr* 
uip_ds6_maddr_add(uip_ipaddr_t *ipaddr) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_if.maddr_list, UIP_DS6_MADDR_NB, sizeof(uip_ds6_maddr),ipaddr, 128, (uip_ds6_element**)&locmaddr) == FREESPACE) {
    locmaddr->isused = 1;
    uip_ipaddr_copy(&locmaddr->ipaddr, ipaddr);
    return locmaddr;
  }
  return NULL; 
}

/*---------------------------------------------------------------------------*/
void 
uip_ds6_maddr_rm(uip_ds6_maddr *maddr) {
  if(maddr != NULL) {
    maddr->isused = 0;
  }
  return;
}

/*---------------------------------------------------------------------------*/
uip_ds6_maddr*
uip_ds6_maddr_lookup(uip_ipaddr_t* ipaddr) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_if.maddr_list, UIP_DS6_MADDR_NB, sizeof(uip_ds6_maddr),ipaddr, 128, (uip_ds6_element**)&locmaddr) == FOUND) {
    return locmaddr;
  }
  return NULL;
}


/*---------------------------------------------------------------------------*/
uip_ds6_aaddr* 
uip_ds6_aaddr_add(uip_ipaddr_t *ipaddr) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_if.aaddr_list, UIP_DS6_AADDR_NB, sizeof(uip_ds6_aaddr),ipaddr, 128, (uip_ds6_element**)&locaaddr) == FREESPACE) {
    locaaddr->isused = 1;
    uip_ipaddr_copy(&locaaddr->ipaddr, ipaddr);
    return locaaddr;
  }
  return NULL; 
}

/*---------------------------------------------------------------------------*/
void 
uip_ds6_aaddr_rm(uip_ds6_aaddr *aaddr) {
  if(aaddr != NULL) {
    aaddr->isused = 0;
  }
  return;
}

/*---------------------------------------------------------------------------*/
uip_ds6_aaddr*
uip_ds6_aaddr_lookup(uip_ipaddr_t* ipaddr) {
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_if.aaddr_list, UIP_DS6_AADDR_NB, sizeof(uip_ds6_aaddr),ipaddr, 128, (uip_ds6_element**)&locaaddr) == FOUND) {
    return locaaddr;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
uip_ipaddr_t*
uip_ds6_route_lookup(uip_ipaddr_t *destipaddr) {
  locipaddr = NULL;
  uint8_t longestmatch = 0;

  for(locroute = uip_ds6_routing_table; locroute < uip_ds6_routing_table + UIP_DS6_ROUTE_NB; locroute++) {
    if((locroute->isused) && 
       (locroute->length >= longestmatch) &&
       (uip_ipaddr_prefixcmp(destipaddr, &locroute->ipaddr, locroute->length))) {
      longestmatch = locroute->length;
      locipaddr = &locroute->ipaddr;
    }
  }
  
  return locipaddr;
}

/*---------------------------------------------------------------------------*/
uip_ds6_route*
uip_ds6_route_add(uip_ipaddr_t *ipaddr, u8_t length,  uip_ipaddr_t *nexthop, u8_t metric) {
 
  if(uip_ds6_list_loop((uip_ds6_element*)uip_ds6_routing_table, UIP_DS6_ROUTE_NB, sizeof(uip_ds6_route), ipaddr, length, (uip_ds6_element**)&locroute) == FREESPACE) {
    locroute->isused = 1;
    uip_ipaddr_copy(&(locroute->ipaddr), ipaddr);
    locroute->length = length;
    uip_ipaddr_copy(&(locroute->nexthop), nexthop);
    locroute->metric = metric;
  }
  
  return locroute;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_route_rm(uip_ds6_route* route) {
  route->isused = 0;
  return;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_select_src(uip_ipaddr_t *src, uip_ipaddr_t *dst)
{   
  uint8_t best = 0; /* number of bit in common with best match*/
  uint8_t n = 0;
  uip_ds6_addr *matchaddr = uip_ds6_if.addr_list;
  
  if(!uip_is_addr_link_local(dst) && !uip_is_addr_mcast(dst)) {
    // find longest match
    for(locaddr = uip_ds6_if.addr_list; locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; locaddr++) {
      if((locaddr->isused) && (locaddr->state == ADDR_PREFERRED)){
        n = get_match_length(dst, &(locaddr->ipaddr));
        if(n >= best){
          best = n;
          matchaddr = locaddr;
        }
      }
    }
  } else {
    // use link local
    for(locaddr = uip_ds6_if.addr_list; locaddr < uip_ds6_if.addr_list + UIP_DS6_ADDR_NB; locaddr++) {
      if((locaddr->isused) && (locaddr->state == ADDR_PREFERRED) && (uip_is_addr_link_local(&locaddr->ipaddr))) {
        matchaddr = locaddr;
        break;
      }
    }
  }

  uip_ipaddr_copy(src, &matchaddr->ipaddr);
  return;
}

/*---------------------------------------------------------------------------*/
void uip_ds6_set_addr_iid(uip_ipaddr_t *ipaddr, uip_lladdr_t *lladdr) {
  /* We consider only links with IEEE EUI-64 identifier or
   * IEEE 48-bit MAC addresses */
#if (UIP_LLADDR_LEN == 8)
  memcpy(ipaddr->u8 + 8, lladdr, UIP_LLADDR_LEN);
  ipaddr->u8[8] ^= 0x02;
#elif (UIP_LLADDR_LEN == 6)
  memcpy(ipaddr->u8 + 8, lladdr, 3);
  ipaddr->u8[11] = 0xff;
  ipaddr->u8[12] = 0xfe;
  memcpy(ipaddr->u8 + 13, (uint8_t*)lladdr + 3, 3);
  ipaddr->u8[8] ^= 0x02;
#else
  PRINTF("CAN NOT BUIL INTERFACE IDENTIFIER");
  PRINTF("THE STACK IS GOING TO SHUT DOWN");
  PRINTF("THE HOST WILL BE UNREACHABLE");
  exit(-1);
#endif
  return;
}

/*---------------------------------------------------------------------------*/
uint8_t
get_match_length(uip_ipaddr_t *src, uip_ipaddr_t *dst) {
  uint8_t j, k, x_or;
  uint8_t len = 0;
  for(j = 0; j < 16; j ++) {
    if(src->u8[j] == dst->u8[j]) {
      len += 8;
    } else {
      x_or = src->u8[j] ^ dst->u8[j];
      for(k = 0; k < 8; k ++) {
        if((x_or & 0x80) == 0){
          len ++;
          x_or <<= 1;
        }
        else {
          break;
        }
      } 
      break;
    }
  }
  return len;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_dad(uip_ds6_addr* addr) {
  /* send maxdadns NS for DAD  */
  if(addr->dadnscount < uip_ds6_if.maxdadns) {
    uip_nd6_ns_output(NULL, NULL, &addr->ipaddr);
    addr->dadnscount++;
    timer_set(&addr->dadtimer, uip_ds6_if.retrans_timer / 1000 * CLOCK_SECOND);
    return;
  }
  /*
   * If we arrive here it means DAD succeeded, otherwise the dad process
   * would have been interrupted in ds6_dad_ns/na_input
   */
  PRINTF("DAD succeeded, ipaddr:");
  PRINT6ADDR(&addr->ipaddr);
  PRINTF("\n");

  addr->state = ADDR_PREFERRED;
  return;
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_dad_failed(uip_ds6_addr *addr) {
  if(uip_is_addr_link_local(&addr->ipaddr)) {
    PRINTF("Contiki shutdown, DAD for link local address failed\n");
    exit(-1);
  }
  uip_ds6_addr_rm(addr);
  return;
}

#if UIP_CONF_ROUTER 
#if UIP_ND6_SEND_RA
/*---------------------------------------------------------------------------*/
void
uip_ds6_send_ra_sollicited(void) {
  /* We have a pb here: RA timer max possible value is 1800s,
   * hence we have to use stimers. However, when receiving a RS, we
   * should delay the reply by a random value between 0 and 500ms timers.  
   * stimers are in seconds, hence we cannot do this. Therefore we just send 
   * the RA (setting the timer to 0 below). We keep the code logic for 
   * the days contiki will support appropriate timers */
  rand_time = 0;
  PRINTF("Solicited RA, random time %u\n", rand_time);  

  if (stimer_remaining(&uip_ds6_timer_ra) > rand_time) {
    if (stimer_elapsed(&uip_ds6_timer_ra) < UIP_ND6_MIN_DELAY_BETWEEN_RAS) {
      /* Ensure that the RAs are rate limited */
/*      stimer_set(&uip_ds6_timer_ra, rand_time +
                 UIP_ND6_MIN_DELAY_BETWEEN_RAS -
                 stimer_elapsed(&uip_ds6_timer_ra));     
  */  } else {
      stimer_set(&uip_ds6_timer_ra, rand_time);     
    }
  }
}

/*---------------------------------------------------------------------------*/
void
uip_ds6_send_ra_periodic(void) {
  if(racount > 0){
    /* send previously scheduled RA*/
    uip_nd6_ra_output(NULL);
    PRINTF("Sending periodic RA\n");
  }

  rand_time = UIP_ND6_MIN_RA_INTERVAL + random_rand() %
    (uint16_t)(UIP_ND6_MAX_RA_INTERVAL - UIP_ND6_MIN_RA_INTERVAL);
  PRINTF("Random time 1 = %u\n", rand_time);

  if(racount < UIP_ND6_MAX_INITIAL_RAS){
    if (rand_time > UIP_ND6_MAX_INITIAL_RA_INTERVAL){
      rand_time = UIP_ND6_MAX_INITIAL_RA_INTERVAL;
      PRINTF("Random time 2 = %u\n", rand_time);
    }
    racount++;
  }
  PRINTF("Random time 3 = %u\n", rand_time);
  stimer_set(&uip_ds6_timer_ra, rand_time);     
}

#endif /* UIP_ND6_SEND_RA */
#else /* UIP_CONF_ROUTER */
/*---------------------------------------------------------------------------*/
void
uip_ds6_send_rs(void) {
  if((uip_ds6_defrt_choose() == NULL) && (rscount < UIP_ND6_MAX_RTR_SOLICITATIONS)){
    PRINTF("Sending RS %u\n", rscount);
    uip_nd6_rs_output();
    rscount++;
    etimer_set(&uip_ds6_timer_rs, UIP_ND6_RTR_SOLICITATION_INTERVAL * CLOCK_SECOND);     
  } else {
    PRINTF("Router found ? (boolean): %u\n", (uip_ds6_defrt_choose() != NULL));
    etimer_stop(&uip_ds6_timer_rs);
  }
  return;
}

#endif /* UIP_CONF_ROUTER */
/*---------------------------------------------------------------------------*/
uint32_t
uip_ds6_compute_reachable_time(void)
{
  return (uint32_t)(UIP_ND6_MIN_RANDOM_FACTOR(uip_ds6_if.base_reachable_time)) +
    ((uint16_t)(random_rand() << 8) + (uint16_t)random_rand()) %
    (uint32_t)(UIP_ND6_MAX_RANDOM_FACTOR(uip_ds6_if.base_reachable_time) -
	       UIP_ND6_MIN_RANDOM_FACTOR(uip_ds6_if.base_reachable_time));
}


/** @} */
