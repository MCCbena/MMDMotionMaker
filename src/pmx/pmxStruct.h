#ifndef pmxStruct_H
#define pmxStruct_H

#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント
#include <stdio.h>


typedef struct {
    int byte_size; //バイト数
    char byte[1024]; //文字列（1024は仮）
} TextBuf;

struct Header_pmx {
    //マジックナンバー
    char header[4]; //"PMX "
    float version; //PMXのバージョン

    //バイト列
    char byte_size; //後続するデータのバイトサイズ（PMX2.0は8で固定）
    char encode; //エンコード方式（0:UTF16 | 1:UTF8）
    unsigned char additional_UV_size; //追加UV数（0〜4）

    char top_index_size; //頂点indexサイズ（1,2,4のどれか）
    char texture_index_size; //テクスチャindexサイズ（1,2,4のどれか）
    char material_index_size; //素材indexサイズ（以下略
    char bone_index_size; //ボーン（以下略
    char morph_index_size; //モーフ
    char rigidBody_index_size; //剛体
};

struct ModelInfo {
    TextBuf model_name_jp; //モデル名
    TextBuf comment_jp; //コメント
    //英語版
    TextBuf model_name_en;
    TextBuf comment_en;

};

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

struct Surface{
    unsigned short face_vert_index[3]; //参照頂点
};


struct Texture{
    TextBuf path; //テクスチャパス
};


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

    short parent_bone_index; //親ボーンのボーンIndex
    int transformation_hierarchy; //変形階層

    short bone_flags;

    //bone_flagsの値で変動する
    Connect0 connect0;
    Connect1 connect1;
    Imparted imparted;
    FixedShaft fixedShaft;
    LocalShaft localShaft;
    Deformation deformation;
    IK ik;

    int child_bone_size; //子ボーンのインデックスサイズ
    int *child_bones; //チェイン法により、子ボーンのインデックスが代入される
};

struct Model{
    struct Header_pmx header;
    struct ModelInfo modelInfo;
    int topData_size;
    struct TopData* topData;
    int surface_size;
    struct Surface* surface;
    int texture_size;
    struct Texture* texture;
    int material_size;
    struct Material* material;
    int bone_size;
    struct Bone *bone;
};

#endif