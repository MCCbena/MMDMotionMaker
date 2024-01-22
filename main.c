#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <wchar.h>
#include <iconv.h>
#include <string.h>

#define MAX_BUF 1024

#pragma pack(push, 1) // 構造体をきつくパッキングし、1バイトのアライメント
// ヘッダ
struct Header
{
    char header[30]; // "Vocaloid Motion Data 0002\0\0\0\0\0" 30byte
// (MMDver2以前のvmdは"Vocaloid Motion Data file\0")
    char modelName[20]; // モデル名 20byte(MMDver2以前のvmdは10byte)
// 内容がカメラ,照明,セルフ影の場合は"カメラ・照明\0on Data"となる
    unsigned int frame;
};

//ボーンキーフレーム要素データ(111Bytes/要素)
struct BoneFrame
{
    char name[15]; // "センター\0"などのボーン名の文字列 15byte
    unsigned int frame; // フレーム番号
    float x; // ボーンのX軸位置,位置データがない場合は0
    float y; // ボーンのY軸位置,位置データがない場合は0
    float z; // ボーンのZ軸位置,位置データがない場合は0
    float qx; // ボーンのクォータニオンX回転,データがない場合は0
    float qy; // ボーンのクォータニオンY回転,データがない場合は0
    float qz; // ボーンのクォータニオンZ回転,データがない場合は0
    float qw; // ボーンのクォータニオンW回転,データがない場合は1
    char bezier[64]; // 補間パラメータ
};
/*
補間パラメータは4点のベジェ曲線(0,0),(x1,y1),(x2,y2),(127,127)で
表している．各軸のパラメータを
X軸の補間パラメータ　(X_x1,X_y1),(X_x2,X_y2)
Y軸の補間パラメータ　(Y_x1,Y_y1),(Y_x2,Y_y2)
Z軸の補間パラメータ　(Z_x1,Z_y1),(Z_x2,Z_y2)
回転の補間パラメータ (R_x1,R_y1),(R_x2,R_y2)
とした時、補間パラメータは以下の通り.
X_x1,Y_x1,Z_x1,R_x1,X_y1,Y_y1,Z_y1,R_y1,
X_x2,Y_x2,Z_x2,R_x2,X_y2,Y_y2,Z_y2,R_y2,
Y_x1,Z_x1,R_x1,X_y1,Y_y1,Z_y1,R_y1,X_x2,
Y_x2,Z_x2,R_x2,X_y2,Y_y2,Z_y2,R_y2, 01,
Z_x1,R_x1,X_y1,Y_y1,Z_y1,R_y1,X_x2,Y_x2,
Z_x2,R_x2,X_y2,Y_y2,Z_y2,R_y2, 01, 00,
R_x1,X_y1,Y_y1,Z_y1,R_y1,X_x2,Y_x2,Z_x2,
R_x2,X_y2,Y_y2,Z_y2,R_y2, 01, 00, 00
*/


//表情キーフレーム要素データ(23Bytes/要素)
struct MorphFrame
{
    char name[15]; // "まばたき\0"などの表情名の文字列 15byte
    unsigned long frame; // フレーム番号
    float value; // 表情値(0?1)
};


//カメラキーフレーム要素データ(61Bytes/要素)
struct CameraFrame
{
    unsigned long frame; // フレーム番号
    float distance; // 目標点とカメラの距離(目標点がカメラ前面でマイナス)
    float x; // 目標点のX軸位置
    float y; // 目標点のY軸位置
    float z; // 目標点のZ軸位置
    float rx; // カメラのx軸回転(rad)(MMD数値入力のマイナス値)
    float ry; // カメラのy軸回転(rad)
    float rz; // カメラのz軸回転(rad)
    char bezier[24]; // 補間パラメータ
    unsigned long viewAngle; // 視野角(deg)
    char parth; // パースペクティブ, 0:ON, 1:OFF
};
/*
補間パラメータは4点のベジェ曲線(0,0),(x1,y1),(x2,y2),(127,127)で
表している.各軸のパラメータを
X軸の補間パラメータ　 (X_x1,X_y1),(X_x2,X_y2)
Y軸の補間パラメータ　 (Y_x1,Y_y1),(Y_x2,Y_y2)
Z軸の補間パラメータ　 (Z_x1,Z_y1),(Z_x2,Z_y2)
回転の補間パラメータ　(R_x1,R_y1),(R_x2,R_y2)
距離の補間パラメータ　(L_x1,L_y1),(L_x2,L_y2)
視野角の補間パラメータ(V_x1,V_y1),(V_x2,V_y2)
とした時、補間パラメータは以下の通り.
X_x1 X_x2 X_y1 X_y2
Y_x1 Y_x2 Y_y1 Y_y2
Z_x1 Z_x2 Z_y1 Z_y2
R_x1 R_x2 R_y1 R_y2
L_x1 L_x2 L_y1 L_y2
V_x1 V_x2 V_y1 V_y2
*/


//照明キーフレーム要素データ(28Bytes/要素)
struct LightFrame
{
    unsigned long frame; // フレーム番号
    float r; // 照明色赤(MMD入力値を256で割った値)
    float g; // 照明色緑(MMD入力値を256で割った値)
    float b; // 照明色青(MMD入力値を256で割った値)
    float x; // 照明x位置(MMD入力値)
    float y; // 照明y位置(MMD入力値)
    float z; // 照明z位置(MMD入力値)
};


//セルフ影キーフレーム要素データ(9Bytes/要素)
struct SelfShadowFrame
{
    unsigned long frame; // フレーム番号
    char type; // セルフシャドウ種類, 0:OFF, 1:mode1, 2:mode2
    float distance ; // シャドウ距離(MMD入力値Lを(10000-L)/100000とした値)
};


char* decode(char* string, int length){
    char inbuf[MAX_BUF + 1] = {0};
    char outbuf[MAX_BUF + 1] = {0};
    char *in = inbuf;
    char *out = outbuf;
    size_t in_size = (size_t) MAX_BUF;
    size_t out_size = (size_t) MAX_BUF;

    iconv_t cd = iconv_open("UTF-8", "SHIFT-JIS");

    memcpy(in, string, length);
    iconv(cd, &in, &in_size, &out, &out_size);
    iconv_close(cd);

    return strdup(outbuf);
}

int main(){
    struct Header header;
    struct BoneFrame boneFrame;

    FILE *fpw = fopen("/home/shuta/ダウンロード/人マニア（モーション配布）/人マニア.vmd", "rb");
    fread(&header, sizeof(header), 1, fpw);

    for(int i = 0; i < 150; i++) {
        fseek(fpw, 54 + (111*i), SEEK_SET);

        fread(&boneFrame, sizeof(boneFrame), 1, fpw);


        printf("--------------------------\n");
        printf("%s\n", decode(boneFrame.name, 15));

        printf("フレーム: %d\n", boneFrame.frame);
        printf("ボーン位置：(%f, %f, %f), (%f, %f, %f)\n", boneFrame.x, boneFrame.y, boneFrame.z,
               boneFrame.qx,boneFrame.qy, boneFrame.qz);
        printf("--------------------------\n");
    }

    printf("%s", decode(header.modelName, 20));
}