#ifndef CCN_LITE_RIOT_H
#define CCN_LITE_RIOT_H

#define CCNL_RIOT

#undef USE_NFN

#define USE_IPV6
#define USE_SUITE_NDNTLV
#define NEEDS_PREFIX_MATCHING

#include "ccnl-defs.h"
#include "ccnl-core.h"

void ccnl_event_loop(struct ccnl_relay_s *ccnl);
void ccnl_minimalrelay_ageing(void *relay, void *aux);

#endif /* CCN_LITE_RIOT_H */
