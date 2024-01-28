#include <stdio.h>
#include <iconv.h>
#include <string.h>
#include <json-c/json.h>

#define MAX_BUF 1024
#pragma pack(push, 1) // 構造体をきつくパッキングし、1バイトのアライメント

typedef struct {
    int byte_size; //バイト数
    char byte[1024]; //文字列（1024は仮）
} TextBuf;

int byte_size = 0;
struct Header {
    //マジックナンバー
    char header[4]; //"PMX "
    float version; //PMXのバージョン

    //バイト列
    char byte_size[1]; //後続するデータのバイトサイズ（PMX2.0は8で固定）
    char encode[1]; //エンコード方式（0:UTF16 | 1:UTF8）
    unsigned char additional_UV_size[1]; //追加UV数（0〜4）

    char top_index_size[1]; //頂点indexサイズ（1,2,4のどれか）
    char texture_index_size[1]; //テクスチャindexサイズ（1,2,4のどれか）
    char material_index_size[1]; //素材indexサイズ（以下略
    char bone_index_size[1]; //ボーン（以下略
    char morph_index_size[1]; //モーフ
    char rigidBody_index_size[1]; //剛体
};

struct Model {
    TextBuf mode_name; //モデル名
    TextBuf comment; //コメント

};

void getModelInfo(FILE *fpw, struct Model *model){
    //モデル名を取得
    fread(&model->mode_name.byte_size, sizeof(int), 1, fpw);
    fread(&model->mode_name.byte, model->mode_name.byte_size, 1, fpw);

    fseek(fpw, 8+17 + model->mode_name.byte_size, SEEK_SET);
    //モデルの説明を取得
    fread(&model->comment.byte_size, sizeof(int), 1, fpw);
    fread(&model->comment.byte, model->comment.byte_size, 1, fpw);
}
struct BDEF1{ //ウェイト変形方式:1
    char bone1[2]; //ウェイト1.0の単一ボーン(参照Index)
};
struct BDEF2{
    char bone1[2]; //ボーン1の参照Index
    char bone2[2]; //ボーン2の参照Index
    float weight1; //ボーン1のウェイト値(0～1.0), ボーン2のウェイト値は 1.0-ボーン1ウェイト
};
struct BDEF4{
    char bone1[2]; //ボーン1の参照Index
    char bone2[2]; //ボーン2の参照Index
    char bone3[2]; //ボーン3の参照Index
    char bone4[2]; //ボーン4の参照Index
    float weight1; //ボーン1のウェイト値
    float weight2; //ボーン2のウェイト値
    float weight3; //ボーン3のウェイト値
    float weight4; //ボーン4のウェイト値 (ウェイト計1.0の保障はない)
};
struct SDEF{
    char bone1[2]; //ボーン1の参照Index
    char bone2[2]; //ボーン2の参照Index
    float weight1; //ボーン1のウェイト値(0～1.0), ボーン2のウェイト値は 1.0-ボーン1ウェイト
    float SDEF_C[3]; //SDEF-C値(x,y,z)
    float SDEF_R0[3]; //DEF-R0値(x,y,z)
    float SDEF_R1[3]; //SDEF-R1値(x,y,z) ※修正値を要計算
};

struct TopData{
    float location[3]; //位置
    float normal[3]; //法線
    float uv[2]; //UV
    float *additional_uv[4]; //追加UV
    char deformation_method[1]; //ウェイト変形方式（０〜３）
    //ウェイト変形方式一覧
    struct BDEF1 bdef1;
    struct BDEF2 bdef2;
    struct BDEF4 bdef4;
    struct SDEF sdef;

    float edge_magnification; //エッジ倍率

};
void getTopData(FILE *fpw, struct TopData *topData, struct Header header){
    float test;
    fread(&test, 8, 1, fpw);

    //位置を設定
    fread(&topData->location, sizeof(float) * 3, 1, fpw);
    //法線を設定
    printf("カレント:%lx\n", ftell(fpw));

    fread(&topData->normal, sizeof(float) * 3, 1, fpw);
    //UVを設定
    fread(&topData->uv, sizeof(float) * 2, 1, fpw);
    //追加UVを設定
    int n = header.additional_UV_size[0];
    for (int i = 0; i<n;i++) {
        float additional_uv[4];
        fread(&additional_uv, sizeof(additional_uv), 1, fpw);
        topData->additional_uv[i] = additional_uv;
    }
    //ウェイト変形方式を設定
    fread(&topData->deformation_method, sizeof(char), 1, fpw);

    switch (topData->deformation_method[0]) {
        case 0:
            fread(&topData->bdef1.bone1, sizeof(char)*2, 1, fpw);
            fseek(fpw, 4, SEEK_CUR);
            break;
        case 1:
            fread(&topData->bdef2.bone1, sizeof(char)*2, 1, fpw);
            fread(&topData->bdef2.bone2, sizeof(char)*2, 1, fpw);

            fread(&topData->bdef2.weight1, sizeof(float)*2, 1, fpw);
            break;
        case 2:
            fread(&topData->bdef4.bone1, sizeof(char)*2, 1, fpw);
            fread(&topData->bdef4.bone2, sizeof(char)*2, 1, fpw);
            fread(&topData->bdef4.bone3, sizeof(char)*2, 1, fpw);
            fread(&topData->bdef4.bone4, sizeof(char)*2, 1, fpw);

            fread(&topData->bdef4.weight1, sizeof(float), 1, fpw);
            fread(&topData->bdef4.weight2, sizeof(float), 1, fpw);
            fread(&topData->bdef4.weight3, sizeof(float), 1, fpw);
            fread(&topData->bdef4.weight4, sizeof(float), 1, fpw);
            break;
        case 3:
            fread(&topData->sdef.bone1, sizeof(char)*2, 1, fpw);
            fread(&topData->sdef.bone2, sizeof(char)*2, 1, fpw);
            fread(&topData->sdef.weight1, sizeof(char)*2, 1, fpw);
            fread(&topData->sdef.SDEF_C, sizeof(float)*3, 1, fpw);
            fread(&topData->sdef.SDEF_R0, sizeof(float)*3, 1, fpw);
            fread(&topData->sdef.SDEF_R1, sizeof(float)*3, 1, fpw);
            break;

    }
    fread(&topData->edge_magnification, sizeof(float), 1, fpw);
}



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


char* path = "/home/shuta/ダウンロード/YYB Hatsune Miku_10th/YYB Hatsune Miku_10th_v1.02.pmx";
int main(){

    FILE *fpw = fopen(path, "rb");
    //ヘッダーの宣言
    struct Header header;
    fread(&header, sizeof(header), 1, fpw);

    //モデル情報の宣言
    struct Model model;
    getModelInfo(fpw, &model);

    //頂点データの宣言
    struct TopData topData[45104];
    for(int i = 0; i < 45103;i++) {
        getTopData(fpw, &topData[i], header);
        fseek(fpw, -12, SEEK_CUR);


        printf("変形方式:%d", topData[i].deformation_method[0]);
        printf("番号:%d", i);
        printf("座標（%f,%f,%f）", topData[i].location[0], topData[i].location[1], topData[i].location[2]);
        if(topData[i].deformation_method[0] == 2){
        }
    }

    printf("%d\n", topData[6239].deformation_method[0]);
    printf("カレント:%lx\n", ftell(fpw));
}