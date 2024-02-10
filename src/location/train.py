import gc
import glob
import json
import os

import keras.optimizers
import tensorflow
import VMDConverter
import librosa
import numpy
import numpy as np
from tensorflow.keras import models, layers
from sklearn.metrics import mean_squared_error

bones = VMDConverter.getModel("../../YYB Hatsune Miku_10th/YYB Hatsune Miku_10th_v1.02.pmx")
datasets = "../../dataset/"

hop_length = 735
max_sound_length = 7938000
max_motion_frame = 10800
time_step = 100

gpus = tensorflow.config.list_physical_devices('GPU')
if gpus:
    # Restrict TensorFlow to only use the first GPU
    try:
        tensorflow.config.set_visible_devices(gpus[0], 'GPU')
        logical_gpus = tensorflow.config.list_logical_devices('GPU')
        print(len(gpus), "Physical GPUs,", len(logical_gpus), "Logical GPU")
    except RuntimeError as e:
        # Visible devices must be set before GPUs have been initialized
        print(e)

# インデックスの作成
index = {}
vmds = {}

for dataset in os.listdir(datasets):
    bone_count = {}
    file_name = glob.glob(datasets + dataset + "/*.vmd")[0]
    vmd = json.loads(VMDConverter.getMotion(file_name))
    vmds[datasets + dataset] = vmd
    print(dataset)
    total_bone = len(vmd["boneFrame"])

    # ボーンが何個あるか確認
    for bone in vmd["boneFrame"]:
        if bone["boneName"] in bone_count:
            bone_count[bone["boneName"]] += 1
        else:
            bone_count[bone["boneName"]] = 1

    for key, value in bone_count.items():
        if (value / total_bone * 100 > 1) and (key not in index) and (key in bones):
            index[key] = len(index)
    gc.collect()

Xdata = []
Ydata = []
for path, vmd in vmds.items():
    skip = False
    motion_data = []  # モーションデータ
    # モーションの前処理
    print(path)
    for i in range(max_motion_frame):
        motion_data.append(numpy.zeros((len(index), 3)))

    for bone in vmd["boneFrame"]:
        if bone["boneName"] not in index:
            continue
        try:
            motion_array = motion_data[bone["frame"]]
        except:
            skip = True
            break
        locations = bone["locations"]
        quaternions = bone["quaternions"]
        location_array = numpy.array([locations[0], locations[1], locations[2]])
        motion_array = numpy.insert(motion_array, index[bone["boneName"]], location_array, axis=0)
        motion_array = numpy.delete(motion_array, len(index), 0)
        motion_data[bone["frame"]] = motion_array
    if skip:
        print("スキップします")
        continue
    motion_data = numpy.array(motion_data)
    print(motion_data.shape)

    # オーディオの前処理
    y, sr = librosa.load(path + "/music.wav")
    shape_length = len(y)
    try:
        zero_array = np.zeros(max_sound_length - shape_length-1)
    except ValueError:
        continue
    y = np.concatenate((y, zero_array))
    # メルスペクトログラムに変換
    x = librosa.feature.melspectrogram(y=y, sr=sr, hop_length=hop_length, win_length=512)
    # dbに変換
    x_dB = librosa.power_to_db(x, ref=np.max)
    print(x_dB.shape)
    x_dB = x_dB.reshape((time_step, -1))
    print(x_dB.shape)
    Ydata.append(motion_data)
    Xdata.append(x_dB)
    print("--------------------------")

Ydata = numpy.array(Ydata)
Xdata = numpy.array(Xdata)

train = Ydata[0:17]
train1 = Xdata[0:17]
test = Ydata[18:]
test1 = Xdata[18:]

model = models.Sequential()
model.add(layers.LSTM(units=256, return_sequences=True))
model.add(layers.BatchNormalization())
model.add(layers.LeakyReLU(alpha=0.01))
model.add(layers.LSTM(units=128, return_sequences=True))
model.add(layers.BatchNormalization())
model.add(layers.LeakyReLU(alpha=0.01))
model.add(layers.Conv1D(2160, 3, padding='same'))
model.add(layers.LeakyReLU(alpha=0.01))
model.add(layers.UpSampling1D(size=108))
model.add(layers.Reshape((10800, -1)))
model.add(layers.Conv1D(3 * 73, 3, padding='same'))
model.add(layers.LeakyReLU(alpha=0.01))
model.add(layers.Reshape((10800, 73, 3)))


# 合体
model.compile(optimizer=keras.optimizers.Adam(learning_rate=0.01), loss="MSE")
# model.summary()
history = model.fit(
    train1, train,
    epochs=50,
    batch_size=1,
    validation_data=(test1, test)
)

# 学習結果の可視化
import matplotlib.pyplot as plt

plt.plot(history.history['loss'])
plt.plot(history.history['val_loss'])
plt.title('Model accuracy')
plt.ylabel('Accuracy')
plt.xlabel('Epoch')
plt.grid()
plt.legend(['Train', 'Validation'], loc='upper left')
plt.savefig("plot.png", format="png")
result = model.predict(train1[0][numpy.newaxis, :, :])[0]*100

write_motion = {
    "header": {
        "header": "Vocaloid Motion Data 0002",
        "modelName": "AIMotion"
    },
}

bone_frames = []

max_frame = 600
last_frame = 0
i_count = 0
for i in result:
    j_count = 0
    if i_count > max_frame:
        print("終了")
        break
    for j in i:
        j_count += 1
        if abs(j[0]) >= 1 or abs(j[1]) >= 1 or abs(j[2]) >= 1:

            print(j)
            bone_name = ""
            for key, value in index.items():
                if value == j_count:
                    bone_name = key
                    break
            bone_frames.append({
                "boneName": bone_name,
                "frame": i_count,
                "locations": [float(j[0]), float(j[1]), float(j[2])],
                "quaternions": [0.0, 0.0, 0.0, 0.0],
                "bezier": [[[20.0, 20.0, 20.0, 20.0], [20.0, 20.0, 20.0, 20.0]],
                           [[107.0, 107.0, 107.0, 107.0], [107.0, 107.0, 107.0, 107.0]]]
            })
            last_frame = i_count

    i_count += 1

write_motion["boneFrame"] = bone_frames
write_motion["maxFrame"] = last_frame

print(write_motion)
print("書き込み")
VMDConverter.writeMotion("test.vmd", json.dumps(write_motion))
