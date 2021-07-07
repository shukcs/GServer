/* MD5lib.h - md5 library
 */

/* Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
rights reserved.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

/* The following makes MD default to MD5 if it has not already been
  defined with C compiler flags.
 */
#ifndef __MD5_LIB_H__
#define __MD5_LIB_H__

#include <string>
#include "stdconfig.h"

SHARED_DECL std::string MD5DigestString(const char *str, int len = -1);
SHARED_DECL std::string MD5String (const char *str, int len = -1);
SHARED_DECL std::string MD5File (const char *filename);

#endif //__MD5_LIB_H__