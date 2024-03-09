#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

#include <stdio.h>
#include "vmdStruct.c"
#include <stdbool.h>
#include "indexlib.c"
#include <math.h>
#include <stdlib.h>

#define rad_to_deg(rad) ((rad)*180/M_PI)
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
            if (getIndex(index, motionData.boneFrame[i].name) == -1){
                addIndex(&index, motionData.boneFrame[i].name);
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
            int id = getIndex(index, temp.name);
            boneFrame[id][temp.frame] = temp;
        }

        for(int i = 0; i < index.assigned; i++){//ボーンを一つづつループ
            int start = 0;
            int end;
            float position_increment;
            //補完に必要な一時変数
            float pos1, pos2;
            for(int j = 0; j < max_frame; j++){//最大フレーム回ループ

                if (boneFrame[i][j].name[0] == 0 || j == max_frame){//フレームが未登録だった場合
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
                            strlcpy(boneFrame[i][k].name, index.name[i], 15);//名前の設定
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

void writeMotion(const char* output_file_path, MotionData motionData){
    //ファイルに書き込み------------------------------------------
    FILE *fpw = fopen(output_file_path, "w");
    fwrite(&motionData.header, 50, 1, fpw);//ヘッダーを書き込み
    fwrite(&motionData.maxFrame.maxFrame, 1, sizeof(int), fpw);//最大フレームを書き込み
    fwrite(motionData.boneFrame,111,motionData.maxFrame.maxFrame , fpw);//ボーンフレーム
    fclose(fpw);
}

int main(){
    MotionData motionData = getMotion("/home/shuta/ダウンロード/天使の翼モーション/ôVÄgé╠ùââéü[âVâçâô/ôVÄgé╠ùâ.vmd", true);
    printf("%d\n",motionData.maxFrame.maxFrame);
    motionData.boneFrame[570818];
    writeMotion("/home/shuta/ダウンロード/天使の翼モーション/天使の翼_test2.vmd", motionData);
    free(motionData.boneFrame);

    return 0;
}