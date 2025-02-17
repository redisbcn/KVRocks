/*
 * Definitions for the NVM Express ioctl interface
 * Copyright (c) 2011-2014, Intel Corporation.
 *
 * Modified by Heekwon Park from Samsung Electronics.
 * See KV_NVME_SUPPORT to check modification.
 * E-mail : heekwon.p@samsung.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef _UAPI_LINUX_NVME_IOCTL_H
#define _UAPI_LINUX_NVME_IOCTL_H

#ifndef KV_NVME_SUPPORT
#define KV_NVME_SUPPORT
#endif

#include <linux/types.h>

struct nvme_user_io {
    __u8	opcode;
    __u8	flags;
    __u16	control;
    __u16	nblocks;
    __u16	rsvd;
    __u64	metadata;
    __u64	addr;
    __u64	slba;
    __u32	dsmgmt;
    __u32	reftag;
    __u16	apptag;
    __u16	appmask;
};

struct nvme_passthru_cmd {
    __u8	opcode;
    __u8	flags;
    __u16	rsvd1;
    __u32	nsid;
    __u32	cdw2;
    __u32	cdw3;
    __u64	metadata;
    __u64	addr;
    __u32	metadata_len;
    __u32	data_len;
    __u32	cdw10;
    __u32	cdw11;
    __u32	cdw12;
    __u32	cdw13;
    __u32	cdw14;
    __u32	cdw15;
    __u32	timeout_ms;
    __u32	result;
};

#ifdef KV_NVME_SUPPORT
#define	KVS_SUCCESS		0
#define KVS_ERR_ALIGNMENT	(-1)
#define KVS_ERR_CAPAPCITY	(-2)
#define KVS_ERR_CLOSE	(-3)
#define KVS_ERR_CONT_EXIST	(-4)
#define KVS_ERR_CONT_NAME	(-5)
#define KVS_ERR_CONT_NOT_EXIST	(-6)
#define KVS_ERR_DEVICE_NOT_EXIST (-7)
#define KVS_ERR_GROUP	(-8)
#define KVS_ERR_INDEX	(-9)
#define KVS_ERR_IO	(-10)
#define KVS_ERR_KEY	(-11)
#define KVS_ERR_KEY_TYPE	(-12)
#define KVS_ERR_NOMEM	(-13)
#define KVS_ERR_NULL_INPUT	(-14)
#define KVS_ERR_OFFSET	(-15)
#define KVS_ERR_OPEN	(-16)
#define KVS_ERR_OPTION_NOT_SUPPORTED	(-17)
#define KVS_ERR_PERMISSION	(-18)
#define KVS_ERR_SPACE	(-19)
#define KVS_ERR_TIMEOUT	(-20)
#define KVS_ERR_TUPLE_EXIST	(-21)
#define KVS_ERR_TUPLE_NOT_EXIST	(-22)
#define KVS_ERR_VALUE	(-23)
#define KVS_ERR_INVALID_OPCODE (-24)
#define KVS_ERR_INVALID_FLAGS (-25)
#define KVS_ERR_EXIST_PENDING_KEY (-26)
#define KVS_ERR_EXIST_IN_RA_HASH (-27)
#define KVS_ERR_FLUSH_FAIL (-28)
#define KVS_ERR_FAULT (-29)
#define KVS_ERR_KEY_NOT_EXIST (-30)


struct nvme_passthru_kv_cmd {
    __u8	opcode;
    __u8	flags;
    __u16	rsvd1;
    __u32	nsid;
    __u32	cdw2;
    __u32	cdw3;
    __u32	cdw4;
    __u32	cdw5;
    __u64	data_addr;
    __u32	data_length;
    __u32	key_length;
    __u32 cdw10;
    __u32 cdw11;
    union {
        struct {
            __u64 key_addr;
            __u32 rsvd5;
            __u32 rsvd6;
        };
        struct {
            __u64 key_low;
            __u64 key_high;
        };
        __u8 key[16];
        struct {
            __u32 cdw12;
            __u32 cdw13;
            __u32 cdw14;
            __u32 cdw15;
        };
    };
    __u32	timeout_ms;
    __u32	result;
    __u32	status;
    __u32	ctxid;
    __u32	reqid;
};

#endif
#define nvme_admin_cmd nvme_passthru_cmd

#define NVME_IOCTL_ID		_IO('N', 0x40)
#define NVME_IOCTL_ADMIN_CMD	_IOWR('N', 0x41, struct nvme_admin_cmd)
#define NVME_IOCTL_SUBMIT_IO	_IOW('N', 0x42, struct nvme_user_io)
#define NVME_IOCTL_IO_CMD	_IOWR('N', 0x43, struct nvme_passthru_cmd)
#define NVME_IOCTL_RESET	_IO('N', 0x44)
#define NVME_IOCTL_SUBSYS_RESET	_IO('N', 0x45)
#define NVME_IOCTL_RESCAN	_IO('N', 0x46)

#ifdef KV_NVME_SUPPORT
#define NVME_IOCTL_KV_NONBLOCKING_READ_CMD	_IOWR('N', 0x47, struct nvme_passthru_kv_cmd)
#define NVME_IOCTL_KV_SYNC_CMD	_IOWR('N', 0x4A, struct nvme_passthru_kv_cmd)
#define NVME_IOCTL_KV_MULTISYNC_CMD	_IOWR('N', 0x4B, struct nvme_passthru_kv_cmd)
#define NVME_IOCTL_KV_ASYNC_CMD	_IOWR('N', 0x4C, struct nvme_passthru_kv_cmd)
#define NVME_IOCTL_KV_MULTIASYNC_CMD	_IOWR('N', 0x4D, struct nvme_passthru_kv_cmd)
#define NVME_IOCTL_KV_FLUSH_CMD _IOWR('N', 0x4E, struct nvme_passthru_kv_cmd)
#define NVME_IOCTL_KV_DISCARD_RA_CMD _IOWR('N', 0x4F, struct nvme_passthru_kv_cmd)

#endif
#endif /* _UAPI_LINUX_NVME_IOCTL_H */
