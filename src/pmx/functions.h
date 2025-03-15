//
// Created by shuta on 25/01/06.
//

#ifndef TEST_FUNCTIONS_H
#define TEST_FUNCTIONS_H

#pragma pack(1)
#include "pmxStruct.h"
#include <stdlib.h>


static void getModelInfo(FILE *fpw, struct ModelInfo *model){
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

static void getTopData(FILE *fpw, struct TopData *topData, struct Header_pmx header){
    //位置を設定
    fread(&topData->location, sizeof(float) * 3, 1, fpw);
    //法線を設定

    fread(&topData->normal, sizeof(float) * 3, 1, fpw);
    //UVを設定
    fread(&topData->uv, sizeof(float) * 2, 1, fpw);
    //追加UVを設定

    int n = header.additional_UV_size;
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

static void getSurface(struct Surface *surface, FILE *fpw){

    for(int i=0; i < 3; i++) {
        fread(&surface->face_vert_index[i], 2, 1, fpw);
    }
}

static void getTexture(struct Texture *texture, FILE *fpw){
    fread(&texture->path.byte_size, sizeof(int), 1, fpw);
    fread(&texture->path.byte, texture->path.byte_size, 1, fpw);
}

static void getMaterialData(struct Header_pmx header, struct Material *material, FILE *fpw){
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
    fread(&material->normal_texture_index, header.texture_index_size, 1, fpw);
    //スフィアテクスチャ
    fread(&material->sphere_texture_index, header.texture_index_size, 1, fpw);
    //スフィアモード
    fread(&material->sphere_mode, sizeof(char),1 , fpw);
    //共有Toonフラグ
    fread(&material->share_toon_flag, sizeof(char), 1, fpw);
    switch (material->share_toon_flag) {
        case 0: //個別Toon
            fread(&material->toon, header.texture_index_size, 1, fpw);
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

static void getBone(struct Header_pmx header, struct Bone *bone, FILE *fpw){
    //初期化
    bone->child_bone_size=0;
    bone->child_bones = malloc(sizeof(int)*32);//32は子ボーンの最大値
    for(int i = 0; i < 1024; i++) bone->model_name_jp.byte[i]=0;
    //ボーン名の書き込み
    fread(&bone->model_name_jp.byte_size, sizeof(int), 1, fpw);
    fread(&bone->model_name_jp.byte, bone->model_name_jp.byte_size, 1, fpw);
    //英語版
    fread(&bone->model_name_en.byte_size, sizeof(int), 1, fpw);
    fread(&bone->model_name_en.byte, bone->model_name_en.byte_size, 1, fpw);
    //位置の書き込み
    fread(&bone->locations, sizeof(float)*3, 1, fpw);
    //親ボーンのボーンindex
    fread(&bone->parent_bone_index, header.bone_index_size, 1, fpw);
    //変形階層の書き込み
    fread(&bone->transformation_hierarchy, sizeof(int), 1, fpw);
    //ボーンフラグの書き込み
    fread(&bone->bone_flags, sizeof(short), 1, fpw);
    //ボーンフラグの選択
    if ((bone->bone_flags & 0x0001) == 0){ //接続0の場合
        fread(&bone->connect0, sizeof(Connect0), 1, fpw);
    }
    if ((bone->bone_flags & 0x0001) == 1){ //接続1の場合
        fread(&bone->connect1, header.bone_index_size, 1, fpw);
    }
    if ((bone->bone_flags & 0x0100) != 0 || (bone->bone_flags & 0x0200) != 0){ //回転付与 or 移動付与が1の場合
        fread(&bone->imparted.parent_bone_index, header.bone_index_size, 1, fpw);
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
        fread(&bone->ik.IK_targetBone_index_size, header.bone_index_size, 1, fpw);
        fread(&bone->ik.IK_loop_count, sizeof(int), 1, fpw);
        fread(&bone->ik.IK_limit_angle, sizeof(float), 1, fpw);

        fread(&bone->ik.IK_link_count, sizeof(int), 1, fpw);
        for(int i = 0; i < bone->ik.IK_link_count; i++){
            IKLink ikLink;
            fread(&ikLink.linkBone_index_size, header.bone_index_size, 1, fpw);
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

#endif //TEST_FUNCTIONS_H
