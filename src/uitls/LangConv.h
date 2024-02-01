//
// Created by shuta on 24/02/01.
//

#ifndef TEST_LANGCONV_H
#define TEST_LANGCONV_H

#endif //TEST_LANGCONV_H

#include <iconv.h>
#include <string.h>

#define MAX_BUF 1024

char* decode(char* string, int length, char* toCode, char* fromCode){ //エンコードされている構造体ファイルのcharをshiftjisでデコード
    char inbuf[MAX_BUF + 1] = {0};
    char outbuf[MAX_BUF + 1] = {0};
    char *in = inbuf;
    char *out = outbuf;
    size_t in_size = (size_t) MAX_BUF;
    size_t out_size = (size_t) MAX_BUF;

    iconv_t cd = iconv_open(toCode, fromCode);

    memcpy(in, string, length);
    iconv(cd, &in, &in_size, &out, &out_size);
    iconv_close(cd);

    return strdup(outbuf);
}