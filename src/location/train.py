import glob
import json
import os

import tensorflow
import VMDConverter
import librosa
import numpy
import numpy as np
from tensorflow.keras import models, layers
from sklearn.metrics import mean_squared_error

bones = VMDConverter.getModel("..//YYB Hatsune Miku_10th/YYB Hatsune Miku_10th_v1.02.pmx")
datasets = "../motions/"

hop_length = 1024*2
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
    del vmd

Xdata = []
Ydata = []
for path, vmd in vmds.items():
    motion_data = []  # モーションデータ

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
    x_dB = librosa.power_to_db(x, ref=np.max) / 80
    print(x_dB.shape)
    Xdata.append(x_dB)
    # モーションの前処理
    for i in range(max_motion_frame):
        motion_data.append(numpy.zeros((len(index) + 1, 3)))

    for bone in vmd["boneFrame"]:
        if bone["boneName"] not in index:
            continue
        motion_array = motion_data[bone["frame"]]
        locations = bone["locations"]
        location_array = numpy.array([locations[0], locations[1], locations[2]])
        motion_array = numpy.insert(motion_array, index[bone["boneName"]], location_array, axis=0)
        motion_array = numpy.delete(motion_array, len(index), 0)
        motion_data[bone["frame"]] = motion_array
    motion_data = numpy.array(motion_data)
    print(motion_data.shape)
    print(path)
    print("--------------------------")
    Ydata.append(motion_data)
Ydata = numpy.array(Ydata, dtype="float16")/100
Xdata = numpy.array(Xdata, dtype="float16")

train = Ydata[0:6]
train1 = Xdata[0:6]
test = Ydata[7:]
test1 = Xdata[7:]

model = models.Sequential()
model.add(layers.LSTM(64, return_sequences=True))
model.add(layers.LeakyReLU(alpha=0.01))
model.add(layers.Dropout(0.5))
model.add(layers.LSTM(128, return_sequences=True))
model.add(layers.LeakyReLU(alpha=0.01))
model.add(layers.Dropout(0.5))
model.add(layers.Flatten())
model.add(layers.Dense(8000))
model.add(layers.Reshape((8000, -1)))
model.add(layers.Dense(64))
model.add(layers.Reshape((8000, 64, -1)))
model.add(layers.Dense(3))
model.add(layers.Reshape((8000, 64, 3)))

# 合体
model.compile(optimizer="adam", loss="mean_squared_error")
#model.summary()
history = model.fit(
    train1, train,
    epochs=100,
    batch_size=1,
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