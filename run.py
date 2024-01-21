import librosa
import numpy as np
from tensorflow import keras
import soundfile

hop_length = 1024 * 2  # 生成したAIで設定した値を入力
model_path = "waveEncoder.h5"  # モデルのパスまたはファイル名

audio_path = "" # 変換する曲名
y, sr = librosa.load(audio_path)

# モデルをロード
model = keras.models.load_model(model_path)

max_length = 7034625  # 自作したモデルでなければ、書き換えてはいけない。
shape_length = len(y)
zero_array = np.zeros(max_length - shape_length)
y = np.concatenate((y, zero_array))

Xdata = []
x = librosa.feature.melspectrogram(y=y, sr=sr, hop_length=hop_length, win_length=1024 * 2)
Xdata.append(x / 32768)

x = np.array(Xdata)
print(x.shape)
Xdata = x.reshape((len(x), np.prod(x.shape[1:])))
print(Xdata.shape)
encode = model.predict(Xdata)
Ydata = model.predict(encode) * 32768
Ydata = Ydata.reshape((1, 128, -1))
Ydata = Ydata[0]
y_reconstructed = librosa.feature.inverse.mel_to_audio(Ydata, sr=sr, hop_length=hop_length, win_length=1024 * 2)
soundfile.write("output.wav", y_reconstructed, samplerate=sr)
