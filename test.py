import VMDConverter
import json

bones = VMDConverter.getModel("/home/shuta/ダウンロード/YYB Hatsune Miku_10th/YYB Hatsune Miku_10th_v1.02.pmx")
motion = json.loads(VMDConverter.getMotion("/home/shuta/ダウンロード/人マニア（モーション配布）/人マニア.vmd"))

for i in motion["boneFrame"]:
    if i["boneName"] not in bones:
        print("除外フレーム", i)