#include <string.h>
#include <iconv.h>

#define MAX_BUF 1024

#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

/*
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

*/


struct Index{
    char name[512][15];
    int assigned;
};

struct Index makeIndex(){
    struct Index index;
    index.assigned = 0;
    return index;
}

int getIndex(struct Index index, char *from){
    for(int i = 0; i < index.assigned; i++){
        if(strncmp(index.name[i], from, 15) == 0){
            return i;
        }
    }
    return -1;
}

void addIndex(struct Index *index, char name[15]){
    strlcpy(index->name[index->assigned], name, 15);
    index->assigned++;
}