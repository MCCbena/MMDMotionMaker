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
        char* name = (word_decode1(boneFrame.name, 15, "UTF-8", "SHIFT-JIS"));
        json_object_object_add(jsonBoneObject, "boneName", json_object_new_string(name));
        free(name);
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
        }
        //ベジュ曲線
        json_object_object_add(jsonBoneObject, "bezier", bezierMainParameters);

        //最終代入
        json_object_array_add(jsonBoneArrayObject, jsonBoneObject);
    }
    //ヘッダーのjsonを作成
    struct json_object* jsonHeaderObject = json_object_new_object();
    json_object_object_add(jsonHeaderObject, "header", json_object_new_string(header.header));
    char* word = word_decode1(header.modelName, 20, "UTF-8", "SHIFT-JIS");
    json_object_object_add(jsonHeaderObject, "modelName", json_object_new_string(word));
    free(word);
    json_object_object_add(jsonMainObject, "header", jsonHeaderObject);
    //最大フレームを格納
    json_object_object_add(jsonMainObject, "maxFrame", json_object_new_int(max_frame));
    //ボーンをメインのjsonに代入
    json_object_object_add(jsonMainObject,"boneFrame", jsonBoneArrayObject);

    char *return_json = strdup(json_object_to_json_string(jsonMainObject));
    json_object_put(jsonMainObject);
    return return_json;
}

void writeMotion(char* output_file_path, char* json){
    struct json_object* output_data = json_tokener_parse(json);

    //ヘッダーを取得-----------------------------------------------------
    struct Header header;
    struct json_object* header_object;
    json_object_object_get_ex(output_data, "header", &header_object);

    //ヘッダー名の読み込み
    struct json_object* header_name_object;
    json_object_object_get_ex(header_object, "header", &header_name_object);
    memcpy(header.header, word_decode1((char*) json_object_get_string(header_name_object), 30, "Shift-JIS", "UTF-8"), 30);
    //最大フレームを読み込み
    struct json_object* max_frame_object;
    json_object_object_get_ex(output_data, "maxFrame", &max_frame_object);
    int max_frame = json_object_get_int(max_frame_object);

    //モデル名の読み込み
    struct json_object* model_name_object;
    json_object_object_get_ex(header_object, "modelName", &model_name_object);
    memcpy(header.modelName, word_decode1((char* )json_object_get_string(model_name_object), 30, "Shift-JIS", "UTF-8"), 20);


    //ボーンフレームを取得---------------------------------------------------
    struct json_object* boneFrame_objects;
    json_object_object_get_ex(output_data, "boneFrame", &boneFrame_objects);

    //ボーンを配列として読み込み
    unsigned long  boneFrame_length = json_object_array_length(boneFrame_objects);
    struct BoneFrame boneFrame[boneFrame_length];

    //構造体に書き込み
    for (int i = 0; i < boneFrame_length; i++){
        struct json_object* boneFrame_object = json_object_array_get_idx(boneFrame_objects, i);
        printf("%s\n", json_object_get_string(boneFrame_object));

        //ボーン名の読み込み
        struct json_object* boneFrame_name_object;
        json_object_object_get_ex(boneFrame_object, "boneName", &boneFrame_name_object);
        memcpy(boneFrame[i].name, word_decode1((char* ) json_object_get_string(boneFrame_name_object), 30, "Shift-JIS", "UTF-8"), 15);

        //ボーンフレームの読み込み
        struct json_object* boneFame_frame_object;
        json_object_object_get_ex(boneFrame_object, "frame", &boneFame_frame_object);
        boneFrame[i].frame = json_object_get_int(boneFame_frame_object);

        //ボーンの座標
        struct json_object* boneFrame_locations_object;
        json_object_object_get_ex(boneFrame_object, "locations", &boneFrame_locations_object);
        for(int j = 0; j < 3; j++){
            struct json_object* boneFrame_location_object = json_object_array_get_idx(boneFrame_locations_object, j);
            float boneFrame_location = (float) json_object_get_double(boneFrame_location_object);

            //書き込み別
            switch (j) {
                case 0:
                    boneFrame[i].x = boneFrame_location;
                    break;
                case 1:
                    boneFrame[i].y = boneFrame_location;
                    break;
                case 2:
                    boneFrame[i].z = boneFrame_location;
                    break;
            }
        }

        //クォータニオンの読み込み
        struct json_object* boneFrame_quaternions_object;
        json_object_object_get_ex(boneFrame_object, "quaternions", &boneFrame_quaternions_object);
        for(int j = 0; j < 4; j++){
            struct json_object* boneFrame_quaternion_object = json_object_array_get_idx(boneFrame_quaternions_object, j);
            float boneFrame_quaternion = (float) json_object_get_double(boneFrame_quaternion_object);

            //書き込み別
            switch (j) {
                case 0:
                    boneFrame[i].qx = boneFrame_quaternion;
                    break;
                case 1:
                    boneFrame[i].qy = boneFrame_quaternion;
                    break;
                case 2:
                    boneFrame[i].qz = boneFrame_quaternion;
                    break;
                case 3:
                    boneFrame[i].qw = boneFrame_quaternion;
                    break;
            }
        }

        //ベジェ曲線の読み込み
        char boneFrame_bezier_positions[18];

        //めんどくさい超入れ子配列の読み込み
        int current = 0;
        struct json_object* boneFrame_beziers_object;
        json_object_object_get_ex(boneFrame_object, "bezier", &boneFrame_beziers_object);
        for(int j = 0; j < 2; j++){
            struct json_object* boneFrame_beziers_object1 = json_object_array_get_idx(boneFrame_beziers_object, j);
            for(int k = 0; k < 2; k++){
                struct json_object* boneFrame_beziers_object2 = json_object_array_get_idx(boneFrame_beziers_object1, k);
                for(int l = 0; l < 4; l++){
                    struct json_object* boneFrame_beziers_position = json_object_array_get_idx(boneFrame_beziers_object2, l);
                    boneFrame_bezier_positions[current] = (char )json_object_get_double(boneFrame_beziers_position);
                    current++;
                }
            }
        }
        //下のベジェで01と00を入れる場合は16, 17と宣言してるからここで別途入れる
        boneFrame_bezier_positions[16] = 1;
        boneFrame_bezier_positions[17] = 0;

        enum beziers{
            X_x1 = 0,X_y1 = 1,
            X_x2 = 2,X_y2 = 3,
            Y_x1 = 4,Y_y1 = 5,
            Y_x2 = 6,Y_y2 = 7,
            Z_x1 = 8,Z_y1 = 9,
            Z_x2 = 10,Z_y2 = 11,
            R_x1 = 12,R_y1 = 13,
            R_x2 = 14,R_y2 = 15,
            a01 = 16, a00 = 17
        };
        //書き込む順番を指定
        char writing_order[64] = {
                X_x1,Y_x1,Z_x1,R_x1,X_y1,Y_y1,Z_y1,R_y1,
                X_x2,Y_x2,Z_x2,R_x2,X_y2,Y_y2,Z_y2,R_y2,
                Y_x1,Z_x1,R_x1,X_y1,Y_y1,Z_y1,R_y1,X_x2,
                Y_x2,Z_x2,R_x2,X_y2,Y_y2,Z_y2,R_y2, a01,
                Z_x1,R_x1,X_y1,Y_y1,Z_y1,R_y1,X_x2,Y_x2,
                Z_x2,R_x2,X_y2,Y_y2,Z_y2,R_y2, a01, a00,
                R_x1,X_y1,Y_y1,Z_y1,R_y1,X_x2,Y_x2,Z_x2,
                R_x2,X_y2,Y_y2,Z_y2,R_y2, a01, a00, a00
        };
        //ベジェ本体に書き込み
        for (int j = 0; j < 64; j++) {
            boneFrame[i].bezier[j] = boneFrame_bezier_positions[writing_order[j]];
        }
    }

    //ファイルに書き込み------------------------------------------
    FILE *fpw = fopen(output_file_path, "w");
    fwrite(&header, 1, sizeof(header), fpw);//ヘッダーを書き込み
    fwrite(&max_frame, 1, sizeof(int), fpw);//最大フレームを書き込み
    fwrite(&boneFrame,1, sizeof(boneFrame), fpw);//ボーンフレーム
    fclose(fpw);
}

int main(){
    writeMotion("/home/shuta/ダウンロード/test/test.vmd", (char*) getMotion("/mnt/E86884F46884C334/D/src/motions/sm29023367/motion.vmd"));
    return 0;
}