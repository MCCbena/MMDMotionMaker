import librosa
import numpy as np
from tensorflow import keras
import soundfile
from sklearn.metrics import mean_squared_error

hop_length = 1024 * 2  # 生成したAIで設定した値を入力
model_path = "waveEncoder.h5"  # モデルのパスまたはファイル名

audio_path = "music/DECO＊27 - 愛言葉Ⅳ feat. 初音ミク.wav" # 変換する曲名
y, sr = librosa.load(audio_path)

# モデルをロード
model = keras.models.load_model(model_path)

max_length = 7034625  # 自作したモデルでなければ、書き換えてはいけない。
shape_length = len(y)
zero_array = np.zeros(max_length - shape_length)
y = np.concatenate((y, zero_array))

Xdata = []
x = librosa.feature.mfcc(y=y, sr=sr, n_mfcc=13)
Xdata.append(x / 32768)

x = np.array(Xdata)
print(x.shape)
Xdata = x.reshape((len(x), np.prod(x.shape[1:])))
print(Xdata.shape)
encode = model.predict(Xdata)
Ydata = model.predict(encode)

mse_data = mean_squared_error(Xdata, Ydata)

print("MSE値:", mse_data)