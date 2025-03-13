#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

#include <stdio.h>
#include <stdlib.h>
#include "vmdStruct.h"
#include "../pmx/pmxStruct.h"
#include <stdbool.h>
#include "indexlib.h"
#include <math.h>
#include "Rotation.h"
#include "../pmx/modelLoader.c"



const char bezier[64] = {20, 20, 0, 0, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 20, 20, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 20, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 0, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 0, 0};

float getPos(const float pos1, const float pos2, float time){
    return (3* powf(1.0f-time, 2)*time*pos1+3.0f* (1-time)* powf(time, 2)*pos2+ powf(time, 3));
}

struct BoneFrame **pBoneFrame;
int f = 0;
MotionData getMotion(const char* path, bool frame_completion){
    //構造体を宣言

    MotionData motionData;

    FILE *fpw = fopen(path, "rb");
    fread(&motionData.header, 50, 1, fpw);
    if(strncmp(motionData.header.header, "Vocaloid Motion Data 0002", 24) != 0){
        motionData.maxFrame.maxFrame = -1;
        return motionData;
    }
    fread(&motionData.maxFrame, 4, 1, fpw);

    motionData.boneFrame = calloc(111, motionData.maxFrame.maxFrame);
    fread(&*motionData.boneFrame, 111, motionData.maxFrame.maxFrame, fpw);


    //データ補完が有効な場合
    if(frame_completion){

        //最終フレームの取得とindexの作成
        int max_frame = 0;
        struct Index index = makeIndex(1024, 1024);

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
        pBoneFrame = (struct BoneFrame**) calloc(sizeof(struct BoneFrame*), index.assigned);

        for(int i = 0; i < index.assigned; i++){
            pBoneFrame[i] = calloc(sizeof(struct BoneFrame), max_frame);
        }

        for(int i = 0; i < motionData.maxFrame.maxFrame; i++){
            struct BoneFrame temp = motionData.boneFrame[i];
            int id = getnIndex(index, temp.name, 15);
            pBoneFrame[id][temp.frame] = temp;
        }

        for(int i = 0; i < index.assigned; i++){//ボーンを一つづつループ
            int start = 0;
            int end;
            float position_increment;
            //補完に必要な一時変数
            float pos1, pos2;
            for(int j = 0; j < max_frame; j++){//最大フレーム回ループ
                if (pBoneFrame[i][j].name[0] == 0 && j != max_frame - 1){//フレームが未登録だった場合
                    if(start == 0){
                        start = j;//未登録フレームの開始地点
                    }
                } else{
                    if(start != 0){
                        end = j;//startが登録されていればendを登録
                        //フレームの補完
                        float time_count_base = (1/(float)(1+(end-start)));
                        struct BoneFrame final_registration_boneFrame = pBoneFrame[i][start - 1];
                        for(int k = start; k <= end; k++){
                            memcpy(pBoneFrame[i][k].name, index.name[i], 15);//名前の設定
                            pBoneFrame[i][k].frame = k;//フレームの設定
                            //printf("---開始:%f-----終了:%f\n", final_registration_boneFrame.qy, boneFrame[i][end].qy);

                            //座標x
                            if(final_registration_boneFrame.x != pBoneFrame[i][end].x) {//計算ショートカット
                                pos1 = (float) final_registration_boneFrame.bezier[4] / 127;
                                pos2 = (float) final_registration_boneFrame.bezier[12] / 127;
                                position_increment = pBoneFrame[i][end].x - final_registration_boneFrame.x;
                                float x = final_registration_boneFrame.x + position_increment * getPos(pos1, pos2,time_count_base *(float) (k -start +1));
                                pBoneFrame[i][k].x = x;
                            }else pBoneFrame[i][k].x = final_registration_boneFrame.x;
                            //座標y
                            if(final_registration_boneFrame.y != pBoneFrame[i][end].y) {
                                pos1 = (float) final_registration_boneFrame.bezier[5] / 127;
                                pos2 = (float) final_registration_boneFrame.bezier[13] / 127;
                                position_increment = pBoneFrame[i][end].y - final_registration_boneFrame.y;
                                float y = final_registration_boneFrame.y + position_increment * getPos(pos1, pos2,time_count_base *(float) (k -start +1));
                                pBoneFrame[i][k].y = y;
                            }else pBoneFrame[i][k].y = final_registration_boneFrame.y;
                            //座標z
                            if(final_registration_boneFrame.z != pBoneFrame[i][end].z) {
                                pos1 = (float) final_registration_boneFrame.bezier[6] / 127;
                                pos2 = (float) final_registration_boneFrame.bezier[14] / 127;
                                position_increment = pBoneFrame[i][end].z - final_registration_boneFrame.z;
                                float z = final_registration_boneFrame.z + position_increment * getPos(pos1, pos2,time_count_base *(float) (k -start +1));
                                pBoneFrame[i][k].z = z;
                            } else pBoneFrame[i][k].z = final_registration_boneFrame.z;


                            //クォータニオン
                            pos1 = (float) final_registration_boneFrame.bezier[7] / 127;
                            pos2 = (float) final_registration_boneFrame.bezier[15] / 127;


                            float t = getPos(pos1, pos2, time_count_base *(float) (k - start +1));


                            struct Quaternion q1, q2;
                            q1.x = final_registration_boneFrame.qx;
                            q1.y = final_registration_boneFrame.qy;
                            q1.z = final_registration_boneFrame.qz;
                            q1.w = final_registration_boneFrame.qw;

                            q2.x = pBoneFrame[i][end].qx;
                            q2.y = pBoneFrame[i][end].qy;
                            q2.z = pBoneFrame[i][end].qz;
                            q2.w = pBoneFrame[i][end].qw;

                            if(q2.x == 0 && q2.y == 0 && q2.z == 0 && q2.w == 0) q2.w = 1;//最終フレームにボーンフレームが何も代入されていないことがあるため、クォータニオンの合計値が0になるときがある。

                            struct Quaternion quaternion = SphericalLinearInterpolation(q1, q2, t);
                            pBoneFrame[i][k].qw = (float)quaternion.w;
                            pBoneFrame[i][k].qx = (float)quaternion.x;
                            pBoneFrame[i][k].qy = (float)quaternion.y;
                            pBoneFrame[i][k].qz = (float)quaternion.z;
                            //printf("%f\n", quaternion.y);
                        }
                    }
                    start = 0;
                }
            }
        }
        //ボーンの合成
        struct BoneFrame *synthesis_boneFrame = calloc(111, index.assigned*max_frame);
        int n = 0;

        for(int i = 0; i < index.assigned; i++){
            for(int j = 0; j < max_frame; j++){
                memcpy(pBoneFrame[i][j].bezier, bezier, 64);//ベジェのコピー
                synthesis_boneFrame[n] = pBoneFrame[i][j];
                n++;
            }
        }
        motionData.maxFrame.maxFrame = n;
        //memcpy(motionData.boneFrame, synthesis_boneFrame, n*111);
        free(motionData.boneFrame);
        motionData.boneFrame = NULL;
        motionData.boneFrame = synthesis_boneFrame;
        /*
        for(int i = 0; i < index.assigned; i++){
            free(boneFrame[i]);
            boneFrame[i] = NULL;
        }
        free(boneFrame);
        boneFrame = NULL;
         */
        destroy_index(&index);
    }

    return motionData;
}
void jointCalculationEncoder(int frame, struct BoneFrame* current_frames, struct Index *model_index, struct EncodeBoneFrame *encodeBoneFrame, struct Model model, const int* parentBoneDataPtrArray){
    for(int bone_i = 0; bone_i < model_index->assigned; bone_i++){
        struct BoneFrame *current_bone_frame = &current_frames[bone_i];
        struct Bone current_edited_bone = model.bone[bone_i];
        //クォータニオンからオイラー角を算出。回転順序はYXZで、オイラー角のYとZに-1をかける必要がある。
        if(current_bone_frame->name[0] == 0){
            //ボーン名前を代入
            char* name = word_decode(model_index->name[bone_i], 15, "SHIFT-JIS", model.header.encode==1 ? "UTF-8" : "UTF-16");
            memcpy(current_bone_frame->name, name, 15);
            free(name);
            name = NULL;
            //フレームを代入
            current_bone_frame->frame = frame;

            //x,y,zとqx,qy,qzを0に、qwを1に初期化
            float *locations = (float*)&current_bone_frame->x; //#pragma pack(1)でメモリが詰められているため有効に動作する。
            for(int i = 0; i < 7; i++) {
                locations[i] = (i == 6) ? 1.0f : 0.0f;
            }
        }

        //親ボーンらが移動した合計を計算
        if(parentBoneDataPtrArray[bone_i] != -1){

            int parent_bone_index =  parentBoneDataPtrArray[bone_i];
            struct Bone parent_bone = model.bone[parent_bone_index];
            struct BoneFrame parent_boneFrame = current_frames[parent_bone_index];
            struct Quaternion quaternion_p;
            quaternion_p.x = parent_boneFrame.qx;
            quaternion_p.y = parent_boneFrame.qy;
            quaternion_p.z = parent_boneFrame.qz;
            quaternion_p.w = parent_boneFrame.qw;
            struct Quaternion quaternion_c;
            quaternion_c.x = current_bone_frame->qx;
            quaternion_c.y = current_bone_frame->qy;
            quaternion_c.z = current_bone_frame->qz;
            quaternion_c.w = current_bone_frame->qw;

            //子ボーンを正とした相対座標(Relative Coordinates)を計算
            float rx = current_edited_bone.locations[0] - parent_bone.locations[0];
            float ry = current_edited_bone.locations[1] - parent_bone.locations[1];
            float rz = current_edited_bone.locations[2] - parent_bone.locations[2];

            long double combination_rx=0, combination_ry=0, combination_rz=0;
            long double *r1, *r2, *r3;
            struct Matrix matrix = QuaternionToMatrix(quaternion_p);

            r1 = matrix.value[0];
            r2 = matrix.value[1];
            r3 = matrix.value[2];

            combination_rx = r1[0] * rx + r1[1] * ry + r1[2] * rz;
            combination_ry = r2[0] * rx + r2[1] * ry + r2[2] * rz;
            combination_rz = r3[0] * rx + r3[1] * ry + r3[2] * rz;

            //絶対座標の計算
            long double ax = combination_rx + current_bone_frame->x + parent_boneFrame.x;
            long double ay = combination_ry + current_bone_frame->y + parent_boneFrame.y;
            long double az = combination_rz + current_bone_frame->z + parent_boneFrame.z;


            current_bone_frame->x = (float)ax;
            current_bone_frame->y = (float)ay;
            current_bone_frame->z = (float)az;

            //printf("%s\n", word_decode(current_bone_frame->name, 15, "UTF-8", "SHIFT-JIS"));

            struct Quaternion q = qmul(quaternion_c, quaternion_p);
            current_bone_frame->qx = (float)q.x;
            current_bone_frame->qy = (float)q.y;
            current_bone_frame->qz = (float)q.z;
            current_bone_frame->qw = (float)q.w;

            //エンコードボーンフレーム構造体に代入
            encodeBoneFrame[bone_i].x = (float)ax;
            encodeBoneFrame[bone_i].y = (float)ay;
            encodeBoneFrame[bone_i].z = (float)az;

            encodeBoneFrame[bone_i].qx = (float)q.x;
            encodeBoneFrame[bone_i].qy = (float)q.y;
            encodeBoneFrame[bone_i].qz = (float)q.z;
            encodeBoneFrame[bone_i].qw = (float)q.w;


            if(frame==0) {
                char *tempstr = malloc(5112);
                sprintf(tempstr, "%s  %4Lf,%4Lf,%4Lf,%4Lf 派生:%s\n",
                        word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size,
                                    "UTF-8", "UTF-16"), quaternion_c.x, quaternion_c.y,
                        quaternion_c.z, quaternion_c.w,
                        word_decode(parent_bone.model_name_jp.byte, parent_bone.model_name_jp.byte_size, "UTF-8",
                                    "UTF-16"));
                printf(tempstr);
                free(tempstr);
            }


        }else{
            current_bone_frame->x += current_edited_bone.locations[0];
            current_bone_frame->y += current_edited_bone.locations[1];
            current_bone_frame->z += current_edited_bone.locations[2];

            //エンコードボーンフレーム構造体に代入
            encodeBoneFrame[bone_i].x = current_bone_frame->x;
            encodeBoneFrame[bone_i].y = current_bone_frame->y;
            encodeBoneFrame[bone_i].z = current_bone_frame->z;

            encodeBoneFrame[bone_i].qx = current_bone_frame->qx;
            encodeBoneFrame[bone_i].qy = current_bone_frame->qy;
            encodeBoneFrame[bone_i].qz = current_bone_frame->qz;
            encodeBoneFrame[bone_i].qw = current_bone_frame->qw;
            /*
            char* tempstr = malloc(5112);
            sprintf(tempstr, "%s %4f,%4f,%4f\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), current_edited_bone.locations[0], current_edited_bone.locations[1], current_edited_bone.locations[2]);
            free(tempstr);
             */
        }
    }
}

/*
 * ボーンの親子関係を生成します。
 */
void makeParentChildLink(int* dist, struct Model model){
    for(int i = 0; i < model.bone_size; i++) dist[i] = -1;
    for(int bone_i = 0; bone_i < model.bone_size; bone_i++) {
        struct Bone current_edited_bone = model.bone[bone_i];
        //printf("親ボーン:%s, ", word_decode(parent_boneFrame->name, 15, "UTF-8", "SHIFT-JIS"));
        //current_bone_frame->y-=0.1f; //TODO 0.1マイナスする（MMDがy軸に0.1ずれてる）
        //子ボーンに加算する値を代入
        for (int child_bone_i = 0; child_bone_i < current_edited_bone.child_bone_size; child_bone_i++) {
            int child_bone_index = current_edited_bone.child_bones[child_bone_i];
            dist[child_bone_index] = bone_i;
        }
    }
}
/*
 * この関数を使用することで、関節の角度によるボーンの移動距離をモデルをベースに算出し、移動座標に付加できる。
 * また、必ずgetMotionのフレームを補完を行ってから実行すること。
*/
EncodeMotionData modelPhysics(struct Model model, MotionData *motionData){
    printf("インデックス作成\n");
    char* encode_codec = model.header.encode==1 ? "UTF-8" : "UTF-16";

    //出力用の構造体
    EncodeMotionData encodeMotionData;
    encodeMotionData.nameIndexer_size = 0;
    encodeMotionData.encodeBoneFrame_size = 0;

    struct Index model_name_index = makeIndex(1024, 1024);
    struct Index motion_bone_name_index = makeIndex(1024, 1024);
    struct Index model_name_index_uft8 = makeIndex(1024, 1024); //モデルのインデックスがutf8で作られたバージョン
    int max_frame = 0; //モーションが何フレームの最大値
    struct BoneFrame** bone_frames = calloc(sizeof(struct BoneFrame), 16384);//[フレーム番号][ボーンインデックス]
    for(int i = 0; i < 16384; i++) bone_frames[i] = calloc((model.bone_size), sizeof(struct BoneFrame));

    //インデックスを作成
    for(int i0 = 0; i0 < model.bone_size; i0++){
        char* model_born_name = word_decode(model.bone[i0].model_name_jp.byte, model.bone[i0].model_name_jp.byte_size, "UTF-8", encode_codec);
        int n = addIndex(&model_name_index, model.bone[i0].model_name_jp.byte, model.bone[i0].model_name_jp.byte_size);
        addIndex(&model_name_index_uft8, model_born_name, (int)strlen(model_born_name));
        free(model_born_name);
        model_born_name = NULL;

        //出力用のインデックスも作成
        char* temp = word_decode(model.bone[i0].model_name_jp.byte, model.bone[i0].model_name_jp.byte_size, "SHIFT-JIS", model.header.encode==0 ? "UTF-16":"UTF-8");
        memcpy(encodeMotionData.nameIndexer[encodeMotionData.nameIndexer_size].name, temp, 15);
        encodeMotionData.nameIndexer[encodeMotionData.nameIndexer_size].name_byte = 15;
        encodeMotionData.nameIndexer[encodeMotionData.nameIndexer_size++].index = n;

    }
    motion_bone_name_index.assigned = model_name_index.assigned;
    for(int i0 = 0; i0 < motionData->maxFrame.maxFrame; i0++){
        struct BoneFrame* boneFrame = &motionData->boneFrame[i0];
        char* temp = word_decode(boneFrame->name, 15, "UTF-8", "SHIFT-JIS");
        int index = getIndex(model_name_index_uft8, temp);
        free(temp);
        temp = NULL;
        if(index!=-1) {
            if (motion_bone_name_index.name[index][0] == 0)
                memcpy(motion_bone_name_index.name[index], boneFrame->name, 15);
            bone_frames[boneFrame->frame][index] = *boneFrame;

            if(boneFrame->frame > max_frame) max_frame=boneFrame->frame;
        }
    }

    int parentBoneDataArray[model_name_index.assigned];//モデルと同じインデックスに、親と子の関係を構築
    for(int i = 0; i < model_name_index.assigned; i++) parentBoneDataArray[i] = -1;

    for(int bone_i = 0; bone_i < model_name_index.assigned; bone_i++) {
        struct Bone current_edited_bone = model.bone[bone_i];
        //printf("親ボーン:%s, ", word_decode(parent_boneFrame->name, 15, "UTF-8", "SHIFT-JIS"));
        //current_bone_frame->y-=0.1f; //TODO 0.1マイナスする（MMDがy軸に0.1ずれてる）
        //子ボーンに加算する値を代入
        for (int child_bone_i = 0; child_bone_i < current_edited_bone.child_bone_size; child_bone_i++) {
            int child_bone_index = current_edited_bone.child_bones[child_bone_i];
            parentBoneDataArray[child_bone_index] = bone_i;
        }
    }
    printf("計算開始\n");
    encodeMotionData.encodeBoneFrame = calloc(sizeof(struct EncodeBoneFrame), max_frame);
    encodeMotionData.encodeBoneFrame_size = max_frame+1;
    for(int i = 0; i <= max_frame; i++){
        encodeMotionData.encodeBoneFrame[i] = calloc(sizeof(struct EncodeBoneFrame), model.bone_size);
        jointCalculationEncoder(i, bone_frames[i], &model_name_index, encodeMotionData.encodeBoneFrame[i], model,
                                parentBoneDataArray);
    }
    printf("完了\n");
    //free
    for(int i = 0; i < 16384; i++) {
        free(bone_frames[i]);
        bone_frames[i] = NULL;
    }
    free(bone_frames);
    bone_frames = NULL;
    destroy_index(&model_name_index);
    destroy_index(&motion_bone_name_index);
    destroy_index(&model_name_index_uft8);

    return encodeMotionData;
}

//エンコードされたencodeBoneFrameで欠落しているボーンを復元する
void jointCompletion(struct EncodeBoneFrame* current_frames, struct Index *model_index, struct Model model, const int* parentBoneDataPtrArray){
    for(int bone_i = 0; bone_i < model_index->assigned; bone_i++){
        struct EncodeBoneFrame *current_bone_frame = &current_frames[bone_i];
        struct Bone current_edited_bone = model.bone[bone_i];
        //クォータニオンからオイラー角を算出。回転順序はYXZで、オイラー角のYとZに-1をかける必要がある。
        if(sqrtl(powl(current_bone_frame->qx, 2) + powl(current_bone_frame->qy, 2) + powl(current_bone_frame->qz, 2) + powl(current_bone_frame->qw, 2)) < 0.0){
            //x,y,zとqx,qy,qzを0に、qwを1に初期化
            float *locations = (float*)&current_bone_frame->x; //#pragma pack(1)でメモリが詰められているため有効に動作する。
            for(int i = 0; i < 7; i++) {
                locations[i] = (i == 6) ? 1.0f : 0.0f;
            }
            printf("補完:%s\n", word_decode(current_edited_bone.model_name_jp.byte,current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"));


            //親ボーンらが移動した合計を計算
            if(parentBoneDataPtrArray[bone_i] != -1){

                int parent_bone_index =  parentBoneDataPtrArray[bone_i];
                struct Bone parent_bone = model.bone[parent_bone_index];
                struct EncodeBoneFrame parent_boneFrame = current_frames[parent_bone_index];
                struct Quaternion quaternion_p;
                quaternion_p.x = parent_boneFrame.qx;
                quaternion_p.y = parent_boneFrame.qy;
                quaternion_p.z = parent_boneFrame.qz;
                quaternion_p.w = parent_boneFrame.qw;
                quaternion_p = quaternionNormalization(quaternion_p);
                struct Quaternion quaternion_c;
                quaternion_c.x = current_bone_frame->qx;
                quaternion_c.y = current_bone_frame->qy;
                quaternion_c.z = current_bone_frame->qz;
                quaternion_c.w = current_bone_frame->qw;
                quaternion_c = quaternionNormalization(quaternion_c);

                //子ボーンを正とした相対座標(Relative Coordinates)を計算
                float rx = current_edited_bone.locations[0] - parent_bone.locations[0];
                float ry = current_edited_bone.locations[1] - parent_bone.locations[1];
                float rz = current_edited_bone.locations[2] - parent_bone.locations[2];

                long double combination_rx=0, combination_ry=0, combination_rz=0;
                long double *r1, *r2, *r3;
                struct Matrix matrix = QuaternionToMatrix(quaternion_p);

                r1 = matrix.value[0];
                r2 = matrix.value[1];
                r3 = matrix.value[2];

                combination_rx = r1[0] * rx + r1[1] * ry + r1[2] * rz;
                combination_ry = r2[0] * rx + r2[1] * ry + r2[2] * rz;
                combination_rz = r3[0] * rx + r3[1] * ry + r3[2] * rz;

                //絶対座標の計算
                long double ax = combination_rx + current_bone_frame->x + parent_boneFrame.x;
                long double ay = combination_ry + current_bone_frame->y + parent_boneFrame.y;
                long double az = combination_rz + current_bone_frame->z + parent_boneFrame.z;


                current_bone_frame->x = (float)ax;
                current_bone_frame->y = (float)ay;
                current_bone_frame->z = (float)az;

                //printf("%s\n", word_decode(current_bone_frame->name, 15, "UTF-8", "SHIFT-JIS"));

                struct Quaternion q = qmul(quaternion_p, quaternion_c);
                current_bone_frame->qx = (float)q.x;
                current_bone_frame->qy = (float)q.y;
                current_bone_frame->qz = (float)q.z;
                current_bone_frame->qw = (float)q.w;
                /*
                char* tempstr = malloc(5112);
                sprintf(tempstr, "%s  %4f,%4f,%4f 派生:%s\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), current_bone_frame->x, current_bone_frame->y, current_bone_frame->z, word_decode(parent_bone.model_name_jp.byte, parent_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"));
                free(tempstr);
                 */

            }else{
                //エンコードボーンフレーム構造体に代入
                current_bone_frame->x += current_edited_bone.locations[0];
                current_bone_frame->y += current_edited_bone.locations[1];
                current_bone_frame->z += current_edited_bone.locations[2];
                /*
                char* tempstr = malloc(5112);
                sprintf(tempstr, "%s %4f,%4f,%4f\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), current_edited_bone.locations[0], current_edited_bone.locations[1], current_edited_bone.locations[2]);
                free(tempstr);
                 */
            }
        }
    }
}
void jointCalculationDecoder(int frame, struct EncodeBoneFrame* current_frames, struct Index *motion_index, struct BoneFrame* decodeBoneFrame, struct Model model, const int* parentBoneDataPtrArray){
    for(int bone_i = 0; bone_i < motion_index->assigned; bone_i++){
        struct EncodeBoneFrame *current_bone_frame = &current_frames[bone_i];
        struct Bone current_edited_bone = model.bone[bone_i];

        //親ボーンらが移動した合計を計算
        if(parentBoneDataPtrArray[bone_i] != -1){

            int parent_bone_index =  parentBoneDataPtrArray[bone_i];
            struct Bone parent_bone = model.bone[parent_bone_index];
            struct EncodeBoneFrame *parent_boneFrame = &current_frames[parent_bone_index];
            struct Quaternion quaternion_p;
            quaternion_p.x = parent_boneFrame->qx;
            quaternion_p.y = parent_boneFrame->qy;
            quaternion_p.z = parent_boneFrame->qz;
            quaternion_p.w = parent_boneFrame->qw;
            struct Quaternion quaternion_c;
            quaternion_c.x = current_bone_frame->qx;
            quaternion_c.y = current_bone_frame->qy;
            quaternion_c.z = current_bone_frame->qz;
            quaternion_c.w = current_bone_frame->qw;

            //子ボーンを正とした相対座標(Relative Coordinates)を計算
            float rx = current_edited_bone.locations[0] - parent_bone.locations[0];
            float ry = current_edited_bone.locations[1] - parent_bone.locations[1];
            float rz = current_edited_bone.locations[2] - parent_bone.locations[2];

            long double combination_rx=0, combination_ry=0, combination_rz=0;
            long double *r1, *r2, *r3;
            struct Matrix matrix = QuaternionToMatrix(quaternion_p);

            r1 = matrix.value[0];
            r2 = matrix.value[1];
            r3 = matrix.value[2];

            combination_rx = r1[0] * rx + r1[1] * ry + r1[2] * rz;
            combination_ry = r2[0] * rx + r2[1] * ry + r2[2] * rz;
            combination_rz = r3[0] * rx + r3[1] * ry + r3[2] * rz;

            //絶対座標の計算
            long double ax = current_bone_frame->x - (parent_boneFrame->x + combination_rx);
            long double ay = current_bone_frame->y - (parent_boneFrame->y + combination_ry);
            long double az = current_bone_frame->z - (parent_boneFrame->z + combination_rz);

            //printf("%s\n", word_decode(current_bone_frame->name, 15, "UTF-8", "SHIFT-JIS"));
            struct Quaternion q = inverse(qmul((quaternion_p), inverse(quaternion_c)));
            if(frame==140) {
                char *tempstr = malloc(5112);
                sprintf(tempstr, "%s  %4Lf,%4Lf,%4Lf,%4Lf 派生:%s\n",
                        word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size,
                                    "UTF-8", "UTF-16"), q.x, q.y,
                        q.z, q.w,
                        word_decode(parent_bone.model_name_jp.byte, parent_bone.model_name_jp.byte_size, "UTF-8",
                                    "UTF-16"));

                printf(tempstr);
                free(tempstr);
                if(strcmp(word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size,
                               "UTF-8", "UTF-16"), "上半身") == 0){
                    printf("equal\n");
                }
            }

            //エンコードボーンフレーム構造体に代入
            decodeBoneFrame[bone_i].x = (float)ax;
            decodeBoneFrame[bone_i].y = (float)ay;
            decodeBoneFrame[bone_i].z = (float)az;

            decodeBoneFrame[bone_i].qx = (float)q.x;
            decodeBoneFrame[bone_i].qy = (float)q.y;
            decodeBoneFrame[bone_i].qz = (float)q.z;
            decodeBoneFrame[bone_i].qw = (float)q.w;

        }else{
            //エンコードボーンフレーム構造体に代入
            decodeBoneFrame[bone_i].x = (float)current_bone_frame->x - current_edited_bone.locations[0];
            decodeBoneFrame[bone_i].y = (float)current_bone_frame->y - current_edited_bone.locations[1];
            decodeBoneFrame[bone_i].z = (float)current_bone_frame->z - current_edited_bone.locations[2];

            decodeBoneFrame[bone_i].qx = (float)current_bone_frame->qx;
            decodeBoneFrame[bone_i].qy = (float)current_bone_frame->qy;
            decodeBoneFrame[bone_i].qz = (float)current_bone_frame->qz;
            decodeBoneFrame[bone_i].qw = (float)current_bone_frame->qw;
            /*
            char* tempstr = malloc(5112);
            sprintf(tempstr, "%s %4f,%4f,%4f\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), current_edited_bone.locations[0], current_edited_bone.locations[1], current_edited_bone.locations[2]);
            free(tempstr);
             */
        }

        memcpy(decodeBoneFrame[bone_i].name, motion_index->name[bone_i], 15);
        memcpy(decodeBoneFrame[bone_i].bezier, bezier, 64);
        decodeBoneFrame[bone_i].frame = frame;
    }
}

//need_boneはshift-jis
MotionData decode(EncodeMotionData encodeMotionData, struct Model model, struct Index need_bone){
    //ボーンフレームを構築
    int max_frame = encodeMotionData.encodeBoneFrame_size;
    struct BoneFrame **decodeBoneFrame2D = calloc(sizeof(struct BoneFrame), encodeMotionData.encodeBoneFrame_size); //[フレーム][ボーン数]
    struct EncodeBoneFrame **encodeBoneFrame2D = calloc(sizeof(struct EncodeBoneFrame), max_frame); //[フレーム][ボーン]   渡されたエンコードモーションの順番を整列させて代入
    for (int i0 = 0; i0 < encodeMotionData.encodeBoneFrame_size; ++i0) {
        decodeBoneFrame2D[i0] = calloc(sizeof(struct BoneFrame), model.bone_size);
        encodeBoneFrame2D[i0] = calloc(sizeof(struct EncodeBoneFrame), model.bone_size);
    }

    struct Index model_name_index_utf8 = makeIndex(model.bone_size, 1024);
    struct Index motion_name_index = makeIndex(model.bone_size, 15);

    for (int i0 = 0; i0 < model.bone_size; ++i0) {
        //model_name_index_utf8のインデックスを作成
        if(model.header.encode == 0){
            char *temp = word_decode(model.bone[i0].model_name_jp.byte, model.bone[i0].model_name_jp.byte_size, "UTF-8", "UTF-16");
            addIndex(&model_name_index_utf8, temp, model.bone[i0].model_name_jp.byte_size);
            free(temp);
            temp = NULL;
        } else addIndex(&model_name_index_utf8, model.bone[i0].model_name_jp.byte, model.bone[i0].model_name_jp.byte_size);

        //motion_name_indexの作成
        char *temp = word_decode(model.bone[i0].model_name_jp.byte, 15, "SHIFT-JIS", model.header.encode==0 ? "UTF-16" : "UTF-8");
        addIndex(&motion_name_index, temp, 15);

        //ボーンフレーム2Dにフレームを代入
        for (int i1 = 0; i1 < encodeMotionData.nameIndexer_size; ++i1) {
            if(strncmp(temp, encodeMotionData.nameIndexer[i1].name, encodeMotionData.nameIndexer[i1].name_byte) == 0){
                for (int i2 = 0; i2 < max_frame; ++i2) {
                    encodeBoneFrame2D[i2][i0] = encodeMotionData.encodeBoneFrame[i2][encodeMotionData.nameIndexer[i1].index];
                }
                break;
            }
        }
        free(temp);
        temp = NULL;
    }

    int link[model_name_index_utf8.assigned];
    makeParentChildLink(link, model);

    for (int i0 = 0; i0 < max_frame; ++i0) {
        jointCompletion(encodeBoneFrame2D[i0], &model_name_index_utf8, model, link);
        jointCalculationDecoder(i0, encodeBoneFrame2D[i0], &motion_name_index, decodeBoneFrame2D[i0], model, link);
    }

    //モーションデータの作成
    MotionData motionData;
    motionData.boneFrame = calloc(sizeof(struct BoneFrame), max_frame*encodeMotionData.nameIndexer_size);
    int assigned = 0;
    for (int i0 = 0; i0 < motion_name_index.assigned; ++i0) {
        for (int i1 = 0; i1 < need_bone.assigned; ++i1) {
            char* temp1 = word_decode(motion_name_index.name[i0], 15, "UTF-8", "SHIFT-JIS");
            char* temp2 = word_decode(need_bone.name[i1], 15, "UTF-8", "SHIFT-JIS");
            if(strncmp(temp1, temp2, 15) == 0){
                for (int i2 = 0; i2 < max_frame; ++i2) {
                    motionData.boneFrame[assigned] = decodeBoneFrame2D[i2][i0];
                    if(strncmp(temp1, "上半身", 15) == 0){
                    //    memcpy(motionData.boneFrame[assigned].name, need_bone.name[i1], 15);
                    }
                    assigned++;
                }
                break;
            }
            free(temp1);
            temp1 = NULL;
            free(temp2);
            temp2 = NULL;
        }
    }
    motionData.maxFrame.maxFrame = assigned;

    free(encodeBoneFrame2D);
    encodeBoneFrame2D = NULL;
    free(decodeBoneFrame2D);
    decodeBoneFrame2D = NULL;

    return motionData;
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
    MotionData motionData = getMotion("/home/shuta/デスクトップ/motion.vmd", true);
    printf("%d\n", motionData.maxFrame.maxFrame);
    writeMotion("/home/shuta/デスクトップ/motion1.vmd", motionData);
    //printf("%s\n", word_decode(motionData.boneFrame[10000].name, 15, "UTF-8", "SHIFT-JIS"));

    struct Index bone_index = makeIndex(400, 15);
    for (int i = 0; i < motionData.maxFrame.maxFrame; i++) {
        if(getnIndex(bone_index, motionData.boneFrame[i].name, 15) == -1)
            addIndex(&bone_index, motionData.boneFrame[i].name, 15);
    }

    struct Model model;
    getModel("/home/shuta/MikuMikuDance_v932x64/models/YYB Hatsune Miku_10th/YYB Hatsune Miku_10th_v1.02.pmx",
             &model);
    EncodeMotionData encodeMotionData = modelPhysics(model, &motionData);
    MotionData decodeMotionData = decode(encodeMotionData, model, bone_index);
    memcpy(decodeMotionData.header.header, motionData.header.header, 30);
    memcpy(decodeMotionData.header.modelName, motionData.header.modelName, 20);
    writeMotion("/home/shuta/デスクトップ/motion2.vmd", decodeMotionData);
    for (int i = 0; i < encodeMotionData.encodeBoneFrame_size; i++) {
        free(encodeMotionData.encodeBoneFrame[i]);
    }
    free(encodeMotionData.encodeBoneFrame);
    for (int i = 0; i < model.bone_size; i++) free(model.bone[i].child_bones);
    free(model.bone);
    free(model.texture);
    free(model.surface);
    free(model.topData);
    free(model.material);

    free(motionData.boneFrame);


    return 0;
}

int main1(){
    struct Quaternion q1 = {0};
    q1.w = 0.87214514;
    q1.x = -0.24410777;
    q1.y = -0.29476663;
    q1.z = 0.30477349;
    struct Quaternion q2 = {0};
    q2.w = -0.82200395;
    q2.x = 0.30170365;
    q2.y = -0.47702741;
    q2.z = 0.07569188;
    struct Quaternion q3 = qmul(q1, q2);

    struct Quaternion q4 = q1;
    for(int i = 0; i < 4; i++){
        ((double *)&q4)[i] *= -1;
    }
    q4.w *= -1;
    struct Quaternion q5 = qmul(q4, q3);

    return 0;

}