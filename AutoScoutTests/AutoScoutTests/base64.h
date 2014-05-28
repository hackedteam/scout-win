#ifndef _BASE64_H
#define _BASE64_H

char *base64_encode(const unsigned char *input, int length);

unsigned char *base64_decode(const char *input, int length, int *outlen);

inline bool is_base64(unsigned char c);

#endif