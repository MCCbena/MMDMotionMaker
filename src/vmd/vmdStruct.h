#ifndef vmdStruct_H
#define vmdStruct_H

#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

// ヘッダ
struct Header_vmd{
    char header[30]; // "Vocaloid Motion Data 0002\0\0\0\0\0" 30byte
// (MMDver2以前のvmdは"Vocaloid Motion Data file\0")
    char modelName[20]; // モデル名 20byte(MMDver2以前のvmdは10byte)
// 内容がカメラ,照明,セルフ影の場合は"カメラ・照明\0on Data"となる
};

struct MaxFrame {
    int maxFrame;
};

//ボーンキーフレーム要素データ(111Bytes/要素)
struct BoneFrame{
    char name[15]; // "センター\0"などのボーン名の文字列 15byte
    int frame; // フレーム番号
    float x; // ボーンのX軸位置,位置データがない場合は0
    float y; // ボーンのY軸位置,位置データがない場合は0
    float z; // ボーンのZ軸位置,位置データがない場合は0
    float qx; // ボーンのクォータニオンX回転,データがない場合は0
    float qy; // ボーンのクォータニオンY回転,データがない場合は0
    float qz; // ボーンのクォータニオンZ回転,データがない場合は0
    float qw; // ボーンのクォータニオンW回転,データがない場合は1
    char bezier[64]; // 補間パラメータ
};

typedef struct {
    struct Header_vmd header;
    struct MaxFrame maxFrame;
    struct BoneFrame *boneFrame;
} MotionData;

struct EncodeBoneFrame{
    float x;
    float y;
    float z;

    float qx;
    float qy;
    float qz;
    float qw;
};

struct NameIndexer{
    char name[32];
    int name_byte;
    int index;
};

typedef struct {
    struct EncodeBoneFrame **encodeBoneFrame;//[フレーム数][ボーン数]
    struct NameIndexer nameIndexer[1024];

    int encodeBoneFrame_size; //最大フレーム数を代入
    int nameIndexer_size;
}EncodeMotionData;

#endif