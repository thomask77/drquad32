/*****************************************************************************
 *
 * cobsr.h
 *
 * Consistent Overhead Byte Stuffing--Reduced (COBS/R)
 *
 ****************************************************************************/

#ifndef COBSR_H_
#define COBSR_H_


/*****************************************************************************
 * Includes
 ****************************************************************************/

#include <stddef.h>
#include <sys/types.h>

/*****************************************************************************
 * Defines
 ****************************************************************************/

#define COBSR_ENCODE_DST_BUF_LEN_MAX(SRC_LEN)   ((SRC_LEN) + ((SRC_LEN)/254u) + 1)
#define COBSR_DECODE_DST_BUF_LEN_MAX(SRC_LEN)   (SRC_LEN)


/*****************************************************************************
 * Function prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

ssize_t cobsr_encode(void *dst_buf_ptr, size_t dst_buf_len, const void *src_ptr, size_t src_len);
ssize_t cobsr_decode(void *dst_buf_ptr, size_t dst_buf_len, const void *src_ptr, size_t src_len);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* COBSR_H_ */
