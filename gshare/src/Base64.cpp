/** \file Base64.cpp
 **	\date  2004-02-13
 **	\author grymse@alhem.net
**/
/*
Copyright (C) 2004-2011  Anders Hedstrom

This library is made available under the terms of the GNU GPL, with
the additional exemption that compiling, linking, and/or using OpenSSL 
is allowed.

If you would like to use this library in a closed-source application,
a separate license agreement is available. For information about 
the closed-source license agreement for the C++ sockets library,
please visit http://www.alhem.net/Sockets/license.html and/or
email license@alhem.net.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "Base64.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

const char *bstr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const uint8_t rstr[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 62,   0xff, 0xff, 0xff, 63,
    52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   0xff, 0xff, 0xff, 64,   0xff, 0xff,
    0xff, 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,
    15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
    41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,   0xff, 0xff, 0xff, 0xff, 0xff
};

std::string Base64::encode(const unsigned char *cnt, size_t l)
{
    size_t len = encode_length(l);
    std::string strRet(len+1, 0);

    char *base64 = &strRet.at(0);
    for (size_t i = 0, pos=0; i < l; i += 3)
	{
        size_t remain = l - i;
        base64[pos++] = bstr[((cnt[i] >> 2) & 0x3f)];
        switch (remain)
        {
        case 1:
            base64[pos++] = bstr[((cnt[i] << 4) & 0x30)];
            base64[pos++] = '=';
            base64[pos++] = '=';
            break;
        case 2:
            base64[pos++] = bstr[((cnt[i] << 4) & 0x30) + ((cnt[i + 1] >> 4) & 0x0f)];
            base64[pos++] = bstr[((cnt[i + 1] << 2) & 0x3c)];
            base64[pos++] = '=';
            break;
        default:
            base64[pos++] = bstr[((cnt[i] << 4) & 0x30) + ((cnt[i + 1] >> 4) & 0x0f)];
            base64[pos++] = bstr[((cnt[i + 1] << 2) & 0x3c) + ((cnt[i + 2] >> 6) & 0x03)];
            base64[pos++] = bstr[(cnt[i + 2] & 0x3f)];
        }
	}
    return strRet.c_str();
}

size_t Base64::decode(const std::string &base64, unsigned char *output, size_t sz)
{
    size_t ret = decode_length(base64);
    size_t l = base64.length();
    if (!output || ret<1 || sz < ret)
        return 0;

    uint8_t c[4];
    const uint8_t *input = (const uint8_t *)base64.c_str();
    for (size_t i = 0, pos=0; i < l; i += 4)
    {
        for (int j = 0; j < 4; ++j)
        {
            c[j] = input[i + j] < 128 ? rstr[input[i + j]] : 255;
            if (c[j] > 128)
                return 0;
        }

        output[pos++] = (uint8_t)(c[0] << 2 & 0xfc) + (c[1] >> 4 & 0x03);
        if (c[2] != 64)
            output[pos++] = (uint8_t)((c[1] << 4 & 0xf0) + (c[2] >> 2 & 0x0f));

        if (c[3] != 64)
            output[pos++] = (uint8_t)((c[2] << 6 & 0xf0) + c[3]);
    }
    return ret;
}

size_t Base64::decode_length(const std::string& str64)
{
    int len = str64.length();
    if (!len || len % 4)
        return 0;

    const uint8_t *base64 = (const uint8_t *)str64.c_str();
    size_t ret = (len * 3) / 4;
    if ('=' == base64[len - 1])
    {
        --ret;
        if ('=' == base64[len - 2])
            --ret;
    }
	return ret;
}

size_t Base64::encode_length(size_t sz)
{
    return ((sz + 2) / 3) * 4;
}

#ifdef SOCKETS_NAMESPACE
}
#endif


