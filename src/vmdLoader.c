#include <stdio.h>
#include <json-c/json.h>
#include <iconv.h>
#include <string.h>

#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント
#define MAX_BUF1 1024

char* word_decode1(char* string, int length, char* toCode, char* fromCode){ //エンコードされている構造体ファイルのcharをshiftjisでデコード
    char inbuf[MAX_BUF1 + 1] = {0};
    char outbuf[MAX_BUF1 + 1] = {0};
    char *in = inbuf;
    char *out = outbuf;
    size_t in_size = (size_t) MAX_BUF1;
    size_t out_size = (size_t) MAX_BUF1;

    iconv_t cd = iconv_open(toCode, fromCode);

    memcpy(in, string, length);
    iconv(cd, &in, &in_size, &out, &out_size);
    iconv_close(cd);

    return strdup(outbuf);
}


// ヘッダ
struct Header
{
    char header[30]; // "Vocaloid Motion Data 0002\0\0\0\0\0" 30byte
// (MMDver2以前のvmdは"Vocaloid Motion Data file\0")
    char modelName[20]; // モデル名 20byte(MMDver2以前のvmdは10byte)
// 内容がカメラ,照明,セルフ影の場合は"カメラ・照明\0on Data"となる
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

const char *getMotion(const char* path){
    //構造体を宣言
    struct Header header;
    struct BoneFrame boneFrame;
    struct json_object* jsonMainObject = json_object_new_object();

    FILE *fpw = fopen(path, "rb");
    fread(&header, sizeof(header), 1, fpw);

    //構造体ファイルの読み過ぎに対応させるために必要
    int max_frame;
    fread(&max_frame, sizeof(int), 1, fpw);

    struct json_object* jsonBoneArrayObject = json_object_new_array();
    for (int i = 0; i < max_frame; i++) {
        struct json_object* jsonBoneObject = json_object_new_object();

        fread(&boneFrame, sizeof(boneFrame), 1, fpw);

        //jsonの作成
        //ボディ
        //ボーン名
        json_object_object_add(jsonBoneObject, "boneName", json_object_new_string(word_decode1(boneFrame.name, 15, "UTF-8", "SHIFT-JIS")));
        //フレーム
        json_object_object_add(jsonBoneObject, "frame", json_object_new_int((int) boneFrame.frame));
        //座標用の配列を作成
        struct json_object* locations = json_object_new_array();
        json_object_array_add(locations, json_object_new_double(boneFrame.x));
        json_object_array_add(locations, json_object_new_double(boneFrame.y));
        json_object_array_add(locations, json_object_new_double(boneFrame.z));
        json_object_object_add(jsonBoneObject, "locations", locations);
        //クォータニオン
        struct json_object* quaternion = json_object_new_array();
        json_object_array_add(quaternion, json_object_new_double(boneFrame.qx));
        json_object_array_add(quaternion, json_object_new_double(boneFrame.qy));
        json_object_array_add(quaternion, json_object_new_double(boneFrame.qz));
        json_object_array_add(quaternion, json_object_new_double(boneFrame.qw));
        json_object_object_add(jsonBoneObject, "quaternions", quaternion);
        //補完パラメータ
        struct json_object* bezierMainParameters = json_object_new_array();

        int current = 0;
        for(int j = 0; j < 2; j++) {
            //ベジュ曲線
            struct json_object* bezierParameters = json_object_new_array();
            for(int k = 0; k < 2; k++) {
                struct json_object* bezierLocations = json_object_new_array();
                for(int l = 0; l < 4; l++) {
                    json_object_array_add(bezierLocations, json_object_new_double(boneFrame.bezier[current]));
                    current++;
                }
                json_object_array_add(bezierParameters, bezierLocations);
            }
            json_object_array_add(bezierMainParameters,bezierParameters);
            //json_object_put(bezierParameters);
        }
        //ベジュ曲線
        json_object_object_add(jsonBoneObject, "bezier", bezierMainParameters);

        //最終代入
        json_object_array_add(jsonBoneArrayObject, jsonBoneObject);
    }
    //ヘッダーのjsonを作成
    struct json_object* jsonHeaderObject = json_object_new_object();
    json_object_object_add(jsonHeaderObject, "header", json_object_new_string(header.header));
    json_object_object_add(jsonHeaderObject, "modelName", json_object_new_string(word_decode1(header.modelName, 20, "UTF-8", "SHIFT-JIS")));
    json_object_object_add(jsonMainObject, "header", jsonHeaderObject);
    //ボーンをメインのjsonに代入
    json_object_object_add(jsonMainObject,"boneFrame", jsonBoneArrayObject);
    const char* return_json = strdup(json_object_to_json_string(jsonMainObject));

    json_object_put(jsonHeaderObject);
    json_object_put(jsonBoneArrayObject);
    json_object_put(jsonMainObject);
    return return_json;
}

int main(){
    printf("%s\n", getMotion("/home/shuta/ダウンロード/人マニア（モーション配布）/人マニア.vmd"));
    return 0;
}