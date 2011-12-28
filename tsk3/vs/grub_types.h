//defines grub-types 

#ifndef _GRUB_TYPES_H_
#define _GRUB_TYPES_H_

#include "tsk_vs_i.h"
#include "tsk_incs.h"

#define grub_disk_t     void *
#define grub_disk_addr_t    uint64_t 

#define grub_uint64_t   uint64_t
#define grub_uint32_t   uint32_t
#define grub_int8_t     int8_t
#define grub_size_t     size_t

 
#define grub_strstr     strstr
#define grub_strlen     strlen
#define grub_strcmp     strcmp
#define grub_strncmp     strncmp
#define grub_strchr     strchr
#define grub_strtoul    TSTRTOUL
#define grub_memcmp     memcmp
#define grub_memcpy     memcpy
#define grub_isspace    isspace
 
#define grub_malloc     malloc
#define grub_zalloc(n)  calloc(1, n)
#define grub_free       free

#define grub_le_to_cpu64(x)    tsk_getu64(1, &x)
#define grub_le_to_cpu32(x)    tsk_getu32(1, &x)


#define grub_util_error     printf
#define grub_error          printf

typedef enum
  {       
    GRUB_ERR_NONE = 0,
    GRUB_ERR_TEST_FAILURE,
    GRUB_ERR_BAD_MODULE,
    GRUB_ERR_OUT_OF_MEMORY,
    GRUB_ERR_BAD_FILE_TYPE,
    GRUB_ERR_FILE_NOT_FOUND,
    GRUB_ERR_FILE_READ_ERROR,
    GRUB_ERR_BAD_FILENAME,
    GRUB_ERR_UNKNOWN_FS,
    GRUB_ERR_BAD_FS,
    GRUB_ERR_BAD_NUMBER,
    GRUB_ERR_OUT_OF_RANGE,
    GRUB_ERR_UNKNOWN_DEVICE,
    GRUB_ERR_BAD_DEVICE,
    GRUB_ERR_READ_ERROR,
    GRUB_ERR_WRITE_ERROR,
    GRUB_ERR_UNKNOWN_COMMAND,
    GRUB_ERR_INVALID_COMMAND,
    GRUB_ERR_BAD_ARGUMENT,
    GRUB_ERR_BAD_PART_TABLE,
    GRUB_ERR_UNKNOWN_OS,
    GRUB_ERR_BAD_OS, 
    GRUB_ERR_NO_KERNEL,
    GRUB_ERR_BAD_FONT,
    GRUB_ERR_NOT_IMPLEMENTED_YET,
    GRUB_ERR_SYMLINK_LOOP,
    GRUB_ERR_BAD_COMPRESSED_DATA,
    GRUB_ERR_MENU,
    GRUB_ERR_TIMEOUT,
    GRUB_ERR_IO,
    GRUB_ERR_ACCESS_DENIED,
    GRUB_ERR_EXTRACTOR,
    GRUB_ERR_BUG
  }
grub_err_t;

#endif
