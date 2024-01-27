#include <stdio.h>
#include <iconv.h>
#include <string.h>
#include <json-c/json.h>

#define MAX_BUF 1024

#pragma pack(push, 1) // 構造体をきつくパッキングし、1バイトのアライメント
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

const char *getMotion(const char* path){
    struct Header header;
    struct BoneFrame boneFrame;
    struct json_object* jsonMainObject = json_object_new_object();

    FILE *fpw = fopen(path, "rb");
    fread(&header, sizeof(header), 1, fpw);

    int last_frame = 0;

    int i = 0;

    struct json_object* jsonBoneArrayObject = json_object_new_array();
    while (1) {
        struct json_object* jsonBoneObject = json_object_new_object();

        fseek(fpw, 54 + (111*i), SEEK_SET);
        fread(&boneFrame, sizeof(boneFrame), 1, fpw);

        if((int) boneFrame.frame - last_frame < 0){
            break;
        }

        last_frame = (int) boneFrame.frame;

        i++;
        //jsonの作成
        //ボディ
        //ボーン名
        json_object_object_add(jsonBoneObject, "boneName", json_object_new_string(decode(boneFrame.name, 15)));
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

        json_object_array_add(jsonBoneArrayObject, jsonBoneObject);
    }
    //ヘッダーのjsonを作成
    struct json_object* jsonHeaderObject = json_object_new_object();
    json_object_object_add(jsonHeaderObject, "header", json_object_new_string(header.header));
    json_object_object_add(jsonHeaderObject, "modelName", json_object_new_string(decode(header.modelName, 20)));
    json_object_object_add(jsonMainObject, "header", jsonHeaderObject);
    //ボーンをメインのjsonに代入
    json_object_object_add(jsonMainObject,"boneFrame", jsonBoneArrayObject);


    return json_object_get_string(jsonMainObject);
}

int main(){}