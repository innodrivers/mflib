/*
 * memp_std.h
 *
 *  Created on: Apr 25, 2013
 *      Author: drivers
 */


/*
 * MF_MEMPOOL(pool_name, number_elements, element_size, pool_description)
 */
MF_MEMPOOL(NETBUF, 128, MF_MEMP_NETBUF_SIZE, "struct mf_netbuf")

#undef MF_MEMPOOL
