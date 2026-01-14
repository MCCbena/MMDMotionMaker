#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

#include <stdio.h>
#include <stdlib.h>
#include "vmdStruct.h"
#include "../pmx/pmxStruct.h"
#include <stdbool.h>
#include "indexlib.h"
#include <math.h>
#include "Rotation.h"


const char bezier[64] = {20, 20, 0, 0, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 20, 20, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 20, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 0, 20, 20, 20, 20, 20, 107, 107, 107, 107, 107, 107, 107, 107, 0, 0, 0};

long double clipByValu(long double value, long double max, long double min){
    if(max < value) return max;
    if(min > value) return min;
    return value;
}

long double getPos(const long double pos1, const long double pos2, long double time){
    return (3* powl(1.0-time, 2)*time*pos1+3.0f* (1-time)* powl(time, 2)*pos2 + powl(time, 3));
}

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

    motionData.boneFrame = calloc(motionData.maxFrame.maxFrame, sizeof(struct BoneFrame));
    fread(&*motionData.boneFrame, sizeof(struct BoneFrame), motionData.maxFrame.maxFrame, fpw);
    fclose(fpw);


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
        struct BoneFrame **pBoneFrame = (struct BoneFrame**) calloc(index.assigned, sizeof(*pBoneFrame));

        for(int i = 0; i < index.assigned; i++){
            pBoneFrame[i] = calloc(max_frame, sizeof(struct BoneFrame));
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
                                float x = (float)(final_registration_boneFrame.x + position_increment * getPos(pos1, pos2,time_count_base *(float) (k -start +1)));
                                pBoneFrame[i][k].x = x;
                            }else pBoneFrame[i][k].x = final_registration_boneFrame.x;
                            //座標y
                            if(final_registration_boneFrame.y != pBoneFrame[i][end].y) {
                                pos1 = (float) final_registration_boneFrame.bezier[5] / 127;
                                pos2 = (float) final_registration_boneFrame.bezier[13] / 127;
                                position_increment = pBoneFrame[i][end].y - final_registration_boneFrame.y;
                                float y = (float)(final_registration_boneFrame.y + position_increment * getPos(pos1, pos2,time_count_base *(float) (k -start +1)));
                                pBoneFrame[i][k].y = y;
                            }else pBoneFrame[i][k].y = final_registration_boneFrame.y;
                            //座標z
                            if(final_registration_boneFrame.z != pBoneFrame[i][end].z) {
                                pos1 = (float) final_registration_boneFrame.bezier[6] / 127;
                                pos2 = (float) final_registration_boneFrame.bezier[14] / 127;
                                position_increment = pBoneFrame[i][end].z - final_registration_boneFrame.z;
                                float z = (float)(final_registration_boneFrame.z + position_increment * getPos(pos1, pos2,time_count_base *(float) (k -start +1)));
                                pBoneFrame[i][k].z = z;
                            } else pBoneFrame[i][k].z = final_registration_boneFrame.z;


                            //クォータニオン
                            pos1 = (float) final_registration_boneFrame.bezier[7] / 127;
                            pos2 = (float) final_registration_boneFrame.bezier[15] / 127;


                            float t = (float)(getPos(pos1, pos2, time_count_base *(float) (k - start +1)));


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
        struct BoneFrame *synthesis_boneFrame = calloc( index.assigned*max_frame, sizeof(struct BoneFrame));
        int n = 0;

        for(int i = 0; i < index.assigned; i++){
            for(int j = 0; j < max_frame; j++){
                memcpy(pBoneFrame[i][j].bezier, bezier, 64);//ベジェのコピー
                synthesis_boneFrame[n] = pBoneFrame[i][j];
                n++;
            }
            //free(pBoneFrame[i]);
            //pBoneFrame[i] = NULL;
        }
        //free(pBoneFrame);
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
        free(pBoneFrame);
        pBoneFrame = NULL;
    }

    return motionData;
}


struct Vector3 CalPositionFromQuat(float rx, float ry, float rz, struct Quaternion quaternion){
    long double *r1, *r2, *r3;
    struct Matrix matrix = QuaternionToMatrix(quaternion);
    struct Vector3 ret;

    r1 = matrix.value[0];
    r2 = matrix.value[1];
    r3 = matrix.value[2];

    ret.x = r1[0] * rx + r1[1] * ry + r1[2] * rz;
    ret.y = r2[0] * rx + r2[1] * ry + r2[2] * rz;
    ret.z = r3[0] * rx + r3[1] * ry + r3[2] * rz;

    return ret;
}
int IKSortCompare(const void *a, const void *b){
    return (*(IKLink*)b).linkBone_index_size - (*(IKLink*)a).linkBone_index_size;
}
/*
 * frame:フレーム番号
 * current_frames:処理を行うフレーム（1フレーム）の中に含まれているモーションデータ
 * encodeBoneFrame:処理内容を渡す変数
 * model:基準モデル
 * parentBoneDataPtrArray:ボーンの親子関係。parentBoneDataPtrArray[子ボーンのインデックス]で親ボーンのインデックスが取得できる。
 */
void jointCalculationEncoder(int frame, struct BoneFrame* current_frames, struct Index *model_index, struct EncodeBoneFrame *encodeBoneFrame, struct Model model, const int* parentBoneDataPtrArray){
    struct BoneFrame* current_frame_temp = malloc(sizeof(struct BoneFrame)*model_index->assigned);
    for(int bone_i = 0; bone_i < model_index->assigned; bone_i++){
        struct BoneFrame *current_bone_frame = &current_frame_temp[bone_i];
        current_bone_frame->x = current_frames[bone_i].x;
        current_bone_frame->y = current_frames[bone_i].y;
        current_bone_frame->z = current_frames[bone_i].z;
        memcpy(current_bone_frame->name, current_frames[bone_i].name, 15);
        memcpy(current_bone_frame->bezier, current_frames[bone_i].bezier, 64);
        current_bone_frame->frame = current_frames[bone_i].frame;
        current_bone_frame->qw = current_frames[bone_i].qw;
        current_bone_frame->qx = current_frames[bone_i].qx;
        current_bone_frame->qy = current_frames[bone_i].qy;
        current_bone_frame->qz = current_frames[bone_i].qz;

        struct Bone current_edited_bone = model.bone[bone_i];
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
            struct BoneFrame parent_boneFrame = current_frame_temp[parent_bone_index];
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

            struct Vector3 combinations = CalPositionFromQuat(rx, ry, rz, quaternion_p);
            //絶対座標の計算
            long double ax = combinations.x + current_bone_frame->x + parent_boneFrame.x;
            long double ay = combinations.y + current_bone_frame->y + parent_boneFrame.y;
            long double az = combinations.z + current_bone_frame->z + parent_boneFrame.z;


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


            /*
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
             */
        }else{
            current_bone_frame->x += current_edited_bone.locations[0];
            current_bone_frame->y += current_edited_bone.locations[1];
            current_bone_frame->z += current_edited_bone.locations[2];

            //エンコードボーンフレーム構造体に代入
            encodeBoneFrame[bone_i].x = current_edited_bone.locations[0];
            encodeBoneFrame[bone_i].y = current_edited_bone.locations[1];
            encodeBoneFrame[bone_i].z = current_edited_bone.locations[2];

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

    //IKに関する計算
    struct Quaternion quaternion_buffer[model.bone_size];
    for (int i = 0; i < model.bone_size; ++i) {
        quaternion_buffer[i].w = 1;
        quaternion_buffer[i].x = 0;
        quaternion_buffer[i].y = 0;
        quaternion_buffer[i].z = 0;
    }
    for (int bone_i = 0; bone_i < model_index->assigned; ++bone_i) {
        struct Bone current_edited_bone = model.bone[bone_i];

        if(current_edited_bone.bone_flags&0x0020){ //IKが有効だった場合
            //IKのソート
            IKLink *sorted_IKlink = malloc(sizeof(IKLink)*current_edited_bone.ik.IK_link_count);
            for (int i = 0; i < current_edited_bone.ik.IK_link_count; ++i) {
                sorted_IKlink[i] = current_edited_bone.ik.ikLink[i];
            }
            qsort(sorted_IKlink, current_edited_bone.ik.IK_link_count, sizeof(IKLink), IKSortCompare); //降順

            int effector_index = current_edited_bone.ik.IK_targetBone_index_size;
            struct Vector3 target_pos = {encodeBoneFrame[bone_i].x, encodeBoneFrame[bone_i].y, encodeBoneFrame[bone_i].z};
            int apply_ik_indices[current_edited_bone.ik.IK_link_count+1];
            apply_ik_indices[current_edited_bone.ik.IK_link_count] = effector_index;
            for (int i = 0; i < current_edited_bone.ik.IK_link_count; ++i) {
                apply_ik_indices[i] = sorted_IKlink[current_edited_bone.ik.IK_link_count-i-1].linkBone_index_size;
            }
            //IKの調整ループ
            for (int ik_loop = 0; ik_loop < current_edited_bone.ik.IK_loop_count; ++ik_loop) {
                for (int i = 0; i < current_edited_bone.ik.IK_link_count; ++i) {
                    int index = sorted_IKlink[i].linkBone_index_size;
                    struct Vector3 joint_pos = {encodeBoneFrame[index].x, encodeBoneFrame[index].y, encodeBoneFrame[index].z};

                    struct Vector3 effector_pos = {encodeBoneFrame[effector_index].x, encodeBoneFrame[effector_index].y, encodeBoneFrame[effector_index].z};
                    struct Vector3 to_effector = NormalizationV(minusV(effector_pos, joint_pos));
                    struct Vector3 to_target = NormalizationV(minusV(target_pos, joint_pos));

                    struct Vector3 axis = NormalizationV(crossV(to_effector, to_target));
                    //if((axis.x*axis.x + axis.y*axis.y + axis.z*axis.z) >= 1) continue;

                    long double angle = dotV(to_effector, to_target);
                    if(angle > 1) angle=1;
                    if(angle < -1) angle=-1;
                    angle = clipByValu(acosl(angle), current_edited_bone.ik.IK_limit_angle, 0);
                    if(fabsl(angle) < 1e-6) continue;

                    struct Quaternion IK_quat = QuaternionFromAxisAngle(axis, angle);
                    struct Quaternion IK_bone_buffer_quat = quaternion_buffer[index];
                    struct Quaternion IK_add = qmul(IK_bone_buffer_quat, IK_quat);
                    struct Quaternion q_c = {encodeBoneFrame[index].qw, encodeBoneFrame[index].qx, encodeBoneFrame[index].qy, encodeBoneFrame[index].qz};
                    struct Quaternion limit_q_c;
                    if(sorted_IKlink[i].limit_angele) {
                        struct Vector3 vec1 = QuaternionToEuler(IK_add);
                        vec1.x = clipByValu(vec1.x, sorted_IKlink[i].upper_limit[0], sorted_IKlink[i].lower_limit[0]);
                        vec1.y = clipByValu(vec1.y, sorted_IKlink[i].upper_limit[1], sorted_IKlink[i].lower_limit[1]);
                        vec1.z = clipByValu(vec1.z, sorted_IKlink[i].upper_limit[2], sorted_IKlink[i].lower_limit[2]);
                        IK_add = EulerToQuaternion(vec1);

                        limit_q_c = (qmul(inverse(IK_bone_buffer_quat), IK_add));
                    }else{
                        struct Vector3 vec1 = QuaternionToEuler(IK_add);
                        //vec1.y = clipByValu(vec1.y, M_PI, 0);
                        IK_add = EulerToQuaternion(vec1);

                        limit_q_c = (qmul(inverse(IK_bone_buffer_quat), IK_add));
                    }

                    quaternion_buffer[index] = IK_add;
                    struct Quaternion q_c_add = qmul(q_c, limit_q_c);
                    encodeBoneFrame[index].qw = q_c_add.w;
                    encodeBoneFrame[index].qx = q_c_add.x;
                    encodeBoneFrame[index].qy = q_c_add.y;
                    encodeBoneFrame[index].qz = q_c_add.z;
                    //回転の適応
                    for (int j = current_edited_bone.ik.IK_link_count-i; j < current_edited_bone.ik.IK_link_count+1; ++j) {
                        int ik_bone_index = apply_ik_indices[j];
                        int IK_parent_bone_index = parentBoneDataPtrArray[ik_bone_index];
                        struct Bone parent_bone = model.bone[IK_parent_bone_index];

                        //子ボーンを正とした相対座標(Relative Coordinates)を計算
                        float rx = model.bone[ik_bone_index].locations[0] - parent_bone.locations[0];
                        float ry = model.bone[ik_bone_index].locations[1] - parent_bone.locations[1];
                        float rz = model.bone[ik_bone_index].locations[2] - parent_bone.locations[2];

                        struct Quaternion q = {encodeBoneFrame[IK_parent_bone_index].qw, encodeBoneFrame[IK_parent_bone_index].qx, encodeBoneFrame[IK_parent_bone_index].qy, encodeBoneFrame[IK_parent_bone_index].qz};
                        struct Vector3 combinations = CalPositionFromQuat(rx, ry, rz, q);
                        //絶対座標の計算
                        long double ax = combinations.x + encodeBoneFrame[IK_parent_bone_index].x;
                        long double ay = combinations.y + encodeBoneFrame[IK_parent_bone_index].y;
                        long double az = combinations.z + encodeBoneFrame[IK_parent_bone_index].z;

                        //エンコードボーンフレーム構造体に代入
                        encodeBoneFrame[ik_bone_index].x = ax;
                        encodeBoneFrame[ik_bone_index].y = ay;
                        encodeBoneFrame[ik_bone_index].z = az;


                        //if(j == current_edited_bone.ik.IK_link_count) continue;
                        struct Quaternion quat_encode_frame_add = qmul(quaternion_buffer[ik_bone_index], q_c_add);
                        encodeBoneFrame[ik_bone_index].qw = quat_encode_frame_add.w;
                        encodeBoneFrame[ik_bone_index].qx = quat_encode_frame_add.x;
                        encodeBoneFrame[ik_bone_index].qy = quat_encode_frame_add.y;
                        encodeBoneFrame[ik_bone_index].qz = quat_encode_frame_add.z;
                    }
                }
            }
            free(sorted_IKlink);
        }
    }
    free(current_frame_temp);
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
EncodeMotionData motionEncoder(struct Model model, MotionData *motionData, int stride){
    //printf("インデックス作成\n");
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
        memcpy(encodeMotionData.nameIndexer[encodeMotionData.nameIndexer_size].name, temp, strlen(temp));
        encodeMotionData.nameIndexer[encodeMotionData.nameIndexer_size].name_byte = (int)strlen(temp);
        encodeMotionData.nameIndexer[encodeMotionData.nameIndexer_size++].index = n;
        free(temp);

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
    //printf("計算開始\n");
    max_frame /= stride;
    encodeMotionData.encodeBoneFrame = calloc(sizeof(struct EncodeBoneFrame), max_frame+1);
    encodeMotionData.encodeBoneFrame_size = max_frame+1;
    for(int i = 0; i <= max_frame; i++){
        encodeMotionData.encodeBoneFrame[i] = calloc(sizeof(struct EncodeBoneFrame), model.bone_size);
        jointCalculationEncoder(i, bone_frames[i*stride], &model_name_index, encodeMotionData.encodeBoneFrame[i], model,
                                parentBoneDataArray);
    }
    //printf("完了\n");
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

int debug = 0;
//エンコードされたencodeBoneFrameで欠落しているボーンを復元する
void jointCompletion(struct EncodeBoneFrame* current_frames, struct Index *model_index, struct Model model, const int* parentBoneDataPtrArray){
    for(int bone_i = 0; bone_i < model_index->assigned; bone_i++){
        struct EncodeBoneFrame *current_bone_frame = &current_frames[bone_i];
        struct Bone current_edited_bone = model.bone[bone_i];
        //クォータニオンからオイラー角を算出。回転順序はYXZで、オイラー角のYとZに-1をかける必要がある。
        if(sqrtl(powl(current_bone_frame->qx, 2) + powl(current_bone_frame->qy, 2) + powl(current_bone_frame->qz, 2) + powl(current_bone_frame->qw, 2)) == 0.0){
            //x,y,zとqx,qy,qzを0に、qwを1に初期化
            float *locations = (float*)&current_bone_frame->x; //#pragma pack(1)でメモリが詰められているため有効に動作する。
            for(int i = 0; i < 7; i++) {
                locations[i] = (i == 6) ? 1.0f : 0.0f;
            }


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
                struct Quaternion quaternion_c;
                quaternion_c.x = 0;
                quaternion_c.y = 0;
                quaternion_c.z = 0;
                quaternion_c.w = 1;

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
                long double ax = combination_rx + parent_boneFrame.x;
                long double ay = combination_ry + parent_boneFrame.y;
                long double az = combination_rz + parent_boneFrame.z;

                /*
                if(debug==1) {
                    printf("補完: %s, %Lf, %Lf, %Lf -> ", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), ax, ay, az);
                    printf("親: %s, %Lf, %Lf, %Lf\n", word_decode(parent_bone.model_name_jp.byte, parent_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), parent_boneFrame.x, parent_boneFrame.y, parent_boneFrame.z);
                }
                 */

                current_bone_frame->x = (float)ax;
                current_bone_frame->y = (float)ay;
                current_bone_frame->z = (float)az;

                //printf("%s\n", word_decode(current_bone_frame->name, 15, "UTF-8", "SHIFT-JIS"));

                current_bone_frame->qx = (float)quaternion_p.x;
                current_bone_frame->qy = (float)quaternion_p.y;
                current_bone_frame->qz = (float)quaternion_p.z;
                current_bone_frame->qw = (float)quaternion_p.w;
                /*
                char* tempstr = malloc(5112);
                sprintf(tempstr, "%s  %4f,%4f,%4f 派生:%s\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), current_bone_frame->x, current_bone_frame->y, current_bone_frame->z, word_decode(parent_bone.model_name_jp.byte, parent_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"));
                free(tempstr);
                 */

            }else{
                //エンコードボーンフレーム構造体に代入
                current_bone_frame->x = current_edited_bone.locations[0];
                current_bone_frame->y = current_edited_bone.locations[1];
                current_bone_frame->z = current_edited_bone.locations[2];

                current_bone_frame->qx = 0;
                current_bone_frame->qy = 0;
                current_bone_frame->qz = 0;
                current_bone_frame->qw = 1;
                /*
                char* tempstr = malloc(5112);
                sprintf(tempstr, "%s %4f,%4f,%4f\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), current_edited_bone.locations[0], current_edited_bone.locations[1], current_edited_bone.locations[2]);
                free(tempstr);
                 */

                //if(debug==1)printf("親不在補完:%s\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"));
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

            //if(debug==1) printf("%s, %s, %Lf, %Lf, %Lf\n", word_decode(current_edited_bone.model_name_jp.byte, current_edited_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), word_decode(parent_bone.model_name_jp.byte, parent_bone.model_name_jp.byte_size, "UTF-8", "UTF-16"), ax, ay, az);
            struct Quaternion q = inverse(qmul((quaternion_p), inverse(quaternion_c)));
            /*
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
             */

            //エンコードボーンフレーム構造体に代入
            if(current_edited_bone.bone_flags & 0x0004) {
                decodeBoneFrame[bone_i].x = (float) ax;
                decodeBoneFrame[bone_i].y = (float) ay;
                decodeBoneFrame[bone_i].z = (float) az;
            }

            if(current_edited_bone.bone_flags & 0x0002) {
                decodeBoneFrame[bone_i].qx = (float) q.x;
                decodeBoneFrame[bone_i].qy = (float) q.y;
                decodeBoneFrame[bone_i].qz = (float) q.z;
                decodeBoneFrame[bone_i].qw = (float) q.w;
            }

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
MotionData motionDecoder(EncodeMotionData encodeMotionData, struct Model model, struct Index need_bone, int stride){
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
            if(strncmp(temp, encodeMotionData.nameIndexer[i1].name, 15) == 0){
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
        if(i0==0) debug=1;
        else debug=0;
        jointCompletion(encodeBoneFrame2D[i0], &model_name_index_utf8, model, link);
        jointCalculationDecoder(i0, encodeBoneFrame2D[i0], &motion_name_index, decodeBoneFrame2D[i0], model, link);
    }

    //モーションデータの作成
    MotionData motionData;

    //ヘッダの書き込み
    char* modelName = word_decode(model.modelInfo.model_name_jp.byte, model.modelInfo.model_name_jp.byte_size, "SHIFT-JIS", model.header.encode == 0 ? "UTF-16" : "UTF-8");
    memcpy(motionData.header.modelName, modelName, model.modelInfo.model_name_jp.byte_size > 20 ? 20 : model.modelInfo.model_name_jp.byte_size);
    memcpy(motionData.header.header, "Vocaloid Motion Data 0002", 25);
    free(modelName);

    motionData.boneFrame = calloc(sizeof(struct BoneFrame), max_frame*encodeMotionData.nameIndexer_size);
    int assigned = 0;
    for (int i0 = 0; i0 < motion_name_index.assigned; ++i0) {
        for (int i1 = 0; i1 < need_bone.assigned; ++i1) {
            if(strcmp(motion_name_index.name[i0], need_bone.name[i1]) == 0){
                for (int i2 = 0; i2 < max_frame; ++i2) {
                    decodeBoneFrame2D[i2][i0].frame *= stride;
                    motionData.boneFrame[assigned] = decodeBoneFrame2D[i2][i0];
                    assigned++;
                }
                break;
            }
        }
    }
    motionData.maxFrame.maxFrame = assigned;

    for (int i = 0; i < encodeMotionData.encodeBoneFrame_size; ++i) {
        free(encodeBoneFrame2D[i]);
        encodeBoneFrame2D[i] = NULL;
        free(decodeBoneFrame2D[i]);
        decodeBoneFrame2D[i] = NULL;
    }
    free(encodeBoneFrame2D);
    encodeBoneFrame2D = NULL;
    free(decodeBoneFrame2D);
    decodeBoneFrame2D = NULL;

    destroy_index(&motion_name_index);
    destroy_index(&model_name_index_utf8);

    return motionData;
}

void writeMotion(const char* output_file_path, MotionData motionData){
    //ファイルに書き込み------------------------------------------
    FILE *fpw = fopen(output_file_path, "w");
    fwrite(&motionData.header, 50, 1, fpw);//ヘッダーを書き込み
    fwrite(&motionData.maxFrame.maxFrame, 1, sizeof(int), fpw);//最大フレームを書き込み
    fwrite(motionData.boneFrame,111,motionData.maxFrame.maxFrame, fpw);//ボーンフレーム
    fclose(fpw);
}

/*
#include "../pmx/modelLoader.c"
int main(){
    for (int k = 0; k < 1; ++k) {

        MotionData motionData = getMotion("/mnt/E86884F46884C334/D/src/motions/sm19277556/夏に去りし君を想フ_モーション.vmd", true);
        printf("%d\n", motionData.maxFrame.maxFrame);
        writeMotion("/home/shuta/デスクトップ/motion1.vmd", motionData);
        //printf("%s\n", word_decode(motionData.boneFrame[10000].name, 15, "UTF-8", "SHIFT-JIS"));

        struct Index bone_index = makeIndex(512, 15);
        for (int i = 0; i < motionData.maxFrame.maxFrame; i++) {
            if (getnIndex(bone_index, motionData.boneFrame[i].name, 15) == -1)
                addIndex(&bone_index, motionData.boneFrame[i].name, 15);
        }

        struct Model model;
        getModel("/home/shuta/MikuMikuDance_v932x64/models/YYB Hatsune Miku_10th/YYB Hatsune Miku_10th_v1.02.pmx",
                 &model);
        EncodeMotionData encodeMotionData = motionEncoder(model, &motionData, 4);
        MotionData decodeMotionData = motionDecoder(encodeMotionData, model, bone_index, 4);
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
        free(decodeMotionData.boneFrame);

        destroy_index(&bone_index);

    }
    return 0;
}
 */