#from tensorflow_similarity.layers import MetricEmbedding
import numpy
import tensorflow
import os
import numpy as np
import random
from keras.models import load_model
import librosa
from conformer import PositionalEncoding, GLU
import main
import tensorflow_similarity as tfsm

vectorizer_wave = main.wave()
vectorizer_wave.load_weights("wave.weights.h5")
vectorizer_motion = main.motion()
vectorizer_motion.load_weights("motion.weights.h5")

loss_function = tfsm.losses.MultiNegativesRankLoss(reduction=tensorflow.keras.losses.Reduction.SUM_OVER_BATCH_SIZE)
random_num = [1000, 1314]

def cos_sim(v1_, v2_):
    # コサイン類似度の計算
    v1 = np.array(v1_)
    v2 = np.array(v2_)

    v1_batch = v1.shape[0]
    v2_batch = v2.shape[0]

    cut = v2_batch
    if v1_batch < cut:
        cut = v1_batch

    v1 = v1[:cut]
    v2 = v2[:cut]
    x = 0
    for i, (y, z) in enumerate(zip(v1, v2)):
        x += (np.dot(y, z) / (np.linalg.norm(y) * np.linalg.norm(z)))
        for i1, (x1, z1) in enumerate(zip(y, z)):
            if x1==z1:
                print(f"index {i1} is equal.({x1})")
    print(x/cut, loss_function(v1, v2))
    return x/cut


def recall_at_k(v1, v2, k=6):
    """
    It is the mean of ratio of correctly retrieved documents
    to the number of relevant documents.
    This implementation is specific to
    data having unique label for each key
    """

    v1_batch = v1.shape[0]
    v2_batch = v2.shape[0]

    cut = v2_batch
    if v1_batch < cut:
        cut = v1_batch
    v1 = numpy.array(v1[:cut-1])
    v2 = numpy.array(v2[:cut-1]).T

    print(v1.shape)
    print(v2.shape)

    sim_matrix = np.matmul(v1, v2)

    sorted_mat = np.argsort(sim_matrix, axis=1)[:, -k:]

    # Each key has unique label
    true_labels = np.arange(sorted_mat.shape[0]).reshape(-1, 1)
    true_labels = np.repeat(true_labels, k, axis=1)
    sorted_mat = sorted_mat - true_labels
    # the position in row corresponding to true positive
    # will be zero
    tps = np.any(sorted_mat == 0, axis=1)
    print(tps.mean())
    return tps.mean()


def data_processing_wave(path):
    return_array_data = []
    return_array_second_data = []

    cut = 22050

    zero_array = np.zeros((44100,))
    wave_data = np.load(path)
    wave_data = np.concatenate([wave_data, zero_array])
    for i in range(wave_data.shape[0]):
        if (i + 1) % cut == 0:
            if (wave_data[i - (cut - 1):i + 1]).max() == 0.0 and i > cut*20:
                print(str((i+1)/cut) + " data cut(wave)")
                break

            stft = np.abs(librosa.stft(wave_data[i - (cut - 1):i + 1], n_fft=512, hop_length=128))**2
            log_stft = librosa.power_to_db(stft)
            x = librosa.feature.melspectrogram(S=log_stft,n_mels=128)
            if x.max() < abs(x.min()):
                x = x/abs(x.min())
            else:
                x = x/x.max()

            return_array_data.append(x)
            return_array_second_data.append((i + 1) / cut)

    return numpy.array(return_array_data)#, numpy.array(return_array_second_data)


def data_processing_motion(path):
    return_array_data = []
    return_array_second_data = []

    cut = 30

    zero_array = np.zeros((60, 31, 7))
    motion_data = np.load(path)
    motion_data = np.concatenate([motion_data, zero_array])
    for i in range(motion_data.shape[0]):
        if (i + 1) % cut == 0:
            if motion_data[i - (cut-1):i + 1].max() == 0.0:
                print(str((i+1)/cut) + " data cut(motion)")
                break
            return_array_data.append(motion_data[i - (cut - 1):i + 1])
            return_array_second_data.append((i + 1) / cut)
    return np.array(return_array_data)#, np.array(return_array_second_data)

def main():
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
        answer = random.randint(random_num[0], random_num[1])
        while data_processing_motion(dataset_paths[answer][0]).shape[0] != data_processing_wave(dataset_paths[answer][1]).shape[0] or data_processing_wave(dataset_paths[answer][1]).shape[0] < 100:
            answer = random.randint(random_num[0], random_num[1])
            print("retrying...")

        print(f"ans num:{answer}")
        motion_data_a = data_processing_motion(dataset_paths[answer][0])
        wave_data_a = data_processing_wave(dataset_paths[answer][1])

        answer_data_wave = vectorizer_wave(wave_data_a)
        answer_data_motion = vectorizer_motion(motion_data_a)
        answer_similarity = cos_sim(answer_data_motion, answer_data_wave)

        for i1 in range(5):
            select = random.randint(random_num[0], random_num[1])
            while answer == select or data_processing_motion(dataset_paths[select][0]).shape[0] < 100:
                select = random.randint(random_num[0], random_num[1])
                print("retrying...")
            motion_data = data_processing_motion(dataset_paths[select][0])

            incorrect_answer_motion = vectorizer_motion.predict(motion_data)

            incorrect_answer_similarity = cos_sim(incorrect_answer_motion, answer_data_wave)

            # print(incorrect_answer_similarity)
            if answer_similarity < incorrect_answer_similarity:
                print("fail")
                file_per += 1
                break

    print("失敗率:", file_per / count * 100)


main()
