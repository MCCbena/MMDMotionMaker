import tensorflow as tf
from keras.layers import Dense, Input, Normalization, MultiHeadAttention, LayerNormalization, \
    BatchNormalization, Activation, Add, Conv2D, Conv1D, LeakyReLU, Dropout, Embedding, Masking
from keras.models import Model
from tensorflow import keras


class GLU(tf.keras.layers.Layer):
    def __init__(self,
                 name: str = "GLU",
                 **kwargs):
        super().__init__(name=name, **kwargs)

    def call(self, inputs, **kwargs):
        mat1, mat2 = tf.split(inputs, 2, axis=-1)
        mat2 = tf.nn.sigmoid(mat2)

        return tf.math.multiply(mat1, mat2)


class PositionalEncoding(tf.keras.layers.Layer):
    """
    Implements the sinusoidal positional encoding function
    Based on https://nlp.seas.harvard.edu/2018/04/03/attention.html#positional-encoding
    """
    def __init__(self,
                 d_model: int = 512,
                 name: str = "PositionalEncoding",
                 **kwargs):
        self.d_model = d_model

        super(PositionalEncoding, self).__init__(name=name, **kwargs)

    def build(self, input_shape):

        zeros = tf.zeros([256, 33, 128])
        self.pe_zeros = tf.Variable(zeros, shape=[None, 33, 128])
        pass

    def encode(self, max_len, d_model, pe, second):
        pos = tf.range(0, max_len, 1.0)
        pos = tf.expand_dims(pos, 0)
        pos = tf.tile(pos, [tf.shape(second)[0], 1])
        pos = pos + second
        pos = tf.expand_dims(pos, 2)
        pos = tf.repeat(pos, 64, 2)

        i = tf.range(0, d_model/2, 1)
        i = tf.cast(i, tf.float32)

        div_term = pos / 10000**(2*i/d_model)

        zeros = self.pe_zeros
        batch = tf.shape(second)[0]
        zeros[:batch, :, 0::2].assign(tf.math.sin(div_term))
        zeros[:batch, :, 1::2].assign(tf.math.cos(div_term))
        return zeros[:batch]

    def call(self, inputs, **kwargs):
        value, second = inputs

        max_len, d_model = tf.shape(value)[-2], tf.shape(value)[-1]
        pe = self.encode(max_len, self.d_model, value, second*128)
        outputs = tf.math.add(value, pe)

        return outputs


def ffm(inputs, unit, dropout_rate):

    middle_layer = LayerNormalization()(inputs)
    middle_layer = Dense(unit)(middle_layer)
    middle_layer = Activation(activation="silu")(middle_layer)

    middle_layer = Dense(unit)(middle_layer)
    output_layer = Add()([middle_layer, inputs])

    return output_layer


def d2(input_dim, head_size, filters, kernel_size=3, expansion_factor=2):
    inputs = Input(shape=input_dim)

    ffm_module = ffm(input_dim, 7, 0)
    #    positinal_encoding = PositionalEncoding(7)

    x_2 = ffm_module(inputs)
    # attention module
    x_2 = Add()([inputs, x_2])
    x_2 = LayerNormalization(epsilon=1e-6)(x_2)
    #    x_2 = positinal_encoding(x_2)
    x_3 = MultiHeadAttention(num_heads=6, key_dim=head_size)(x_2, x_2)
    x_3 = Dropout(0.5)(x_3)

    # CNN Module
    x_1 = Add()([x_2, x_3])
    x_1 = LayerNormalization(epsilon=1e-6)(x_1)
    x_2 = Conv2D(filters * expansion_factor, 1)(x_1)
    # Glu
    # x_2 = GLU()(x_2)

    x_2 = Conv2D(filters=filters, kernel_size=kernel_size, padding="same", groups=filters)(x_2)
    x_2 = BatchNormalization()(x_2)
    x_2 = Activation(activation="silu")(x_2)
    x_2 = Conv2D(filters, 1)(x_2)

    x_1 = MultiHeadAttention(num_heads=6, key_dim=head_size)(x_2, x_2, x_2)
    x_2 = Add()([x_1, inputs])
    x_2 = LayerNormalization()(x_2)
    x_1 = Add()([x_2, x_1])

    x_3 = ffm_module(x_1)
    x_3 = Add()([x_1, x_3])
    x = LayerNormalization()(x_3)

    return Model(inputs=inputs, outputs=x)


def d1(inputs, input_dim, head_size, filters, dropout_rate=0.5, kernel_size=3, expansion_factor=2):

    x_2 = ffm(inputs, input_dim[1], dropout_rate)
    # attention module

    x_2 = Add()([inputs, x_2])
    x_2 = LayerNormalization(epsilon=1e-6)(x_2)
    x_3 = MultiHeadAttention(num_heads=6, key_dim=head_size)(x_2, x_2)

    # CNN Module
    x_1 = Add()([x_2, x_3])
    x_1 = LayerNormalization(epsilon=1e-6)(x_1)
    x_2 = Conv1D(filters * expansion_factor, 1)(x_2)
    # Glu
    # x_2 = GLU()(x_2)

    x_2 = Conv1D(filters=filters, kernel_size=kernel_size, padding="same", groups=filters)(x_2)
    x_2 = BatchNormalization()(x_2)
    x_2 = Activation(activation="silu")(x_2)
    x_2 = Conv1D(filters, 1)(x_2)

    x_1 = Add()([x_2, x_1])

    x_2 = ffm(inputs, input_dim[1], dropout_rate)
    x = Add()([x_2, x_1])
    x = BatchNormalization()(x)

    return x


def position_wise_ffm(inputs, input_dim, d_ff):
    x_1 = Dense(d_ff, activation="relu")(inputs)
    x_1 = Dense(input_dim[-1])(x_1)

    return x_1

def attention(inputs, input_dim, head_size):
    x_1 = MultiHeadAttention(num_heads=6, key_dim=head_size)(inputs, inputs, inputs)
    x_2 = Add()([x_1, inputs])
    x_2 = BatchNormalization()(x_2)
    x_3 = position_wise_ffm(x_2, input_dim, 128)
    # x_3 = Dropout(0.1)(x_3)
    x_3 = Add()([x_2, x_3])
    x = BatchNormalization()(x_3)
    return x
