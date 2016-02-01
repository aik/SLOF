/******************************************************************************
 * Copyright (c) 2011 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <cpu.h>
#include <cache.h>
#include <byteorder.h>
#include "virtio.h"
#include "helpers.h"

/* PCI virtio header offsets */
#define VIRTIOHDR_DEVICE_FEATURES	0
#define VIRTIOHDR_GUEST_FEATURES	4
#define VIRTIOHDR_QUEUE_ADDRESS 	8
#define VIRTIOHDR_QUEUE_SIZE		12
#define VIRTIOHDR_QUEUE_SELECT		14
#define VIRTIOHDR_QUEUE_NOTIFY		16
#define VIRTIOHDR_DEVICE_STATUS 	18
#define VIRTIOHDR_ISR_STATUS		19
#define VIRTIOHDR_DEVICE_CONFIG 	20

struct virtio_dev_common {
	le32 dev_features_sel;
	le32 dev_features;
	le32 drv_features_sel;
	le32 drv_features;
	le16 msix_config;
	le16 num_queues;
	uint8_t dev_status;
	uint8_t cfg_generation;

	le16 q_select;
	le16 q_size;
	le16 q_msix_vec;
	le16 q_enable;
	le16 q_notify_off;
	le64 q_desc;
	le64 q_avail;
	le64 q_used;
} __attribute__ ((packed));

/**
 * Calculate ring size according to queue size number
 */
unsigned long virtio_vring_size(unsigned int qsize)
{
	return VQ_ALIGN(sizeof(struct vring_desc) * qsize +
			sizeof(struct vring_avail) + sizeof(uint16_t) * qsize) +
		VQ_ALIGN(sizeof(struct vring_used) +
			 sizeof(struct vring_used_elem) * qsize);
}


/**
 * Get number of elements in a vring
 * @param   dev  pointer to virtio device information
 * @param   queue virtio queue number
 * @return  number of elements
 */
int virtio_get_qsize(struct virtio_device *dev, int queue)
{
	int size = 0;

	ci_write_16(dev->base+VIRTIOHDR_QUEUE_SELECT,
		    cpu_to_le16(queue));
	eieio();
	size = le16_to_cpu(ci_read_16(dev->base+VIRTIOHDR_QUEUE_SIZE));

	return size;
}


/**
 * Get address of descriptor vring
 * @param   dev  pointer to virtio device information
 * @param   queue virtio queue number
 * @return  pointer to the descriptor ring
 */
struct vring_desc *virtio_get_vring_desc(struct virtio_device *dev, int queue)
{
	struct vring_desc *desc = 0;

	ci_write_16(dev->base+VIRTIOHDR_QUEUE_SELECT,
		    cpu_to_le16(queue));
	eieio();
	desc = (void*)(4096L *
		       le32_to_cpu(ci_read_32(dev->base+VIRTIOHDR_QUEUE_ADDRESS)));

	return desc;
}


/**
 * Get address of "available" vring
 * @param   dev  pointer to virtio device information
 * @param   queue virtio queue number
 * @return  pointer to the "available" ring
 */
struct vring_avail *virtio_get_vring_avail(struct virtio_device *dev, int queue)
{
	return (void*)((uint64_t)virtio_get_vring_desc(dev, queue)
		       + virtio_get_qsize(dev, queue) * sizeof(struct vring_desc));
}


/**
 * Get address of "used" vring
 * @param   dev  pointer to virtio device information
 * @param   queue virtio queue number
 * @return  pointer to the "used" ring
 */
struct vring_used *virtio_get_vring_used(struct virtio_device *dev, int queue)
{
	return (void*)VQ_ALIGN((uint64_t)virtio_get_vring_avail(dev, queue)
			       + virtio_get_qsize(dev, queue)
			       * sizeof(struct vring_avail));
}

/**
 * Fill the virtio ring descriptor depending on the legacy mode or virtio 1.0
 */
void virtio_fill_desc(struct vring_desc *desc, bool is_modern,
                      uint64_t addr, uint32_t len,
                      uint16_t flags, uint16_t next)
{
	if (is_modern) {
		desc->addr = cpu_to_le64(addr);
		desc->len = cpu_to_le32(len);
		desc->flags = cpu_to_le16(flags);
		desc->next = cpu_to_le16(next);
	} else {
		desc->addr = addr;
		desc->len = len;
		desc->flags = flags;
		desc->next = next;
	}
}

/**
 * Reset virtio device
 */
void virtio_reset_device(struct virtio_device *dev)
{
	ci_write_8(dev->base+VIRTIOHDR_DEVICE_STATUS, 0);
}


/**
 * Notify hypervisor about queue update
 */
void virtio_queue_notify(struct virtio_device *dev, int queue)
{
	ci_write_16(dev->base+VIRTIOHDR_QUEUE_NOTIFY, cpu_to_le16(queue));
}

/**
 * Set queue address
 */
void virtio_set_qaddr(struct virtio_device *dev, int queue, unsigned long qaddr)
{
	uint32_t val = qaddr;
	val = val >> 12;
	ci_write_16(dev->base+VIRTIOHDR_QUEUE_SELECT,
		    cpu_to_le16(queue));
	eieio();
	ci_write_32(dev->base+VIRTIOHDR_QUEUE_ADDRESS,
		    cpu_to_le32(val));
}

int virtio_queue_init_vq(struct virtio_device *dev, struct vqs *vq, unsigned int id)
{
	vq->size = virtio_get_qsize(dev, id);
	vq->desc = SLOF_alloc_mem_aligned(virtio_vring_size(vq->size), 4096);
	if (!vq->desc) {
		printf("memory allocation failed!\n");
		return -1;
	}
	memset(vq->desc, 0, virtio_vring_size(vq->size));
	virtio_set_qaddr(dev, id, (unsigned long)vq->desc);
	vq->avail = virtio_get_vring_avail(dev, id);
	vq->used = virtio_get_vring_used(dev, id);
	vq->id = id;
	return 0;
}

/**
 * Set device status bits
 */
void virtio_set_status(struct virtio_device *dev, int status)
{
	ci_write_8(dev->base+VIRTIOHDR_DEVICE_STATUS, status);
}


/**
 * Set guest feature bits
 */
void virtio_set_guest_features(struct virtio_device *dev, int features)

{
	ci_write_32(dev->base+VIRTIOHDR_GUEST_FEATURES, bswap_32(features));
}

/**
 * Get host feature bits
 */
void virtio_get_host_features(struct virtio_device *dev, int *features)

{
	*features = bswap_32(ci_read_32(dev->base+VIRTIOHDR_DEVICE_FEATURES));
}


/**
 * Get additional config values
 */
uint64_t virtio_get_config(struct virtio_device *dev, int offset, int size)
{
	uint64_t val = ~0ULL;
	void *confbase;

	confbase = dev->base + VIRTIOHDR_DEVICE_CONFIG;
	switch (size) {
	case 1:
		val = ci_read_8(confbase+offset);
		break;
	case 2:
		val = ci_read_16(confbase+offset);
		break;
	case 4:
		val = ci_read_32(confbase+offset);
		break;
	case 8:
		/* We don't support 8 bytes PIO accesses
		 * in qemu and this is all PIO
		 */
		val = ci_read_32(confbase+offset);
		val <<= 32;
		val |= ci_read_32(confbase+offset+4);
		break;
	}

	return val;
}

/**
 * Get config blob
 */
int __virtio_read_config(struct virtio_device *dev, void *dst,
			 int offset, int len)
{
	void *confbase;
	unsigned char *buf = dst;
	int i;

	confbase = dev->base + VIRTIOHDR_DEVICE_CONFIG;
	for (i = 0; i < len; i++)
		buf[i] = ci_read_8(confbase + offset + i);

	return len;
}
