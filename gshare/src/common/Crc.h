/* Crc.h - crc library
 */

#ifndef __CRC_LIB_H__
#define __CRC_LIB_H__

#include "stdconfig.h"
#include <stdint.h>

namespace Crc {
    SHARED_DECL uint32_t Crc32(const char *src, int len);
    SHARED_DECL uint16_t Crc16(const char *str, int len);
    SHARED_DECL uint32_t Crc24(const char *str, int len);
}

#endif //__CRC_LIB_H__