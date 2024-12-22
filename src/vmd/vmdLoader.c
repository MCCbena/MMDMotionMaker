#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

#include <stdio.h>
#include "vmdStruct.h"
#include <stdbool.h>
#include "indexlib.h"
#include <math.h>
#include <stdlib.h>
#include "../pmx/modelLoader.c"
#include <pthread.h>

#define deg_to_rad(deg) ((deg)*M_PI/180)

const char bezier[64] = {20, 20, 0, 0, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 20, 20, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 20, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 0, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 0, 0};
unsigned long ex_counter = 0;

float getPos(const float pos1, const float pos2, float time){
    return (3* powf(1.0f-time, 2)*time*pos1+3.0f* (1-time)* powf(time, 2)*pos2+ powf(time, 3));
}

struct Quaternion{
    float w;
    float x;
    float y;
    float z;
};

struct Euler{
    float x;
    float y;
    float z;
};

struct Matrix{
    float value[3][3];
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

//TODO ジンバルロックの対応ができていない
//回転順序はYXZ
struct Euler QuaternionToEuler(float qw, float qx, float qy, float qz){
    struct Euler euler;
    euler.x = asinf(-(2*qy*qz-2*qx*qw))*180/M_PI;
    if(cosf(euler.x)==0.0f){
        euler.y = atanf(-(2*qx*qz+2*qy*qw)/(2*qw*qw+2*qx*qx-1))*180/M_PI;
        euler.z = 0;
    } else{
        euler.z = atanf((2*qx*qy+2*qz*qw)/(2*qw*qw+2*qy*qy-1))*180/M_PI;
        euler.y = atanf((2*qx*qz+2*qy*qw)/(2*qw*qw+2*qz*qz-1))*180/M_PI;
    }

    return euler;
}

struct Matrix QuaternionToMatrix(float qw, float qx, float qy, float qz){
    struct Matrix matrix;

    matrix.value[0][0] = 2*powf(qw,2) + 2* powf(qx,2)-1;
    matrix.value[0][1] = 2*qx*qy - 2*qz*qw;
    matrix.value[0][2] = 2*qx*qz + 2*qy*qw;

    matrix.value[1][0] = 2*qx*qy + 2*qz*qw;
    matrix.value[1][1] = 2*powf(qw,2) + 2*powf(qy, 2)-1;
    matrix.value[1][2] = 2*qy*qz - 2*qx*qw;

    matrix.value[2][0] = 2*qx*qz - 2*qy*qw;
    matrix.value[2][1] = 2*qy*qz + 2*qx*qw;
    matrix.value[2][2] = 2*powf(qw,2) + 2*powf(qz, 2)-1;

    return matrix;
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
                        for(int k = start; k <= end; k++){
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

struct JCArgs{
    struct Model model;
    MotionData *motionData;
    struct BoneFrame* parent_boneFrame;
    struct Bone model_parent_bone;
    struct Index *model_name_index;
    struct Index *motion_name_index;
};
struct PtrArray{
    void** addr;
    int size;
    int access_count;
};
struct TrigFunctionData{
    int bone_index; //この三角関数を計算するために使ったボーンのインデックス

    struct Matrix matrix;

    float x; //座標軸に対して加算する値x
    float y; //座標軸に対して加算する値y
    float z; //座標軸に対して加算する値z

    int access_count;
};
int count = 0; //現在、どれだけのスレッドが同時に動いているか代入する
void jointCalculation(int frame, struct BoneFrame* current_frames, struct Index *model_index, struct Index *motion_index, struct Model model){
    struct PtrArray trigsPtrArray;//モデルと同じインデックスに、親ボーンの回転角が代入された三角関数構造体が代入される
    trigsPtrArray.addr = malloc(sizeof(long) * model_index->assigned);
    for(int i = 0; i < model_index->assigned; i++) trigsPtrArray.addr[i] = NULL;

    trigsPtrArray.size = model_index->assigned;
    for(int bone_i = 0; bone_i < model_index->assigned; bone_i++){
        struct BoneFrame *current_bone_frame = &current_frames[bone_i];
        struct Bone current_edited_bone = model.bone[bone_i];
        //printf("親ボーン:%s, ", word_decode(parent_boneFrame->name, 15, "UTF-8", "SHIFT-JIS"));
        current_bone_frame->y-=0.1f; //TODO 0.1マイナスする（MMDがy軸に0.1ずれてる）
        //子ボーンに加算する値を代入
        for (int child_bone_i = 0; child_bone_i < current_edited_bone.child_bone_size; child_bone_i++) {
            struct PtrArray *temp;
            int child_bone_index = current_edited_bone.child_bones[child_bone_i];
            if (trigsPtrArray.addr[child_bone_index] == NULL) {
                temp = malloc((sizeof(struct PtrArray)));
                temp->addr = malloc(model.bone_size);
                temp->size = 0;
                trigsPtrArray.addr[child_bone_index] = temp;
            }
        }
        //クォータニオンからオイラー角を算出。回転順序はYXZで、オイラー角のYとZに-1をかける必要がある。
        struct TrigFunctionData *trigFunctionData = malloc(sizeof(struct TrigFunctionData));
        if(current_bone_frame->name[0] != 0) {
            float qx, qy, qz, qw;
            qx = current_bone_frame->qx;
            qy = current_bone_frame->qy*-1;
            qz = current_bone_frame->qz*-1;
            qw = current_bone_frame->qw;

            printf("sum %f\n", qw*qw+qx*qx+qy*qy+qz*qz);
            struct Matrix matrix_convert = QuaternionToMatrix(qw, qx, qy, qz);
            struct Matrix matrix;
            qy *= -1;
            qz *= -1;

            //移動座標のオフセットを代入
            trigFunctionData->x = current_bone_frame->x;
            trigFunctionData->y = current_bone_frame->y;
            trigFunctionData->z = current_bone_frame->z;
            trigFunctionData->bone_index = bone_i;
            trigFunctionData->access_count=0;

            if(trigsPtrArray.addr[bone_i] != NULL){//このボーンの親がNULLでなければボーンに親ボーンの角度を加算
                struct TrigFunctionData* temp = (struct TrigFunctionData*) trigsPtrArray.addr[bone_i];
                //内積を計算して回転行列に回転を合成
                for (int i0 = 0; i0 < 3; i0++){
                    for(int i1 = 0; i1 < 3; i1++){
                        for(int i2 = 0; i2 < 3; i2++){
                            matrix.value[i0][i1] += temp->matrix.value[i0][i2]*matrix_convert.value[i2][i1];
                        }
                    }
                }
            }else matrix = matrix_convert;
            trigFunctionData->matrix = matrix;
        } else{
            //ボーン名前を代入
            char* name = word_decode(model_index->name[bone_i], 15, "SHIFT-JIS", model.header.encode==1 ? "UTF-8" : "UTF-16");
            memcpy(current_bone_frame->name, name, 15);
            free(name);
            //フレームを代入
            current_bone_frame->frame = frame;

            //x,y,zとqx,qy,qzを0に、qwを1に初期化
            float *locations = (float*)&current_bone_frame->x; //#pragma pack(1)でメモリが詰められているため有効に動作する。
            for(int i = 0; i < 7; i++) {
                locations[i] = (i == 6) ? 1.0f : 0.0f;
            }

            trigFunctionData->bone_index = bone_i;
            trigFunctionData->access_count=0; //アクセスカウントを0で初期化
            struct Matrix matrix = {0};
            if(trigsPtrArray.addr[bone_i] != NULL){//このボーンの親がNULLでなければボーンに親ボーンの角度を加算
                struct TrigFunctionData* temp = (struct TrigFunctionData*) trigsPtrArray.addr[bone_i];

                matrix = temp->matrix;
            }else{
                matrix.value[0][0] = 1;
                matrix.value[1][1] = 1;
                matrix.value[2][2] = 1;
            }
            /*回転行列の初期値を代入してtirgFunctionDataに代入*/
            trigFunctionData->matrix = matrix;

            trigFunctionData->x = 0;
            trigFunctionData->y = 0;
            trigFunctionData->z = 0;
        }
        count++;
        //子ボーンに加算する値を代入
        for (int child_bone_i = 0; child_bone_i < current_edited_bone.child_bone_size; child_bone_i++) {
            int child_bone_index = current_edited_bone.child_bones[child_bone_i];

            trigsPtrArray.addr[child_bone_index] = trigFunctionData;
            trigFunctionData->access_count++;
        }

        if(trigFunctionData->access_count == 0){
            free(trigFunctionData);
            count--;
        }


        //親ボーンらが移動した合計を計算
        if(trigsPtrArray.addr[bone_i] != NULL){
            //ボーンの絶対座標を取得（Absolute coordinates）
            float ax = current_edited_bone.locations[0];
            float ay = current_edited_bone.locations[1];
            float az = current_edited_bone.locations[2];

            struct TrigFunctionData *trigFunctionData_temp = (struct TrigFunctionData*) trigsPtrArray.addr[bone_i];
            struct Bone parent_bone = model.bone[trigFunctionData_temp->bone_index];
            struct BoneFrame parent_boneFrame = current_frames[trigFunctionData_temp->bone_index];

            //子ボーンを正とした相対座標(Relative Coordinates)を計算
            float rx = current_edited_bone.locations[0] - parent_bone.locations[0];
            float ry = current_edited_bone.locations[1] - parent_bone.locations[1];
            float rz = current_edited_bone.locations[2] - parent_bone.locations[2];

            float trig_rx=0, trig_ry=0, trig_rz=0;
            float *r1, *r2, *r3;

            r1 = trigFunctionData_temp->matrix.value[0];
            r2 = trigFunctionData_temp->matrix.value[1];
            r3 = trigFunctionData_temp->matrix.value[2];

            trig_rx = r1[0]*rx+r1[1]*ry+r1[2]*rz-rx;
            trig_ry = r2[0]*rx+r2[1]*ry+r2[2]*rz-ry;
            trig_rz = r3[0]*rx+r3[1]*ry+r3[2]*rz-rz;

            //絶対座標の計算
            ax += trig_rx + (parent_boneFrame.x - parent_bone.locations[0]);
            ay += trig_ry + (parent_boneFrame.y - parent_bone.locations[1]);
            az += trig_rz + (parent_boneFrame.z - parent_bone.locations[2]);


            current_bone_frame->x += ax;
            current_bone_frame->y += ay;
            current_bone_frame->z += az;

            char* tempstr = malloc(5112);
            sprintf(tempstr, "%s  %4f,%4f,%4f 派生:%s\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), current_bone_frame->x, current_bone_frame->y, current_bone_frame->z, word_decode(parent_bone.model_name_jp.byte, parent_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"));
            free(tempstr);

            if(--trigFunctionData_temp->access_count <= 0) {
                free(trigFunctionData_temp);
                count--;
            }
        }else{
            current_bone_frame->x = current_edited_bone.locations[0];
            current_bone_frame->y = current_edited_bone.locations[1];
            current_bone_frame->z = current_edited_bone.locations[2];
            char* tempstr = malloc(5112);
            sprintf(tempstr, "%s %4f,%4f,%4f\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), current_edited_bone.locations[0], current_edited_bone.locations[1], current_edited_bone.locations[2]);
            printf(tempstr);
        }
    }
    free(trigsPtrArray.addr);
}
/*
 * この関数を使用することで、関節の角度によるボーンの移動距離をモデルをベースに算出し、移動座標に付加できる。
 * また、必ずgetMotionのフレームを補完を行ってから実行すること。
*/
void modelPhysics(struct Model model, MotionData *motionData){
    printf("インデックス作成\n");
    char* encode_codec = model.header.encode==1 ? "UTF-8" : "UTF-16";

    struct Index model_name_index = makeIndex();
    struct Index motion_bone_name_index = makeIndex();
    struct Index model_name_index_uft8 = makeIndex(); //モデルのインデックスがutf8で作られたバージョン
    int max_frame = 0; //モーションが何フレームの最大値
    struct BoneFrame** bone_frames = malloc(sizeof(struct BoneFrame)*8192);//[フレーム番号][ボーンインデックス]
    for(int i = 0; i < 8192; i++) bone_frames[i] = malloc((model.bone_size) * sizeof(struct BoneFrame));

    //インデックスを作成
    for(int i0 = 0; i0 < model.bone_size; i0++){
        char* model_born_name = word_decode(model.bone[i0].model_name_jp.byte, model.bone[i0].model_name_jp.byte_size, "UTF-8", encode_codec);

        addIndex(&model_name_index, model.bone[i0].model_name_jp.byte, model.bone[i0].model_name_jp.byte_size);
        addIndex(&model_name_index_uft8, model_born_name, (int)strlen(model_born_name));
        free(model_born_name);
    }
    motion_bone_name_index.assigned = model_name_index.assigned;
    for(int i0 = 0; i0 < motionData->maxFrame.maxFrame; i0++){
        struct BoneFrame* boneFrame = &motionData->boneFrame[i0];
        char* temp = word_decode(boneFrame->name, 15, "UTF-8", "SHIFT-JIS");
        int index = getIndex(model_name_index_uft8, temp);
        free(temp);
        if(index!=-1) {
            if (motion_bone_name_index.name[index][0] == 0)
                memcpy(motion_bone_name_index.name[index], boneFrame->name, 15);
            bone_frames[boneFrame->frame][index] = *boneFrame;

            if(boneFrame->frame > max_frame) max_frame=boneFrame->frame;
        }
    }

    printf("計算開始\n");
    for(int i = 0; i <= max_frame; i++){
        if(i!=108) continue;
        jointCalculation(i,bone_frames[i], &model_name_index, &motion_bone_name_index, model);
        count=0;
    }
    for(int i = 0; i < 8192; i++) free(bone_frames[i]);
    free(bone_frames);
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
    printf("%d\n",motionData.maxFrame.maxFrame);
    writeMotion("/home/shuta/デスクトップ/motion1.vmd", motionData);
    //printf("%s\n", word_decode(motionData.boneFrame[10000].name, 15, "UTF-8", "SHIFT-JIS"));

    struct Model model;
    getModel("/home/shuta/MikuMikuDance_v932x64/models/YYB Hatsune Miku_10th/YYB Hatsune Miku_10th_v1.02.pmx", &model);
    modelPhysics(model, &motionData);

    for(int i = 0; i > model.bone_size; i++) free(model.bone[i].child_bones);
    free(model.bone);
    free(model.texture);
    free(model.surface);
    free(model.topData);
    free(model.material);

    free(motionData.boneFrame);


    return 0;
}