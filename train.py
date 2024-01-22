import librosa
import numpy as np
import tensorflow as tf
from tensorflow.keras import models, layers
import os


hop_length = 1024 * 2

# データセットはmusicディレクトリに入れる
dataset_path = "music/"

gpus = tf.config.list_physical_devices('GPU')
if gpus:
    try:
        # Currently, memory growth needs to be the same across GPUs
        for gpu in gpus:
            tf.config.experimental.set_memory_growth(gpu, True)
        logical_gpus = tf.config.list_logical_devices('GPU')
        print(len(gpus), "Physical GPUs,", len(logical_gpus), "Logical GPUs")
    except RuntimeError as e:
        # Memory growth must be set before GPUs have been initialized
        print(e)

max_length = 0
librosa_loads = []

Xdata = []

for path in os.listdir(dataset_path):
    # wavファイルをロードして配列に代入
    y, sr = librosa.load(dataset_path + path)
    librosa_loads.append([y, sr])
    # 一番でかい数値を代入
    if len(y) > max_length:
        max_length = len(y)

for i in librosa_loads:
    y, sr = i

    # さっき保存した一番長いファイルの数値から今のファイルの再生時間を引いて、足りない分を0で埋める
    shape_length = len(y)
    zero_array = np.zeros(max_length - shape_length)
    y = np.concatenate((y, zero_array))

    # メルスペクトログラムに変換
    x = librosa.feature.mfcc(y=y, sr=sr, n_mfcc=13)
    print(x.shape)
    Xdata.append(x / 32768)  # データを-1.0から-0.999...の間にする

# メルスペクトログラムをnumpy配列にぶちこむ
x = np.array(Xdata)
print(x.shape)
Xdata = x.reshape((len(x), np.prod(x.shape[1:])))

# 学習とテストデータに分類（めんどいからランダムなし
Xtrain = Xdata[0:7]
Xtest = Xdata[8:]

print(Xtrain.shape)
print(Xtest.shape)

# モデルの構築
model = models.Sequential()

print(Xdata.shape)
# エンコーダー部分
model.add(layers.InputLayer(input_shape=Xdata.shape[1]))
model.add(layers.Dense(128, activation="relu"))
model.add(layers.Dense(32, activation="relu"))

# デコーダー部分
model.add(layers.Dense(128, activation="relu"))
model.add(layers.Dense(Xdata.shape[1]))

# 学習開始
model.compile(optimizer="adam", loss="mse")

history = model.fit(Xtrain, Xtrain,
                    epochs=200,
                    batch_size=1,
                    shuffle=True,
                    validation_data=(Xtest, Xtest))

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

model.save("waveEncoder.h5")
print("max_length is", max_length)
