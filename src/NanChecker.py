import math
from tensorflow_similarity.layers import MetricEmbedding
import tensorflow
from tensorflow import keras
import os
import numpy as np
import random


vectorizer_motion = keras.models.load_model("../vectorizer_motion.h5")
vectorizer_wave = keras.models.load_model("../vectorizer_wave.h5")

base_path = "/root/data/D/src/numpayDatasets"
data = os.listdir(base_path)


nans = []
for i in range(1810):
    wave_path = os.path.join(base_path, f"{i}_music.npy")
    motion_path = os.path.join(base_path, f"{i}_motion.npy")

    motion_data = np.load(motion_path)
    wave_data = np.load(wave_path)

    motion_data = motion_data.reshape((1, 10800, 200, 7))
    wave_data = wave_data.reshape((1, 128, 15504))

    answer_data_wave = vectorizer_wave(wave_data)
    answer_data_motion = vectorizer_motion(motion_data)


    data_wave = tensorflow.math.l2_normalize(answer_data_wave, axis=1)
    data_motion = tensorflow.math.l2_normalize(answer_data_motion, axis=1)
    answer_similarity = tensorflow.reduce_sum(answer_data_motion * answer_data_wave, axis=1)

    if math.isnan(answer_similarity.numpy()[0]):
        nans.append(i)
        print(i, "isNan")
        print(data_wave)
        print(data_motion)

print(nans)