#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント

#include <stdio.h>
#include "vmdStruct.c"
#include <stdbool.h>
#include <malloc.h>
#include "indexlib.c"

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
    MotionData motionData = getMotion("/home/shuta/ダウンロード/HIASOBI_motion.vmd", true);
    free(motionData.boneFrame);

    return 0;
}