#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

#include <stdio.h>
#include "vmdStruct.h"
#include <stdbool.h>
#include "indexlib.h"
#include <math.h>
#include <stdlib.h>
#include "../pmx/modelLoader.c"

#define deg_to_rad(deg) ((deg)*M_PI/180)

float getPos(const float pos1, const float pos2, float time){
    return (3* powf(1.0f-time, 2)*time*pos1+3.0f* (1-time)* powf(time, 2)*pos2+ powf(time, 3));
}

struct Quaternion{
    float w;
    float x;
    float y;
    float z;
};
struct Quaternion SphericalLinearInterpolation(const float *q0, const float *q1, const float t){
    float q[4];
    float dot = q0[0] * q1[0] + q0[1] * q1[1] + q0[2] * q1[2] + q0[3] * q1[3];

    if (dot > 0.99999){//内積が1だったら計算する意味なし
        struct Quaternion quaternion;
        quaternion.w = q0[0];
        quaternion.x = q0[1];
        quaternion.y = q0[2];
        quaternion.z = q0[3];

        return quaternion;
    }

    float q0_copy[4];
    memcpy(q0_copy, q0, sizeof(float)*4);
    float q1_copy[4];
    memcpy(q1_copy, q1, sizeof(float)*4);
    float phi = acosf(dot);
    if(dot < 0.0f){//内積が負の場合、微調整
        phi = acosf(-dot);
        for(int i = 0; i < 4; i++){
            q1_copy[i] = -q1_copy[i];
        }
    } else{
        for(int i = 0; i < 4; i++) {//負でない場合反転
            q0_copy[i] = -1 * q0_copy[i];
            q1_copy[i] = -1 * q1_copy[i];
        }
    }

    float sin_phi = sinf(deg_to_rad(phi));

    for(int i = 0; i < 4; i++){
        if(q0[i] != q1[i]) {//計算ショートカット
            q[i] = (sinf(deg_to_rad(1 - t))*phi / sin_phi*q0_copy[i]) + (sinf(deg_to_rad(t * phi)) / sin_phi*q1_copy[i]);
        }else q[i] = q0[i];
    }

    struct Quaternion quaternion;
    quaternion.w = q[0];
    quaternion.x = q[1];
    quaternion.y = q[2];
    quaternion.z = q[3];

    return quaternion;
}

MotionData getMotion(const char* path, bool frame_completion){
    //構造体を宣言

    MotionData motionData;

    FILE *fpw = fopen(path, "rb");
    fread(&motionData.header, 50, 1, fpw);
    fread(&motionData.maxFrame, 4, 1, fpw);

    motionData.boneFrame = malloc(111 * motionData.maxFrame.maxFrame);
    fread(&*motionData.boneFrame, 111, motionData.maxFrame.maxFrame, fpw);

    //データ補完が有効な場合
    if(frame_completion){

        //最終フレームの取得とindexの作成
        int max_frame = 0;
        struct Index index = makeIndex();

        for(int i = 0; i < motionData.maxFrame.maxFrame; i++){

            if (max_frame < motionData.boneFrame[i].frame){
                //フレーム
                max_frame = motionData.boneFrame[i].frame;
            }
            //インデックス
            if (getnIndex(index, motionData.boneFrame[i].name, 15) == -1){
                addIndex(&index, motionData.boneFrame[i].name, 15);
            }
        }
        max_frame++;


        //フレームを補完
        struct BoneFrame **boneFrame = (struct BoneFrame**) malloc(sizeof(struct BoneFrame*) * index.assigned);
        for(int i = 0; i < index.assigned; i++){
            boneFrame[i] = malloc(sizeof(struct BoneFrame) * max_frame);
        }
        for(int i = 0; i < motionData.maxFrame.maxFrame; i++){
            struct BoneFrame temp = motionData.boneFrame[i];
            int id = getnIndex(index, temp.name, 15);
            boneFrame[id][temp.frame] = temp;
        }

        for(int i = 0; i < index.assigned; i++){//ボーンを一つづつループ
            int start = 0;
            int end;
            float position_increment;
            //補完に必要な一時変数
            float pos1, pos2;
            for(int j = 0; j < max_frame; j++){//最大フレーム回ループ

                if (boneFrame[i][j].name[0] == 0 && j != max_frame-1){//フレームが未登録だった場合
                    if(start == 0){
                        start = j;//未登録フレームの開始地点
                    }
                } else{
                    if(start != 0){
                        end = j;//startが登録されていればendを登録
                        //フレームの補完
                        float time_count_base = (1/(float)(1+(end-start)));
                        struct BoneFrame final_registration_boneFrame = boneFrame[i][start-1];
                        for(int k = start; k < end; k++){
                            memcpy(boneFrame[i][k].name, index.name[i], 15);//名前の設定
                            boneFrame[i][k].frame = k;//フレームの設定
                            //printf("---開始:%f-----終了:%f\n", final_registration_boneFrame.qy, boneFrame[i][end].qy);

                            //座標x
                            if(final_registration_boneFrame.x != boneFrame[i][end].x) {//計算ショートカット
                                pos1 = (float) final_registration_boneFrame.bezier[4] / 127;
                                pos2 = (float) final_registration_boneFrame.bezier[12] / 127;
                                position_increment = boneFrame[i][end].x - final_registration_boneFrame.x;
                                float x = final_registration_boneFrame.x + position_increment * getPos(pos1, pos2,time_count_base *(float) (k -start +1));
                                boneFrame[i][k].x = x;
                            }else boneFrame[i][k].x = final_registration_boneFrame.x;
                            //座標y
                            if(final_registration_boneFrame.y != boneFrame[i][end].y) {
                                pos1 = (float) final_registration_boneFrame.bezier[5] / 127;
                                pos2 = (float) final_registration_boneFrame.bezier[13] / 127;
                                position_increment = boneFrame[i][end].y - final_registration_boneFrame.y;
                                float y = final_registration_boneFrame.y + position_increment * getPos(pos1, pos2,time_count_base *(float) (k -start +1));
                                boneFrame[i][k].y = y;
                            }else boneFrame[i][k].y = final_registration_boneFrame.y;
                            //座標z
                            if(final_registration_boneFrame.z != boneFrame[i][end].z) {
                                pos1 = (float) final_registration_boneFrame.bezier[6] / 127;
                                pos2 = (float) final_registration_boneFrame.bezier[14] / 127;
                                position_increment = boneFrame[i][end].z - final_registration_boneFrame.z;
                                float z = final_registration_boneFrame.z + position_increment * getPos(pos1, pos2,time_count_base *(float) (k -start +1));
                                boneFrame[i][k].z = z;
                            } else boneFrame[i][k].z = final_registration_boneFrame.z;


                            //クォータニオン
                            pos1 = (float) final_registration_boneFrame.bezier[7] / 127;
                            pos2 = (float) final_registration_boneFrame.bezier[15] / 127;


                            float t = getPos(pos1, pos2, time_count_base *(float) (k - start +1));

                            float q0[4] = {
                                    final_registration_boneFrame.qw,
                                    final_registration_boneFrame.qx,
                                    final_registration_boneFrame.qy,
                                    final_registration_boneFrame.qz
                            };
                            float q1[4] = {
                                    boneFrame[i][end].qw,
                                    boneFrame[i][end].qx,
                                    boneFrame[i][end].qy,
                                    boneFrame[i][end].qz
                            };
                            struct Quaternion quaternion = SphericalLinearInterpolation(q0, q1, t);
                            boneFrame[i][k].qw = quaternion.w;
                            boneFrame[i][k].qx = quaternion.x;
                            boneFrame[i][k].qy = quaternion.y;
                            boneFrame[i][k].qz = quaternion.z;
                            //printf("%f\n", quaternion.y);
                        }
                    }
                    start = 0;
                }
            }
        }
        //ボーンの合成
        struct BoneFrame *synthesis_boneFrame = malloc(111*index.assigned*max_frame);
        int n = 0;

        for(int i = 0; i < index.assigned; i++){
            for(int j = 0; j < max_frame; j++){
                char bezier[64] = {20, 20, 0, 0, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 20, 20, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 20, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 0, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 0, 0};
                memcpy(boneFrame[i][j].bezier, bezier, 64);//ベジェのコピー

                synthesis_boneFrame[n] = boneFrame[i][j];
                n++;
            }
        }
        motionData.maxFrame.maxFrame = n;
        //memcpy(motionData.boneFrame, synthesis_boneFrame, n*111);
        free(motionData.boneFrame);
        motionData.boneFrame = synthesis_boneFrame;
        for(int i = 0; i < index.assigned; i++){
            free(boneFrame[i]);
        }
        free(boneFrame);
    }

    return motionData;
}

void jointCalculation(struct Model model, MotionData *motionData, struct BoneFrame *parent_boneFrame, struct Bone model_parent_bone, struct Index model_index, struct Index motion_index){

    printf("親ボーン:%s, ", word_decode(parent_boneFrame->name, 15, "UTF-8", "SHIFT-JIS"));
    //クォータニオンからオイラー角を算出。回転順序はYXZで、オイラー角のYとZに-1をかける必要がある。
    float qx, qy, qz, qw;
    qx = parent_boneFrame->qx;
    qy = parent_boneFrame->qy;
    qz = parent_boneFrame->qz;
    qw = parent_boneFrame->qw;

    float ox, oy, oz;
    ox = asinf(-(2*qy*qz-2*qx*qw));
    oy = -atanf((2*qx*qz+2*qy*qw)/(2*powf(qw, 2)+2*powf(qz, 2)-1));
    if(cosf(ox)==0.0f){
        oy *= -1;
        oz = 0;
    } else oz = -atanf((2*qx*qy+2*qz*qw)/(2* powf(qw, 2)+2+powf(qy, 2)-1));

    for (int child_bone_i = 0; child_bone_i < model_parent_bone.child_bone_size; child_bone_i++) {
        int child_bone_index = model_parent_bone.child_bones[child_bone_i];
        printf("子ボーン計算:%s\n", word_decode(model.bone[child_bone_index].model_name_jp.byte, 15, "UTF-8", "UTF-16"));
        //子ボーンを正とした相対座標(Relative Coordinates)を計算
        float rx = model.bone[child_bone_index].locations[0] - model_parent_bone.locations[0];
        float ry = model.bone[child_bone_index].locations[1] - model_parent_bone.locations[1];
        float rz = model.bone[child_bone_index].locations[2] - model_parent_bone.locations[2];

        //回転後の座標を計算
        float cx = 0;
        float cy = 0;
        float cz = 0;
        //z方向の回転を計算
        cx+=rx*cosf(oz);
        cy+=ry*sinf(oz);
        //y方向の回転を計算
        cx+=rx*sinf(oy);
        cz+=rz*cosf(oy);
        //x方向の回転を計算
        cy+=ry*cosf(ox);
        cz+=rz*sinf(ox);

        for(int child2_bone_i = 0; child2_bone_i < model.bone[child_bone_index].child_bone_size; child2_bone_i++){
            int index = getIndex(model_index, model.bone[model.bone[child_bone_index].child_bones[child2_bone_i]].model_name_jp.byte);
            if(index==-1)
                return;
            for(int frame_i = 0; frame_i < motionData->maxFrame.maxFrame; frame_i++){
                struct BoneFrame tmp_frame = motionData->boneFrame[frame_i];
                if(strncmp(motion_index.name[index], tmp_frame.name, 15) == 0 && tmp_frame.frame == parent_boneFrame->frame){
                    jointCalculation(model, motionData, &tmp_frame, model.bone[child_bone_index], model_index, motion_index);
                }
            }
        }
    }
}
/*
 * この関数を使用することで、関節の角度によるボーンの移動距離をモデルをベースに算出し、移動座標に付加できる。
 * また、必ずgetMotionのフレームを補完を行ってから実行すること。
*/
void modelPhysics(struct Model model, MotionData *motionData){
    char* encode_codec = "UTF-16";
    if(model.header.encode==1) encode_codec = "UTF-8";

    struct Index model_name_index = makeIndex();
    struct Index motion_bone_name_index = makeIndex();

    for(int i0 = 0; i0 < model.bone_size; i0++){
        char* model_born_name = word_decode(model.bone[i0].model_name_jp.byte, model.bone[i0].model_name_jp.byte_size, "UTF-8", encode_codec);
        for(int i1 = 0; i1 < motionData->maxFrame.maxFrame; i1++){
            char* motion_born_name = word_decode(motionData->boneFrame[i1].name, 15, "UTF-8", "SHIFT-JIS");
            if(strncmp(model_born_name, motion_born_name, 15) == 0){
                addIndex(&model_name_index, model.bone[i0].model_name_jp.byte, model.bone[i0].model_name_jp.byte_size);
                addIndex(&motion_bone_name_index, motionData->boneFrame[i1].name, 15);
                printf("%s\n", model_born_name);
                break;
            }
        }
    }
    for(int i = 0; i < motionData->maxFrame.maxFrame; i++){
        struct BoneFrame parent_boneFrame = motionData->boneFrame[i];

        //モーションのボーンと対応するモデルのボーンを取得
        int index = getnIndex(motion_bone_name_index, parent_boneFrame.name, 15);
        struct Bone model_parent_bone;
        for(int bone_i = 0; bone_i < model.bone_size+1; bone_i++){
            if(strcmp(model.bone[bone_i].model_name_jp.byte, model_name_index.name[index]) == 0){
                model_parent_bone=model.bone[bone_i];
                break;
            }
        }

        //子ボーンの座標を計算
        if(model_parent_bone.child_bone_size != 0) {
            jointCalculation(model, motionData, &parent_boneFrame, model_parent_bone, model_name_index, motion_bone_name_index);
        }
    }
}

void writeMotion(const char* output_file_path, MotionData motionData){
    //ファイルに書き込み------------------------------------------
    FILE *fpw = fopen(output_file_path, "w");
    fwrite(&motionData.header, 50, 1, fpw);//ヘッダーを書き込み
    fwrite(&motionData.maxFrame.maxFrame, 1, sizeof(int), fpw);//最大フレームを書き込み
    fwrite(motionData.boneFrame,111,motionData.maxFrame.maxFrame , fpw);//ボーンフレーム
    fclose(fpw);
}



int main(){
    MotionData motionData = getMotion("/home/shuta/デスクトップ/motion.vmd", false);
    printf("%d\n",motionData.maxFrame.maxFrame);
    printf("%s\n", word_decode(motionData.boneFrame[20000].name, 15, "UTF-8", "SHIFT-JIS"));

    struct Model model;
    getModel("/home/shuta/MikuMikuDance_v932x64/models/YYB Hatsune Miku_10th/YYB Hatsune Miku_10th_v1.02.pmx", &model);
    modelPhysics(model, &motionData);

    return 0;
}