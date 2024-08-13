# vectorizers
import math
import os

import numpy
import numpy as np
import tensorflow
import tensorflow_similarity
from keras.layers import Input, Dense, Normalization, Flatten, BatchNormalization, Reshape, Dropout, Conv1D, LeakyReLU
from keras.models import Model
from keras import backend as K
from keras.saving import save_model
from tensorflow_similarity.layers import MetricEmbedding
from tensorflow.python import debug as tf_debug
from tensorflow.python.debug.lib.debug_data import has_inf_or_nan
import random
from sklearn.preprocessing import MinMaxScaler

import librosa

import conformer

print(tensorflow.keras.__version__)
print("Num GPUs Available: ", len(tensorflow.config.list_physical_devices('GPU')))

gpus = tensorflow.config.list_physical_devices('GPU')
if gpus:
    try:
        # Currently, memory growth needs to be the same across GPUs
        for gpu in gpus:
            tensorflow.config.experimental.set_memory_growth(gpu, True)
        logical_gpus = tensorflow.config.list_logical_devices('GPU')
        print(len(gpus), "Physical GPUs,", len(logical_gpus), "Logical GPUs")
    except RuntimeError as e:
        # Memory growth must be set before GPUs have been initialized
        print(e)


def cos_sim(v1, v2):
    return np.dot(v1, v2) / (np.linalg.norm(v1) * np.linalg.norm(v2))


def predictor(vectorizer_wave, vectorizer_motion):
    base_path = "/root/data/src/numpayDatasets"
    dataset_paths = []
    for i in range(1810):
        wave_path = os.path.join(base_path, f"{i}_music.npy")
        motion_path = os.path.join(base_path, f"{i}_motion.npy")
        dataset_paths.append((motion_path, wave_path))

    file_per = 0
    count = 10

    for i in range(count):
        print("----------------------------------------")
        answer = random.randint(600, 700)
        # answer = random.randint(0, 1500)
        motion_data_a = np.load(dataset_paths[answer][0])[900:930]
        wave_data_a = np.load(dataset_paths[answer][1])

        stft = np.abs(librosa.stft(wave_data_a[661500:683550], n_fft=512, hop_length=128)) ** 2
        log_stft = librosa.power_to_db(stft)
        x_dB = librosa.feature.melspectrogram(S=log_stft, n_mels=128)
        if x_dB.max() < abs(x_dB.min()):
            x_dB = x_dB / abs(x_dB.min())
        else:
            x_dB = x_dB / x_dB.max()

        print(motion_data_a.shape)
        if motion_data_a.shape[0] == 0:
            continue
        motion_data_a = motion_data_a.reshape((1, 30, 31, 7))
        print(x_dB.shape)
        wave_data_a = x_dB.reshape((1, 128, 173))

        answer_data_wave = vectorizer_wave(wave_data_a)
        answer_data_motion = vectorizer_motion(motion_data_a)

        answer_data_wave = tensorflow.math.l2_normalize(answer_data_wave, axis=1)
        answer_data_motion = tensorflow.math.l2_normalize(answer_data_motion, axis=1)
        answer_similarity = cos_sim(answer_data_motion[0], answer_data_wave[0])
        print(answer_similarity)

        for i1 in range(5):
            select = random.randint(600, 700)
            while answer == select:
                select = random.randint(600, 700)
            motion_data = np.load(dataset_paths[select][0])[:30]
            motion_data = motion_data.reshape((1, 30, 31, 7))

            incorrect_answer_motion = vectorizer_motion.predict(motion_data)
            incorrect_answer_motion = tensorflow.math.l2_normalize(incorrect_answer_motion, axis=1)

            incorrect_answer_similarity = cos_sim(incorrect_answer_motion[0], answer_data_wave[0])

            print(incorrect_answer_similarity)
            if answer_similarity < incorrect_answer_similarity:
                print("fail")
                file_per += 1
                break

    print("失敗率:", file_per / count * 100)


def motion():
    inputs = Input(shape=(30, 31, 7))
    x = Reshape((30, -1))(inputs)

    for i in range(32):
        x = conformer.attention(x, (30, 217), 8)
    x = Flatten()(x)
    x = MetricEmbedding(512)(x)
    motion = Model(inputs=inputs, outputs=x)
    motion.summary()
    return motion


def wave():
    inputs = Input(shape=(128, 173))
    x = BatchNormalization()(inputs)
    for i in range(16):
        # x = conformer.d1(x, (20, 44), 2, 44, 0)
        x = conformer.attention(x, (128, 173), 2)
    x = Flatten()(x)
    x = MetricEmbedding(512)(x)
    wave = Model(inputs=inputs, outputs=x)
    wave.summary()
    return wave


motion_optimizer = tensorflow.keras.optimizers.Adam(learning_rate=1e-5)
motion_encoder = motion()

wave_encoder = wave()
wave_optimizer = tensorflow.keras.optimizers.Adam(learning_rate=1e-5)

train_step_losses = []
train_epoch_losses = []

loss_fn = tensorflow_similarity.losses.MultiNegativesRankLoss(
    reduction=tensorflow.keras.losses.Reduction.SUM_OVER_BATCH_SIZE)


@tensorflow.function
def train_sys(second_data, motion_data, wave_data):
    with tensorflow.GradientTape() as motion_tape, tensorflow.GradientTape() as wave_taps:
        motion_embedding = motion_encoder(motion_data, training=True)
        wave_embedding = wave_encoder(wave_data, training=True)

        motion_embedding = tensorflow.math.l2_normalize(motion_embedding, axis=1)
        wave_embedding = tensorflow.math.l2_normalize(wave_embedding, axis=1)

        loss_value = loss_fn(wave_embedding, motion_embedding)

    motion_grads = motion_tape.gradient(loss_value, motion_encoder.trainable_weights)
    wave_grads = wave_taps.gradient(loss_value, wave_encoder.trainable_weights)

    motion_optimizer.apply_gradients(zip(motion_grads, motion_encoder.trainable_weights))
    wave_optimizer.apply_gradients(zip(wave_grads, wave_encoder.trainable_weights))

    return loss_value


nan = []

loaded_data_wave = []
loaded_data_motion = []
loaded_data_second = []
data_counter = 0
new_epoch = False


def nan_checker(array):
    """
    input ndarray
    '''
    return
    配列にNaNが含まれない   -> True
    配列にNaNが含まれる     -> False
    """
    if np.any(np.isnan(array)):
        return False
    else:
        return True


def request_data(dataset, batch):
    global data_counter, new_epoch

    if len(loaded_data_wave) != len(loaded_data_motion):
        print(len(loaded_data_wave))
        print(len(loaded_data_motion))
        exit(1)
    while batch > len(loaded_data_wave):
        print(f"data loading {data_counter}")
        zero_array = np.zeros((60, 31, 7))
        md = np.load(dataset[data_counter][0])
        md_backup = np.concatenate([md, zero_array])

        zero_array = np.zeros((44100,))
        wd = np.load(dataset[data_counter][1])
        wd_backup = np.concatenate([wd, zero_array])
        md = md_backup
        wd = wd_backup
        for i0 in range(4):
            for i in range(md.shape[0]):
                if (i + 1) % 30 == 0:
                    if i > 60 and md[i - 29:i + 1].max() == 0.0:
                        print(md.shape[0] / 30)
                        print(wd.shape[0] / 22050)
                        break

                    stft = np.abs(librosa.stft(wd[(i - 29) * 735:(i + 1) * 735], n_fft=512, hop_length=128)) ** 2
                    log_stft = librosa.power_to_db(stft)
                    x = librosa.feature.melspectrogram(S=log_stft, n_mels=128)
                    if abs(x.max()) < abs(x.min()):
                        x = x / abs(x.min())
                    else:
                        x = x / abs(x.max())

                    if not nan_checker(x) or not nan_checker(md):
                        continue
                    loaded_data_wave.append(x)
                    loaded_data_motion.append(md[i - 29:i + 1])
                    loaded_data_second.append((i + 1) / 30)

            # 水増し
            if i0 == 1:  # 音声シフト
                md = md[15:]
                wd = wd[11025:]
            if i0 == 2:  # ピッチ変更
                wd = librosa.effects.pitch_shift(y=wd_backup, sr=22050, n_steps=random.uniform(-12, 12),
                                                 bins_per_octave=12, res_type='kaiser_best')
                md = md_backup
            if i0 == 3:  # ノイズ付加
                wn = np.random.randn(len(wd_backup))
                wd = wd_backup + 0.005 * wn
                md = md_backup

        data_counter += 1
        if data_counter >= len(dataset):
            data_counter = 0
            new_epoch = True
            break

    batch_wave = []
    batch_motion = []
    batch_second = []

    total_batch = 0

    for i in range(batch):
        wd = loaded_data_wave.pop(0)
        md = loaded_data_motion.pop(0)
        bs = loaded_data_second.pop(0)
        batch_wave.append(wd)
        batch_motion.append(md)
        batch_second.append(bs)
        if len(loaded_data_wave) == 0:
            total_batch = i + 1
            break
    if total_batch == 0:
        total_batch = batch
    return np.array(batch_second), np.array(batch_motion), np.array(batch_wave), total_batch


def training(dataset, epochs, batch):
    global new_epoch
    for epoch in range(epochs):
        step = 0
        epoch_loss = 0

        while True:
            second_data, motion_data, wave_data, total_batch = request_data(dataset, batch)
            if not nan_checker(motion_data) or not nan_checker(wave_data):
                continue
            loss_value = train_sys(second_data, motion_data, wave_data)
            epoch_loss += float(loss_value)
            if step % 1 == 0:
                print(f"Training loss (for {total_batch} batch) at step {step + 1}: {float(loss_value):.4f}")
                print("Seen so far: %s samples" % ((step + 1) * total_batch))
            step += 1
            if new_epoch:
                new_epoch = False
                break

        print(f"Epoch loss {epoch}: {epoch_loss / step}")
        motion_encoder.save_weights("motion.weights.h5")
        wave_encoder.save_weights("wave.weights.h5")

        # save_model(motion_encoder, "vectorizer_motion.keras")
        # save_model(wave_encoder, "vectorizer_wave.keras")
        predictor(wave_encoder, motion_encoder)


if __name__ == "__main__":
    base_path = "/root/data/src/numpayDatasets"
    data = os.listdir(base_path)
    dataset_paths = []

    print("npy loading")
    for i in range(100000):
        wave_path = os.path.join(base_path, f"{i}_music.npy")
        motion_path = os.path.join(base_path, f"{i}_motion.npy")
        dataset_paths.append((motion_path, wave_path))

    print("finish")

    training(dataset_paths[1:1000], 100, 64)
