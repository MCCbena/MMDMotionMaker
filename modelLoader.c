#include <stdio.h>
#include <iconv.h>
#include <string.h>
#include <json-c/json.h>

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
    fseek(fpw, 8, SEEK_CUR);

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
    fseek(fpw, 4, SEEK_CUR);
}

struct Surface{
    unsigned short face_vert_index[3]; //参照頂点
};

void getSurface(struct Surface *surface, FILE *fpw){

    for(int i=0; i < 3; i++) {
        char face_vert_index[2];
        fread(&face_vert_index, 2, 1, fpw);

        surface->face_vert_index[i] = *((short*)face_vert_index);
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
    TextBuf materialName; //素材名

    float diffuse[4]; //Diffuse (R,G,B,A)
    float specular[3]; //Specular (R,G,B)
    float specular_coefficient; //Specular係数
    float ambient[3]; //ambient (R,G,B)

    char drawing_flag; //描画フラグ(8bit) TODO 一時的にcharにしてるけど、bit flagを作ること。

    float edge_color[4]; //エッジ色 (R,G,B,A)
    float edge_size; //エッジサイズ

    char normal_texture_index[1]; //通常テクスチャ, テクスチャテーブルの参照Index
    char sphere_texture_index[1]; //スフィアテクスチャ, テクスチャテーブルの参照Index  ※テクスチャ拡張子の制限なし
    char sphere_mode; //スフィアモード 0:無効 1:乗算(sph) 2:加算(spa) 3:サブテクスチャ(追加UV1のx,yをUV参照して通常テクスチャ描画を行う)

    char share_toon_flag; //共有Toonフラグ 0:継続値は個別Toon 1:継続値は共有Toon

    char toon[1]; //Toonのデータ

    TextBuf memo; //メモ : 自由欄／スクリプト記述／エフェクトへのパラメータ配置など

    int vertex_size; //材質に対応する面(頂点)数 (必ず3の倍数になる)
};
void getMaterialData(struct Header header, struct Material *material, FILE *fpw){
    //素材の名前を設定
    fread(&material->materialName.byte_size, sizeof(int), 1, fpw);
    fread(&material->materialName.byte, material->materialName.byte_size, 1, fpw);
    //Diffuse, Specular, Specular係数, Ambientを順に設定
    fseek(fpw, 4, SEEK_CUR);
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
    printf("カレント:%lx\n", ftell(fpw));
    fread(&material->memo.byte, material->memo.byte_size, 1, fpw);
    //材質に対応する面(頂点)数 (必ず3の倍数になる)
    fread(&material->vertex_size, sizeof(int), 1, fpw);
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
    fseek(fpw, 4, SEEK_CUR);
    printf("カレント:%lx\n", ftell(fpw));
    int top_len;
    fread(&top_len, sizeof(int), 1, fpw);
    printf("頂点サイズ:%d\n", top_len);
    struct TopData topData[top_len];
    for(int i = 0; i < top_len;i++) {
        getTopData(fpw, &topData[i], header);
        fseek(fpw, -12, SEEK_CUR);
    }

    //面データ
    fseek(fpw, 8, SEEK_CUR);
    int surface_len;
    fread(&surface_len, sizeof(int), 1, fpw);
    fseek(fpw, 0, SEEK_CUR);
    printf("カレント:%lx\n", ftell(fpw));
    printf("面サイズ:%d\n", surface_len);
    struct Surface surface[74805];
    for(int i = 0; i < 74805; i++){
        getSurface(&surface[i], fpw);
    }

    //テクスチャデータ
    int texture_size;
    fread(&texture_size, sizeof(int),1, fpw);
    printf("テクスチャデータサイズ:%d\n", texture_size);
    for(int i = 0; i < texture_size; i++){
        struct Texture texture;
        getTexture(&texture, fpw);
        printf("テクスチャパス:%s, %d\n", decode(texture.path.byte, texture.path.byte_size), i);
    }

    //素材データ
    int material_size;
    fread(&material_size, sizeof(int), 1, fpw);
    struct Material material[material_size];
    for(int i = 0; i < material_size; i++){
        getMaterialData(header, &material[i], fpw);

    }
    printf("サイズ:%d\n", material_size);
}