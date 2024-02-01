#include <stdio.h>
#include <json-c/json.h>
#include "uitls/LangConv.h"

#define MAX_BUF 1024
#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

typedef struct {
    int byte_size; //バイト数
    char byte[1024]; //文字列（1024は仮）
} TextBuf;

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
    TextBuf model_name_jp; //モデル名
    TextBuf comment_jp; //コメント
    //英語版
    TextBuf model_name_en;
    TextBuf comment_en;

};

void getModelInfo(FILE *fpw, struct Model *model){
    //モデル名を取得
    fread(&model->model_name_jp.byte_size, sizeof(int), 1, fpw);
    fread(&model->model_name_jp.byte, model->model_name_jp.byte_size, 1, fpw);
    //英語版
    fread(&model->model_name_en.byte_size, sizeof(int), 1, fpw);
    fread(&model->model_name_en.byte, model->model_name_en.byte_size, 1, fpw);

    //モデルの説明を取得
    fread(&model->comment_jp.byte_size, sizeof(int), 1, fpw);
    fread(&model->comment_jp.byte, model->comment_jp.byte_size, 1, fpw);
    //英語版
    fread(&model->comment_en.byte_size, sizeof(int), 1, fpw);
    fread(&model->comment_en.byte, model->comment_en.byte_size, 1, fpw);
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
    //位置を設定
    fread(&topData->location, sizeof(float) * 3, 1, fpw);
    //法線を設定

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
            break;
        case 1:
            fread(&topData->bdef2.bone1, sizeof(char)*2, 1, fpw);
            fread(&topData->bdef2.bone2, sizeof(char)*2, 1, fpw);
            fread(&topData->bdef2.weight1, sizeof(float), 1, fpw);

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
            fread(&topData->sdef.weight1, sizeof(float), 1, fpw);
            fread(&topData->sdef.SDEF_C, sizeof(float)*3, 1, fpw);
            fread(&topData->sdef.SDEF_R0, sizeof(float)*3, 1, fpw);
            fread(&topData->sdef.SDEF_R1, sizeof(float)*3, 1, fpw);
            break;

    }
    fread(&topData->edge_magnification, sizeof(float), 1, fpw);
}

struct Surface{
    unsigned short face_vert_index[3]; //参照頂点
};

void getSurface(struct Surface *surface, FILE *fpw){

    for(int i=0; i < 3; i++) {
        fread(&surface->face_vert_index[i], 2, 1, fpw);
    }
}

struct Texture{
    TextBuf path; //テクスチャパス
};

void getTexture(struct Texture *texture, FILE *fpw){
    fread(&texture->path.byte_size, sizeof(int), 1, fpw);
    fread(&texture->path.byte, texture->path.byte_size, 1, fpw);
}

struct Material{
    TextBuf materialName_jp; //素材名
    TextBuf materialName_en;

    float diffuse[4]; //Diffuse (R,G,B,A)
    float specular[3]; //Specular (R,G,B)
    float specular_coefficient; //Specular係数
    float ambient[3]; //ambient (R,G,B)

    char drawing_flag; //描画フラグ(8bit) TODO 一時的にcharにしてるけど、bit flagを作ること。

    float edge_color[4]; //エッジ色 (R,G,B,A)
    float edge_size; //エッジサイズ

    char *normal_texture_index; //通常テクスチャ, テクスチャテーブルの参照Index
    char *sphere_texture_index; //スフィアテクスチャ, テクスチャテーブルの参照Index  ※テクスチャ拡張子の制限なし
    char sphere_mode; //スフィアモード 0:無効 1:乗算(sph) 2:加算(spa) 3:サブテクスチャ(追加UV1のx,yをUV参照して通常テクスチャ描画を行う)

    char share_toon_flag; //共有Toonフラグ 0:継続値は個別Toon 1:継続値は共有Toon

    char toon[1]; //Toonのデータ

    TextBuf memo; //メモ : 自由欄／スクリプト記述／エフェクトへのパラメータ配置など

    int vertex_size; //材質に対応する面(頂点)数 (必ず3の倍数になる)
};
void getMaterialData(struct Header header, struct Material *material, FILE *fpw){
    //素材の名前を設定
    fread(&material->materialName_jp.byte_size, sizeof(int), 1, fpw);
    fread(&material->materialName_jp.byte, material->materialName_jp.byte_size, 1, fpw);
    //英語版
    fread(&material->materialName_en, sizeof(int), 1, fpw);
    fread(&material->materialName_en.byte, material->materialName_en.byte_size, 1, fpw);
    //Diffuse, Specular, Specular係数, Ambientを順に設定
    fread(&material->diffuse, sizeof(float)*4, 1, fpw);
    fread(&material->specular, sizeof(float)*3, 1, fpw);
    fread(&material->specular_coefficient, sizeof(float), 1, fpw);
    fread(&material->ambient, sizeof(float)*3, 1, fpw);
    //描画フラグを設定
    fread(&material->drawing_flag, sizeof(char), 1, fpw);
    //エッジ系
    fread(&material->edge_color, sizeof(float)*4, 1, fpw);
    fread(&material->edge_size, sizeof(float), 1, fpw);
    //通常テクスチャ
    fread(&material->normal_texture_index, header.texture_index_size[0], 1, fpw);
    //スフィアテクスチャ
    fread(&material->sphere_texture_index, header.texture_index_size[0], 1, fpw);
    //スフィアモード
    fread(&material->sphere_mode, sizeof(char),1 , fpw);
    //共有Toonフラグ
    fread(&material->share_toon_flag, sizeof(char), 1, fpw);
    switch (material->share_toon_flag) {
        case 0: //個別Toon
            fread(&material->toon, header.texture_index_size[0], 1, fpw);
            break;
        case 1: //共有Toon
            fread(&material->toon, sizeof(char), 1, fpw);
            break;
    }
    //メモ
    fread(&material->memo.byte_size, sizeof(int), 1, fpw);
    fread(&material->memo.byte, material->memo.byte_size, 1, fpw);
    //材質に対応する面(頂点)数 (必ず3の倍数になる)
    fread(&material->vertex_size, sizeof(int), 1, fpw);
}


typedef struct{ //接続先0
    float location_offset[3]; //座標オフセット, ボーン位置からの相対分
}Connect0;
typedef struct{ //接続先1
    char *connected_bone_index; //ボーンIndexサイズ  | 接続先ボーンのボーンIndex
}Connect1;

typedef struct { //回転付与:1 または 移動付与:1
    char *parent_bone_index; //付与親ボーンのボーンIndex
    float grant_rate; //付与率
}Imparted;

typedef struct { //軸固定
    float shaft_vector[3]; //軸の方向ベクトル
}FixedShaft;
typedef struct { //ローカル軸
    float x_vector[3]; //X軸方向のベクトル
    float z_vector[3]; //y軸方向のベクトル
}LocalShaft;

typedef struct { //外部親変形
    int key; //キー値
}Deformation;

typedef struct { //IKリンク
    char *linkBone_index_size; //リンクボーンのボーンIndex
    char limit_angele; //角度制限 0:OFF 1:ON

    float lower_limit[3]; //下限 (x,y,z) -> ラジアン角
    float upper_limit[3]; //上限 (x,y,z) -> ラジアン角
} IKLink;
typedef struct {
    char *IK_targetBone_index_size; //IKターゲットボーンのボーンIndex
    int IK_loop_count; //IKループ回数
    float IK_limit_angle; //IKループ計算時の1回あたりの制限角度 -> ラジアン角

    int IK_link_count; //後続の要素数

    IKLink ikLink[4]; //IKリンクの要素
}IK;

struct Bone{
    TextBuf model_name_jp; //ボーンの名前
    TextBuf model_name_en; //英語版

    float locations[3]; //位置

    char *parent_bone_index; //親ボーンのボーンIndex
    int transformation_hierarchy; //変形階層

    short bone_flags; //TODO ここにはbitflagが入る

    //bone_flagsの値で変動する
    Connect0 connect0;
    Connect1 connect1;
    Imparted imparted;
    FixedShaft fixedShaft;
    LocalShaft localShaft;
    Deformation deformation;
    IK ik;
};

void getBone(struct Header header, struct Bone *bone, FILE *fpw){
    //ボーン名の書き込み
    fread(&bone->model_name_jp.byte_size, sizeof(int), 1, fpw);
    fread(&bone->model_name_jp.byte, bone->model_name_jp.byte_size, 1, fpw);
    //英語版
    fread(&bone->model_name_en.byte_size, sizeof(int), 1, fpw);
    fread(&bone->model_name_en.byte, bone->model_name_en.byte_size, 1, fpw);
    //位置の書き込み
    fread(&bone->locations, sizeof(float)*3, 1, fpw);
    //親ボーンのボーンindex
    fread(&bone->parent_bone_index, header.bone_index_size[0], 1, fpw);
    //変形階層の書き込み
    fread(&bone->transformation_hierarchy, sizeof(int), 1, fpw);
    //ボーンフラグの書き込み
    fread(&bone->bone_flags, sizeof(short), 1, fpw);
    //ボーンフラグの選択
    if ((bone->bone_flags & 0x0001) == 0){ //接続0の場合
        fread(&bone->connect0, sizeof(Connect0), 1, fpw);
    }
    if ((bone->bone_flags & 0x0001) == 1){ //接続1の場合
        fread(&bone->connect1, header.bone_index_size[0], 1, fpw);
    }
    if ((bone->bone_flags & 0x0100) != 0 || (bone->bone_flags & 0x0200) != 0){ //回転付与 or 移動付与が1の場合
        fread(&bone->imparted.parent_bone_index, header.bone_index_size[0], 1, fpw);
        fread(&bone->imparted.grant_rate, sizeof(float), 1, fpw);
    }
    if ((bone->bone_flags & 0x0400) != 0){ //軸固定が1の場合
        fread(&bone->fixedShaft, sizeof(FixedShaft), 1, fpw);
    }
    if ((bone->bone_flags & 0x0800) != 0){ //ローカル軸が1の場合
        fread(&bone->localShaft, sizeof(LocalShaft), 1, fpw);
    }
    if ((bone->bone_flags & 0x2000) != 0){ //外部親変形が1の場合
        fread(&bone->deformation, sizeof(Deformation), 1, fpw);
    }
    if ((bone->bone_flags & 0x0020) != 0){ //IKが1の場合
        fread(&bone->ik.IK_targetBone_index_size, header.bone_index_size[0], 1, fpw);
        fread(&bone->ik.IK_loop_count, sizeof(int), 1, fpw);
        fread(&bone->ik.IK_limit_angle, sizeof(float), 1, fpw);

        fread(&bone->ik.IK_link_count, sizeof(int), 1, fpw);
        for(int i = 0; i < bone->ik.IK_link_count; i++){
            IKLink ikLink;
            fread(&ikLink.linkBone_index_size, header.bone_index_size[0], 1, fpw);
            fread(&ikLink.limit_angele, sizeof(char), 1, fpw);
            if(ikLink.limit_angele == 1){
                fread(&ikLink.lower_limit, sizeof(float)*3, 1, fpw);
                fread(&ikLink.upper_limit, sizeof(float)*3, 1, fpw);
            }
            //ikLinksへ書き込み
            bone->ik.ikLink[i] = ikLink;
        }
    }
}


char* getModel(const char* path){
    FILE *fpw = fopen(path, "rb");
    //ヘッダーの宣言
    struct Header header;
    fread(&header, sizeof(header), 1, fpw);

    //モデル情報の宣言
    struct Model model;
    getModelInfo(fpw, &model);

    //頂点データの宣言
    printf("カレント:%lx\n", ftell(fpw));
    int top_len;
    fread(&top_len, sizeof(int), 1, fpw);
    printf("頂点サイズ:%d\n", top_len);
    struct TopData topData[top_len];
    for(int i = 0; i < top_len;i++) {
        getTopData(fpw, &topData[i], header);
    }

    //面データ
    int surface_len;
    fread(&surface_len, sizeof(int), 1, fpw);
    surface_len = surface_len/3;
    printf("カレント:%lx\n", ftell(fpw));
    printf("面サイズ:%d\n", surface_len);
    struct Surface surface[surface_len];
    for(int i = 0; i < surface_len; i++){
        getSurface(&surface[i], fpw);
    }

    //テクスチャデータ
    int texture_size;
    fread(&texture_size, sizeof(int),1, fpw);
    printf("テクスチャデータサイズ:%d\n", texture_size);
    for(int i = 0; i < texture_size; i++){
        struct Texture texture;
        getTexture(&texture, fpw);
        printf("テクスチャパス:%s, %d\n", decode(texture.path.byte, texture.path.byte_size, "UTF-8", "UTF-16"), i);
    }

    //素材データ
    int material_size;
    fread(&material_size, sizeof(int), 1, fpw);
    struct Material material[material_size];
    for(int i = 0; i < material_size; i++){
        getMaterialData(header, &material[i], fpw);

    }

    //ボーンデータ
    int bone_size;
    fread(&bone_size, sizeof(int), 1, fpw);
    printf("カレント:%lx\n", ftell(fpw));

    struct Bone bone[bone_size];
    for(int i = 0; i < bone_size; i++){
        getBone(header, &bone[i], fpw);
    }

    //jsonの生成
    struct json_object* jsonMainObject = json_object_new_object(); //メインのjson

    //ボーン
    struct json_object* jsonMainBoneObject = json_object_new_array(); //ボーンのメインjson
    for(int i = 0; i < bone_size; i++){
        struct json_object* jsonBoneObject = json_object_new_object();
        json_object_object_add(jsonBoneObject, "name", json_object_new_string(decode(bone[i].model_name_jp.byte, bone[i].model_name_jp.byte_size, "UTF-8", "UTF-16")));
        //jsonを代入
        json_object_array_add(jsonMainBoneObject,jsonBoneObject);
    }
    json_object_object_add(jsonMainObject, "bones", jsonMainBoneObject);
    printf("%s", json_object_to_json_string(jsonMainObject));
}