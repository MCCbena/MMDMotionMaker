//
// Created by shuta on 24/11/05.
//

#ifndef TEST_INDEXLIB_H
#define TEST_INDEXLIB_H
#include <string.h>
#include <iconv.h>

#define MAX_BUF 1024

#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

char* word_decode(char* string, int length, char* toCode, char* fromCode){ //エンコードされている構造体ファイルのcharをshiftjisでデコード
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



struct Index{
    char name[1024][1024];
    int assigned;
};

struct Index makeIndex(){
    struct Index index;
    index.assigned = 0;
    return index;
}

int getnIndex(struct Index index, char *from, int n){
    for(int i = 0; i < index.assigned; i++){
        if(strncmp(index.name[i], from, n) == 0){
            return i;
        }
    }
    return -1;
}
int getIndex(struct Index index, char* from){
    for(int i = 0; i < index.assigned; i++){
        if(strcmp(index.name[i], from) == 0){
            return i;
        }
    }
    return -1;
}

void addIndex(struct Index *index, char* name, int n){
    memcpy(index->name[index->assigned], name, n);
    index->assigned++;
}
#endif //TEST_INDEXLIB_H
