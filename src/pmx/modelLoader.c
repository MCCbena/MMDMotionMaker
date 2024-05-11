#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

#include <stdio.h>
#include <malloc.h>
#include "pmxStruct.h"

void getModel(const char* path, struct Model *model){
    
    FILE *fpw = fopen(path, "rb");

    //ヘッダーの宣言
    struct Header_pmx header;
    fread(&header, sizeof(header), 1, fpw);
    model->header = header;

    //モデル情報の宣言
    struct ModelInfo modelInfo;
    getModelInfo(fpw, &modelInfo);
    model->modelInfo = modelInfo;


    //頂点データの宣言
    int top_len;
    fread(&top_len, sizeof(int), 1, fpw);
    model->topData_size = top_len;
    model->topData = (struct TopData*)malloc(sizeof(struct TopData) * top_len);
    for(int i = 0; i < top_len;i++) {
        struct TopData topData;
        getTopData(fpw, &topData, header);
        model->topData[i] = topData;
    }

    //面データ
    int surface_len;
    fread(&surface_len, sizeof(int), 1, fpw);
    surface_len = surface_len/3;
    model->surface_size = surface_len;
    model->surface = (struct Surface*)malloc(sizeof(struct Surface) * surface_len);
    for(int i = 0; i < surface_len; i++){
        struct Surface surface;
        getSurface(&surface, fpw);

        model->surface[i] = surface;
    }



    //テクスチャデータ
    int texture_size;
    fread(&texture_size, sizeof(int),1, fpw);
    model->texture_size = texture_size;
    model->texture = (struct Texture*) malloc(sizeof(struct Texture)*texture_size);
    for(int i = 0; i < texture_size; i++){
        struct Texture texture;
        getTexture(&texture, fpw);
        model->texture[i] = texture;
    }

    //素材データ
    int material_size;
    fread(&material_size, sizeof(int), 1, fpw);
    model->material_size = material_size;
    model->material = (struct Material*) malloc(sizeof(struct Material) * material_size);
    for(int i = 0; i < material_size; i++){
        struct Material material;

        getMaterialData(header, &material, fpw);
        model->material[i] = material;
    }

    //ボーンデータ
    int bone_size;
    fread(&bone_size, sizeof(int), 1, fpw);
    model->bone_size = bone_size;
    model->bone = (struct Bone*) malloc(sizeof(struct Bone) * bone_size);
    for(int i = 0; i < bone_size; i++){
        struct Bone bone;
        getBone(header, &bone, fpw);
        model->bone[i] = bone;
    }

}