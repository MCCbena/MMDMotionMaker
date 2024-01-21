import librosa
import numpy as np
import soundfile
import tensorflow
from tensorflow.keras import models, layers
import os

print(tensorflow.test.gpu_device_name())

dataset_path = "music/"
max_length = 0
librosa_loads = []

Xdata = []

for path in os.listdir(dataset_path):
    y, sr = librosa.load(dataset_path + path)
    librosa_loads.append([y, sr])
    # 一番でかい数値を代入
    if len(y) > max_length:
        max_length = len(y)

for i in librosa_loads:
    y, sr = i

    shape_length = len(y)
    zero_array = np.zeros(max_length - shape_length)
    y = np.concatenate((y, zero_array))

    x = librosa.feature.melspectrogram(y=y, sr=sr, hop_length=1024 * 2, win_length=1024 * 2)
    Xdata.append(x / 32768)

x = np.array(Xdata)
Xdata = x.reshape((len(x), np.prod(x.shape[1:])))

# 学習とテストデータに分類
Xtrain = Xdata[0:7]
Xtest = Xdata[8:]

print(Xtrain.shape)
print(Xtest.shape)

print("Num GPUs Available: ", len(tensorflow.config.list_physical_devices('GPU')))

# モデルの構築
model = models.Sequential()

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

y, sr = librosa.load("【初音ミク】 ウミユリ海底譚 【オリジナル曲】.wav")
shape_length = len(y)
zero_array = np.zeros(max_length - shape_length)
y = np.concatenate((y, zero_array))

Xdata = []
x = librosa.feature.melspectrogram(y=y, sr=sr, hop_length=1024 * 2, win_length=1024 * 2)
Xdata.append(x / 32768)

x = np.array(Xdata)
print(x.shape)
Xdata = x.reshape((len(x), np.prod(x.shape[1:])))
print(Xdata.shape)
encode = model.predict(Xdata)
Ydata = model.predict(encode) * 32768
Ydata = Ydata.reshape((1, 128, -1))
Ydata = Ydata[0]
y_reconstructed = librosa.feature.inverse.mel_to_audio(Ydata, sr=sr, hop_length=1024 * 2, win_length=1024 * 2)
soundfile.write("test.wav", y_reconstructed, samplerate=sr)