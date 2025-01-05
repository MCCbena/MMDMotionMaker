#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

#include <stdio.h>
#include "vmdStruct.h"
#include <stdbool.h>
#include "indexlib.h"
#include <math.h>
#include "../pmx/modelLoader.c"
#include "Rotation.h"


const char bezier[64] = {20, 20, 0, 0, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 20, 20, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 20, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 0, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 0, 0};

float getPos(const float pos1, const float pos2, float time){
    return (3* powf(1.0f-time, 2)*time*pos1+3.0f* (1-time)* powf(time, 2)*pos2+ powf(time, 3));
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


                            struct Quaternion q1, q2;
                            q1.x = final_registration_boneFrame.qx;
                            q1.y = final_registration_boneFrame.qy;
                            q1.z = final_registration_boneFrame.qz;
                            q1.w = final_registration_boneFrame.qw;

                            q2.x = boneFrame[i][end].qx;
                            q2.y = boneFrame[i][end].qy;
                            q2.z = boneFrame[i][end].qz;
                            q2.w = boneFrame[i][end].qw;

                            if(q2.x + q2.y + q2.z + q2.w == 0) q2.w = 1; //最終フレームにボーンフレームが何も代入されていないことがあるため、クォータニオンの合計値が0になるときがある。

                            struct Quaternion quaternion = SphericalLinearInterpolation(q1, q2, t);
                            boneFrame[i][k].qw = (float)quaternion.w;
                            boneFrame[i][k].qx = (float)quaternion.x;
                            boneFrame[i][k].qy = (float)quaternion.y;
                            boneFrame[i][k].qz = (float)quaternion.z;
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
struct PtrArray{
    void** addr;
    int size;
    int access_count;
};
struct BoneData{
    int bone_index; //この三角関数を計算するために使ったボーンのインデックス

    int access_count;
};
int count = 0; //現在、どれだけのスレッドが同時に動いているか代入する
void jointCalculation(int frame, struct BoneFrame* current_frames, struct Index *model_index, struct Index *motion_index, struct Model model, struct PtrArray trigsPtrArray){
    for(int bone_i = 0; bone_i < model_index->assigned; bone_i++){
        struct BoneFrame *current_bone_frame = &current_frames[bone_i];
        struct Bone current_edited_bone = model.bone[bone_i];
        //クォータニオンからオイラー角を算出。回転順序はYXZで、オイラー角のYとZに-1をかける必要がある。
        if(current_bone_frame->name[0] == 0){
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

        }

        //親ボーンらが移動した合計を計算
        if(trigsPtrArray.addr[bone_i] != NULL){

            struct BoneData *trigFunctionData_temp = (struct BoneData*) trigsPtrArray.addr[bone_i];
            struct Bone parent_bone = model.bone[trigFunctionData_temp->bone_index];
            struct BoneFrame parent_boneFrame = current_frames[trigFunctionData_temp->bone_index];
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

            double trig_rx=0, trig_ry=0, trig_rz=0;
            double *r1, *r2, *r3;
            struct Matrix matrix = QuaternionToMatrix(quaternion_p);

            r1 = matrix.value[0];
            r2 = matrix.value[1];
            r3 = matrix.value[2];

            trig_rx = r1[0]*rx+r1[1]*ry+r1[2]*rz;
            trig_ry = r2[0]*rx+r2[1]*ry+r2[2]*rz;
            trig_rz = r3[0]*rx+r3[1]*ry+r3[2]*rz;

            //絶対座標の計算
            double ax = trig_rx + current_bone_frame->x + parent_boneFrame.x;
            double ay = trig_ry + current_bone_frame->y + parent_boneFrame.y;
            double az = trig_rz + current_bone_frame->z + parent_boneFrame.z;


            current_bone_frame->x = (float)ax;
            current_bone_frame->y = (float)ay;
            current_bone_frame->z = (float)az;

            struct Quaternion q = qmul(quaternion_c, quaternion_p);
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
            current_bone_frame->x = current_edited_bone.locations[0];
            current_bone_frame->y = current_edited_bone.locations[1];
            current_bone_frame->z = current_edited_bone.locations[2];
            /*
            char* tempstr = malloc(5112);
            sprintf(tempstr, "%s %4f,%4f,%4f\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), current_edited_bone.locations[0], current_edited_bone.locations[1], current_edited_bone.locations[2]);
            free(tempstr);
             */
        }
    }
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

    struct PtrArray trigsPtrArray;//モデルと同じインデックスに、親ボーンの回転角が代入された三角関数構造体が代入される
    trigsPtrArray.addr = malloc(sizeof(long) * model_name_index.assigned);
    for(int i = 0; i < model_name_index.assigned; i++) trigsPtrArray.addr[i] = NULL;

    trigsPtrArray.size = model_name_index.assigned;
    for(int bone_i = 0; bone_i < model_name_index.assigned; bone_i++) {
        struct Bone current_edited_bone = model.bone[bone_i];
        //printf("親ボーン:%s, ", word_decode(parent_boneFrame->name, 15, "UTF-8", "SHIFT-JIS"));
        //current_bone_frame->y-=0.1f; //TODO 0.1マイナスする（MMDがy軸に0.1ずれてる）
        //子ボーンに加算する値を代入
        struct BoneData *boneData = malloc(sizeof(struct BoneData));
        boneData->bone_index = bone_i;
        for (int child_bone_i = 0; child_bone_i < current_edited_bone.child_bone_size; child_bone_i++) {
            int child_bone_index = current_edited_bone.child_bones[child_bone_i];
            trigsPtrArray.addr[child_bone_index] = boneData;
        }
    }
    printf("計算開始\n");
    for(int i = 0; i <= max_frame; i++){
        jointCalculation(i,bone_frames[i], &model_name_index, &motion_bone_name_index, model, trigsPtrArray);
        if(i%100==0){
            printf("%dフレーム\n", i);
        }
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