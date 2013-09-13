/*
 * Structure of a Macintosh driver descriptor (block 0)
 * and partition table (blocks 1..n).
 *
 * Copyright 1996 Paul Mackerras.
 */

#define MAC_PARTITION_MAGIC	0x504d

/* type field value for A/UX or other Unix partitions */
#define APPLE_AUX_TYPE	"Apple_UNIX_SVR2"

/* type field for bootstrap partition */
#define APPLE_BOOT_TYPE "Apple_Bootstrap"

struct mac_partition {
    uint16_t	signature;	/* expected to be MAC_PARTITION_MAGIC */
    uint16_t	res1;
    uint32_t	map_count;	/* # blocks in partition map */
    uint32_t	start_block;	/* absolute starting block # of partition */
    uint32_t	block_count;	/* number of blocks in partition */
    char	name[32];	/* partition name */
    char	type[32];	/* string type description */
    uint32_t	data_start;	/* rel block # of first data block */
    uint32_t	data_count;	/* number of data blocks */
    uint32_t	status;		/* partition status */
    uint32_t	boot_start;	/* logical start block no. of bootstrap */
    uint32_t	boot_size;	/* no. of bytes in bootstrap */
    uint32_t	boot_load;	/* bootstrap load address in memory */
    uint32_t	boot_load2;	/* reserved for extension of boot_load */
    uint32_t	boot_entry;	/* entry point address for bootstrap */
    uint32_t	boot_entry2;	/* reserved for extension of boot_entry */
    uint32_t	boot_cksum;
    char	processor[16];	/* name of processor that boot is for */
};

/* Bit in status field */
#define STATUS_BOOTABLE	8	/* partition is bootable */

#define MAC_DRIVER_MAGIC	0x4552

/* Driver descriptor structure, in block 0 */
struct mac_driver_desc {
    uint16_t	signature;	/* expected to be MAC_DRIVER_MAGIC */
    uint16_t	block_size;
    uint32_t	block_count;
    /* ... more stuff */
};
