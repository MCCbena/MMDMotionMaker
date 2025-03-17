from typing import overload, Type


def loadVMD(motionPath: str, enableFrameInterpolation=False) -> PyMotion:
    """VMDファイルからモーションを読み込みます。
    :param motionPath: モーションのパス
    :param enableFrameInterpolation: フレーム補完を有効にするか
    """

def writeVMD(motionData:PyMotion, motionPath:str):
    """
    VMDファイルへモーションデータを書き込みます。

    :param motionData: 書き込むモーションのデータ
    :param motionPath: 書き込むモーションのパス
    """

def encodeMotion(motionData:PyMotion, model:PyModel) -> PyEncodeMotion:
    """モーションデータをAIが認識可能なデータへ変換します。
    :param motionData: エンコードするモーションのデータ
    :param model: 基準となるモデル
    """

def decodeMotion(encodeMotion:PyEncodeMotion, model:PyModel, needBones:list[str]) -> PyMotion:
    """エンコードされたモーションをvmd形式にデコードします。
    :param encodeMotion: デコードするモーション
    :param model: エンコードした際に使った基準となるモデル（必ず同じモデルを使用してください）
    :param needBones: デコードしたPyMotionに代入するボーン（髪1やスカート1などの不要なボーンを含まないようにするためのものです）shift-jisでエンコードしてください。
    """

@overload
def createEncodeMotion(bones:list[bytes]) -> PyEncodeMotion:
    """
    EncodeMotionを新しく作成します。ボーンフレームは内部的に2次元配列で管理されます。
    :param bones: [フレーム番号][ボーン番号]という配列の内[ボーン番号]の部分のボーンです。渡されたリストのインデックスと対応します。
    """
@overload
def createEncodeMotion(model:PyModel) -> PyEncodeMotion:
    """
    EncodeMotionを新しく作成します。ボーンフレームは内部的に2次元配列で管理されます。
    :param model: モデルからボーン名を参照し、PyEncodeMotionを構築します。[フレーム番号][ボーン番号]という配列の内[ボーン番号]の部分のボーンです。
    """

class PyMotion:
    """モーションデータを格納するクラスです。"""
    def getBoneFrame(self) -> list[PyBoneFrame]:
        """ボーンフレームを取得します。"""

class PyBoneFrame:
    """ボーンフレームを格納するクラスです。"""
    def __init__(self):
        self.name = Type[bytes]
        """ボーンの名前が保存されています。上限15バイト、SHIFT-JISでエンコードされた状態です。"""
        self.frame = int
        """ボーンが何フレーム目のものであるかを示すフレーム番号が格納されます。0から始まります。"""

        self.x = float
        """ボーンがどれだけ平行移動したかを表すx座標です。"""
        self.y = float
        """ボーンがどれだけ平行移動したかを表すy座標です。"""
        self.z = float
        """ボーンがどれだけ平行移動したかを表すz座標です。"""

        self.qx = float
        """ボーンの回転を表すクォータニオンのx軸です。"""
        self.qy = float
        """ボーンの回転を表すクォータニオンのy軸です。"""
        self.qz = float
        """ボーンの回転を表すクォータニオンのz軸です。"""
        self.qw = float
        """ボーンの回転を表すクォータニオンのw軸です。"""

        self.bezier = [int, ...]
        """ベジェ曲線の値が代入されます。0~127までの値が有効なサイズ64固定の配列です。"""

class PyModel:
    """
    pmx形式のモデルを読み込むモデルローダーです。基本的に書き込み・読み取り不可です。
    """
    def __init__(self, modelPath:str):
        """
        :param modelPath: モデルのパス
        """

class PyEncodeMotion:
    """
    エンコードされたモーションを格納するクラスです。
    """
    @overload
    def getBoneFrame(self) -> list[list[PyEncodeBoneFrame]]:
        """
        全てのボーンフレームを取得します。
        2次元配列として返され、[フレーム番号][ボーン番号]というインデックスで保存されています。
        """
    @overload
    def getBoneFrame(self, frame:int) -> list[PyEncodeBoneFrame]:
        """
        指定したフレーム番号に含まれるボーンフレームを取得します。
        1次元配列として返され、[フレーム番号]というインデクスで保存されています。
        :param frame: フレーム番号
        """
    @overload
    def getBoneFrame(self, frame:int, bone:int) -> PyEncodeBoneFrame:
        """
        指定したボーンフレームを取得します。
        :param frame:フレーム番号
        :param bone: ボーン番号
        """

    def setBoneFrame(self, frame:int, bone:int, boneframe:PyEncodeBoneFrame):
        """
        指定したフレーム・ボーン番号にPyEncodeBoneFrameを代入します。
        :param boneframe: 代入するボーンフレーム
        :param frame:フレーム番号
        :param bone: ボーン番号
        """
    def getNameIndex(self) -> list[bytes]:
        """
        ボーンの名前のリストを取得します。返されるリストのインデックスとエンコードモーションデータ内部のインデックスは同じです。
        """

class PyEncodeBoneFrame:
    """エンコードされたボーンフレームを格納するクラスです。"""
    def __init__(self):
        self.x = float
        """ボーンの位置を絶対座標で表すx座標です。"""
        self.y = float
        """ボーンの位置を絶対座標で表すy座標です。"""
        self.z = float
        """ボーンの位置を絶対座標で表すz座標です。"""

        self.qx = float
        """現在の絶対的な回転を表すクォータニオンのx軸です。"""
        self.qy = float
        """現在の絶対的な回転を表すクォータニオンのy軸です。"""
        self.qz = float
        """現在の絶対的な回転を表すクォータニオンのz軸です。"""
        self.qw = float
        """現在の絶対的な回転を表すクォータニオンのw軸です。"""