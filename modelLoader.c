#include <stdio.h>
#include <iconv.h>
#include <string.h>
#include <json-c/json.h>

#define MAX_BUF 1024

int byte_size = 0;
struct Header {
    //マジックナンバー
    char header[4]; //"PMX "
    float version; //PMXのバージョン

    //バイト列
    char byte_size[1]; //後続するデータのバイトサイズ（PMX2.0は8で固定）
    char encode[1]; //エンコード方式（0:UTF16 | 1:UTF8）
    char additional_UV_size[1]; //追加UV数（0〜4）

    char top_index_size[1]; //頂点indexサイズ（1,2,4のどれか）
    char texture_index_size[1]; //テクスチャindexサイズ（1,2,4のどれか）
    char material_index_size[1]; //素材indexサイズ（以下略
    char bone_index_size[1]; //ボーン（以下略
    char morph_index_size[1]; //モーフ
    char rigidBody_index_size[1]; //剛体
};

struct Model {
    char mode_name[46]; //モデル名
    char comment[7][46]; //コメント

};

char* decode(char* string, int length){ //エンコードされている構造体ファイルのcharをshiftjisでデコード
    char inbuf[MAX_BUF + 1] = {0};
    char outbuf[MAX_BUF + 1] = {0};
    char *in = inbuf;
    char *out = outbuf;
    size_t in_size = (size_t) MAX_BUF;
    size_t out_size = (size_t) MAX_BUF;

    iconv_t cd = iconv_open("UTF-8", "UTF-16");

    memcpy(in, string, length);
    iconv(cd, &in, &in_size, &out, &out_size);
    iconv_close(cd);

    return strdup(outbuf);
}


char* path = "/home/shuta/ダウンロード/YYB Hatsune Miku_10th/YYB Hatsune Miku_10th_v1.02_toonchange.pmx";
int main(){
    FILE *fpw = fopen(path, "rb");
    //ヘッダーの宣言
    struct Header header;
    fread(&header, sizeof(header), 1, fpw);

    printf("%d\n", header.additional_UV_size[0]);

    struct Model model;
    fseek(fpw, 1, SEEK_CUR);
    fread(&model, sizeof(model), 1, fpw);

    printf("%s", decode(model.comment[1], 46));
}