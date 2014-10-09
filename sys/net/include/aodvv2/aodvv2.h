#include "common/netaddr.h"
#include "rfc5444/rfc5444_print.h"

#include "aodvv2/types.h"

#ifndef AODVV2_H_
#define AODVV2_H_

void aodv_init(void);

/**
 * @brief   Set the metric type. If metric_type does not match any known metric
 *          types, no changes will be made.
 *
 * @param[in] metric_type       type of new metric
 */
void aodv_set_metric_type(aodvv2_metric_t metric_type);

#endif /* AODVV2_H_ */
