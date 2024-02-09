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

hop_length = 1024 * 2
max_sound_length = 7034640
max_motion_frame = 8000
look_back = 30

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
        motion_data.append(numpy.zeros((len(index), 7)))

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
        location_array = numpy.array(
            [locations[0], locations[1], locations[2], quaternions[0], quaternions[1], quaternions[2], quaternions[3]])
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
        zero_array = np.zeros(max_sound_length - shape_length)
    except ValueError:
        continue
    y = np.concatenate((y, zero_array))
    # メルスペクトログラムに変換
    x = librosa.feature.melspectrogram(y=y, sr=sr, hop_length=hop_length, win_length=512)
    # dbに変換
    x_dB = librosa.power_to_db(x, ref=np.max)
    print(x_dB.shape)
    Ydata.append(motion_data)
    Xdata.append(x_dB)
    print("--------------------------")

Ydata = numpy.array(Ydata) / 100
Xdata = numpy.array(Xdata) / 80

train = Ydata[0:25]
train1 = Xdata[0:25]
test = Ydata[26:]
test1 = Xdata[26:]

model = models.Sequential()
model.add(layers.Conv2D(64, kernel_size=(3, 3), input_shape=(128, 3435, 1)))
model.add(layers.BatchNormalization())
model.add(layers.LeakyReLU(alpha=0.01))
model.add(layers.MaxPooling2D(pool_size=(2, 2)))
model.add(layers.Conv2D(128, kernel_size=(3, 3)))
model.add(layers.BatchNormalization())
model.add(layers.LeakyReLU(alpha=0.01))
model.add(layers.MaxPooling2D(pool_size=(2, 2)))
model.add(layers.Conv2D(256, kernel_size=(3, 3)))
model.add(layers.BatchNormalization())
model.add(layers.LeakyReLU(alpha=0.01))
model.add(layers.MaxPooling2D(pool_size=(2, 2)))
model.add(layers.Reshape((256, -1)))
model.add(layers.LSTM(256, return_sequences=True, kernel_initializer=keras.initializers.Zeros()))
model.add(layers.LSTM(128, return_sequences=False))
model.add(layers.BatchNormalization())
model.add(layers.LeakyReLU(alpha=0.01))
model.add(layers.Dense(8000))
model.add(layers.Reshape((8000, -1)))
model.add(layers.Dense(len(index)))
model.add(layers.Reshape((8000, len(index), -1)))
model.add(layers.Dense(7))
model.add(layers.BatchNormalization())
model.add(layers.Reshape((8000, len(index), 7)))
# 合体
model.compile(optimizer=keras.optimizers.Adam(learning_rate=0.001), loss="MSE")
# model.summary()
history = model.fit(
    train1, train,
    epochs=50,
    batch_size=4,
    shuffle=True,
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
result = model.predict(train1[0][numpy.newaxis, :, :])[0] * 100


write_motion = {
    "header": {
        "header": "Vocaloid Motion Data 0002",
        "modelName": "AIMotion"
    },
}

bone_frames = []

i_count = 0
for i in result:
    j_count = 0
    for j in i:
        j_count += 1
        if abs(j[0]) >= 1 or abs(j[1]) >= 1 or abs(j[2]) >= 1 or abs(j[3]) >= 1 or abs(j[4]) >= 1 or abs(
                j[5]) >= 1 or abs(j[6]) >= 1:

            bone_name = ""
            for key, value in index.items():
                if value == j_count:
                    bone_name = key
                    break
            print(i_count, "フレーム目")
            print(j)
            bone_frames.append({
                "boneName": bone_name,
                "frame": i_count,
                "locations": [j[0], j[1], j[2]],
                "quaternions": [j[3], j[4], j[5], j[6]],
                "bezier": [[[20.0, 20.0, 20.0, 20.0], [20.0, 20.0, 20.0, 20.0]], [[107.0, 107.0, 107.0, 107.0], [107.0, 107.0, 107.0, 107.0]]]
            })
    i_count += 1

write_motion["maxFrame"] = i_count
write_motion["boneFrame"] = bone_frames

