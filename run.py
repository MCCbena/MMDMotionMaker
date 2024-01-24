import librosa
import numpy as np
from tensorflow import keras
from sklearn.metrics import mean_squared_error

hop_length = 1024 * 2  # 生成したAIで設定した値を入力
model_path = "waveEncoder.h5"  # モデルのパスまたはファイル名

audio_path = "【初音ミク】 ウミユリ海底譚 【オリジナル曲】.wav"  # 変換する曲名
y, sr = librosa.load(audio_path)

# モデルをロード
model = keras.models.load_model(model_path)

max_length = 7034626  # 自作したモデルでなければ、書き換えてはいけない。
shape_length = len(y)
zero_array = np.zeros(max_length - shape_length)
y = np.concatenate((y, zero_array))

Xdata = []
x = librosa.feature.melspectrogram(y=y, sr=sr, hop_length=hop_length, win_length=512)
# dB単位に変換
x_dB = librosa.power_to_db(x, ref=np.max)
Xdata.append(x_dB / 80)  # データを1.0から0の間にする

x = np.array(Xdata)
print(x.shape)
Xdata = x.reshape((len(x), np.prod(x.shape[1:])))
print(Xdata.shape)
encode = model.predict(Xdata)
print(encode.shape)
Ydata = model.predict(encode)

mse_data = mean_squared_error(Xdata, Ydata)

print("MSE値:", mse_data)
