#include <python3.11/Python.h>
#include <stddef.h>
#include <python3.11/structmember.h>
#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント
#include "vmd/indexlib.h"
#include "vmd/vmdStruct.h"
#include "pmx/pmxStruct.h"
#include "vmd/Rotation.h"
#include <stdbool.h>

extern MotionData getMotion(const char*, bool);
extern void getModel(const char *, struct Model *);
extern EncodeMotionData motionEncoder(struct Model model, MotionData *motionData);
extern MotionData motionDecoder(EncodeMotionData encodeMotionData, struct Model model, struct Index need_bone);
extern void writeMotion(const char*, MotionData);

extern struct Quaternion SphericalLinearInterpolation(struct Quaternion q1, struct Quaternion q2, const long double t);
extern long double getPos(long double pos1, long double pos2, long double time);


//Dictを用いないdirectパッケージの実装------------------------------------------------------------------------------------------
typedef struct {
    PyObject_HEAD
    MotionData motionData;
}PyMotion;

static PyMethodDef PyMotionMethods[];
static PyTypeObject PyMotionType;
static void PyMotionDealloc(PyMotion* self);
static PyObject* getPyBoneFrame_wrapper(PyMotion* self);
static PyObject* loadVMD_wrapper(PyObject* self, PyObject* args);
static PyObject* writeVMD_wrapper(PyObject* self, PyObject* args);
static PyObject* decodeMotion_wrapper(PyObject* self, PyObject* args);

typedef struct {
    PyObject_HEAD
    PyObject *name; //String 15バイト
    int frame; //int

    double x; //float
    double y; //float
    double z; //float

    double qx; //float
    double qy; //float
    double qz; //float
    double qw; //float

    PyObject *bezier; //64バイトのリストchar
}PyBoneFrame;

static PyMemberDef PyBoneFrameMembers[];
static PyTypeObject PyBoneFrameType;

typedef struct {
    PyObject_HEAD
    struct Model model;
}PyModel;

static PyTypeObject PyEncodeMotionType;
static PyMethodDef pyEncodeMotionMethods[];
static PyObject* PyModelNew(PyTypeObject *type, PyObject* args, PyObject *kwds);
static void PyModelDealloc(PyModel* self);

typedef struct {
    PyObject_HEAD
    EncodeMotionData encodeMotionData;
}PyEncodeMotion;

static PyTypeObject PyModelType;
static PyMethodDef PyDirectMethods[];
static PyObject* PyEncodeMotionNew(PyObject *self, PyObject *args);
static void PyEncodeMotionDealloc(PyEncodeMotion* self);
static PyObject* getEncodeBoneFrame(PyEncodeMotion* self, PyObject* args);
static PyObject* setEncodeBoneFrame(PyEncodeMotion* self, PyObject* args);
static PyObject* encodeMotion_wrapper(PyObject* self, PyObject* args);
static PyObject* getIndexFromBoneName(PyEncodeMotion* self, PyObject* args);

static PyObject* getNameIndexer(PyEncodeMotion* self, PyObject* args);

typedef struct {
    PyObject_HEAD
    double x; //float
    double y; //float
    double z; //float

    double qx; //float
    double qy; //float
    double qz; //float
    double qw; //float
} PyEncodeBoneFrame;

#define FrameExpansionRate 512 //PyEncodeBoneFrameの配列がメモリ不足に陥った際、なんフレーム分のメモリを確保するかを決定する

static PyMemberDef PyEncodeBoneFrameMembers[];
static void PyEncodeBoneFrameDealloc(PyEncodeBoneFrame* self);
static PyTypeObject PyEncodeBoneFrameType;
static PyObject* moveComplement(PyObject* self, PyObject* args);


//PyMotionの実装--------------------------------------------------
static void PyMotionDealloc(PyMotion* self){
    free(self->motionData.boneFrame);
    self->motionData.boneFrame = NULL;
    Py_TYPE(self)->tp_free((PyObject*) self);
}

static PyObject* getPyBoneFrame_wrapper(PyMotion* self){
    PyObject* pyBoneFrames = PyList_New(0);

    for (int i = 0; i <= self->motionData.maxFrame.maxFrame; ++i){
        PyBoneFrame* pyBoneFrame = (PyBoneFrame*)PyObject_CallObject((PyObject*)&PyBoneFrameType, NULL);
        pyBoneFrame->name = PyBytes_FromStringAndSize(self->motionData.boneFrame[i].name, 15);

        pyBoneFrame->frame = self->motionData.boneFrame[i].frame;

        pyBoneFrame->x = self->motionData.boneFrame[i].x;
        pyBoneFrame->y = self->motionData.boneFrame[i].x;
        pyBoneFrame->z = self->motionData.boneFrame[i].x;

        pyBoneFrame->qx = self->motionData.boneFrame[i].qx;
        pyBoneFrame->qy = self->motionData.boneFrame[i].qy;
        pyBoneFrame->qz = self->motionData.boneFrame[i].qz;
        pyBoneFrame->qw = self->motionData.boneFrame[i].qw;

        PyObject* bezier = PyList_New(0);
        for (int j = 0; j < 64; ++j) {
            PyList_Append(bezier, PyLong_FromLong(self->motionData.boneFrame[i].bezier[j]));
        }
        pyBoneFrame->bezier = bezier;

        PyList_Append(pyBoneFrames, (PyObject*)pyBoneFrame);
    }

    return pyBoneFrames;
}

static PyMethodDef PyMotionMethods[] = {
        {"getBoneFrame", (PyCFunction)getPyBoneFrame_wrapper, METH_NOARGS, "ボーンフレームをListとして取得できます。"},
        {NULL}
};

static PyTypeObject PyMotionType = {
        .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "VMDConverter.PyMotion",
        .tp_doc = "Python motion object",
        .tp_basicsize = sizeof(PyMotion),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = PyType_GenericNew,
        .tp_methods = PyMotionMethods,
        .tp_dealloc = (destructor) PyMotionDealloc,
};

//VMDからファイルを読み込み関数のwrapper
static PyObject* loadVMD_wrapper(PyObject* self, PyObject* args){
    char *motionPath;
    int frameInterpolation = 0;

    //引数１つ目はvmdファイルのパス、２つ目はフレーム補完を有効にするか
    if(!PyArg_ParseTuple(args, "s|p", &motionPath, &frameInterpolation)) return NULL;

    PyMotion* pyMotion = (PyMotion *) PyObject_CallObject((PyObject *) &PyMotionType, NULL);
    MotionData motionData = getMotion(motionPath, frameInterpolation);
    pyMotion->motionData = motionData;

    return (PyObject*) pyMotion;
}

//VMDにファイルを書き込み関数のwrapper
static PyObject* writeVMD_wrapper(PyObject* self, PyObject* args){
    char *motionPath;
    PyMotion* pyMotion;

    //引数１つ目はPyMotion、２つ目に書き込むファイルのパス
    if(!PyArg_ParseTuple(args, "Os", &pyMotion, &motionPath)) return NULL;

    writeMotion(motionPath, pyMotion->motionData);

    Py_INCREF(Py_None);
    return Py_None;
}

//エンコードされたモーションデータをデコードする関数のwrapper
static PyObject* decodeMotion_wrapper(PyObject* self, PyObject* args){
    PyEncodeMotion *pyEncodeMotion;
    PyModel *pyModel;
    PyObject *needBoneList;


    //引数１つ目はPyMotion、２つ目は基準モデル（エンコードしたときに使ったモデルを使うこと）、3つ目は必要なボーンを一覧のリスト
    if(!PyArg_ParseTuple(args, "OOO", &pyEncodeMotion, &pyModel, &needBoneList)) return NULL;

    PyMotion* pyMotion = (PyMotion*) PyObject_CallObject((PyObject*)&PyMotionType, NULL);

    struct Index needBoneIndex = makeIndex(PyList_GET_SIZE(needBoneList), 15);
    for (int i = 0; i < PyList_GET_SIZE(needBoneList); ++i) {
        char* boneName = PyBytes_AsString(PyList_GetItem(needBoneList, i));
        if(getnIndex(needBoneIndex, boneName, 15) == -1){
            addIndex(&needBoneIndex, boneName, 15);
        }
    }
    pyMotion->motionData = motionDecoder(pyEncodeMotion->encodeMotionData, pyModel->model, needBoneIndex);
    destroy_index(&needBoneIndex);

    return (PyObject*) pyMotion;
}

//PyBoneFrameに関する実装----------------------------------------------------------------------
void PyBoneFrameDealloc(PyBoneFrame* self){
    Py_XDECREF(self->name);
    Py_XDECREF(self->bezier);
    Py_TYPE(self)->tp_free((PyObject*) self);
}

static PyMemberDef PyBoneFrameMembers[] = {
        {"name", T_OBJECT, offsetof(PyBoneFrame, name), 0, "ボーン名(SHIFT-JIS・先頭から15バイトのみが有効です。)"},
        {"frame", T_INT, offsetof(PyBoneFrame, frame), 0, "フレーム番号"},
        {"x", T_DOUBLE, offsetof(PyBoneFrame, x), 0, "x座標"},
        {"y", T_DOUBLE, offsetof(PyBoneFrame, y), 0, "y座標"},
        {"z", T_DOUBLE, offsetof(PyBoneFrame, z), 0, "z座標"},
        {"qx", T_DOUBLE, offsetof(PyBoneFrame, qx), 0, "クォータニオンx座標"},
        {"qy", T_DOUBLE, offsetof(PyBoneFrame, qy), 0, "クォータニオンy座標"},
        {"qz", T_DOUBLE, offsetof(PyBoneFrame, qz), 0, "クォータニオンz座標"},
        {"qw", T_DOUBLE, offsetof(PyBoneFrame, qw), 0, "クォータニオンw座標"},
        {"bezier", T_OBJECT, offsetof(PyBoneFrame, bezier), 0, "ベジェ曲線"},
        {NULL}
};

static PyTypeObject PyBoneFrameType = {
        .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "VMDConverter.PyBoneFrame",
        .tp_doc = "Python boneframe object",
        .tp_basicsize = sizeof(PyBoneFrame),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = PyType_GenericNew,
        .tp_members = PyBoneFrameMembers,
        .tp_dealloc = (destructor) PyBoneFrameDealloc
};

//PyModelに関する実装------------------------------------------------------------------
//PyModelのインスタンス化コード
static PyObject* PyModelNew(PyTypeObject *type, PyObject* args, PyObject *kwds){
    char *modelPath;
    //モデルのパスが引数
    if(!PyArg_ParseTuple(args, "s", &modelPath)) return NULL;

    PyModel* self = (PyModel*)type->tp_alloc(type, 0);
    getModel(modelPath, &self->model);

    return (PyObject*) self;
}

//PyModelのfree
static void PyModelDealloc(PyModel* self){
    struct Model model = self->model;

    for (int i = 0; i < model.bone_size; i++) free(model.bone[i].child_bones);
    free(model.bone);
    free(model.texture);
    free(model.surface);
    free(model.topData);
    free(model.material);

    Py_TYPE(self)->tp_free((PyObject*) self);
}


static PyTypeObject PyModelType = {
        .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "VMDConverter.PyModel",
        .tp_doc = "Python model object",
        .tp_basicsize = sizeof(PyModel),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = PyModelNew,
        .tp_dealloc = (destructor) PyModelDealloc,
};

//PyEncodeMotionに関する実装------------------------------------------------------------
static PyObject* PyEncodeMotionNew(PyObject *self, PyObject *args){
    PyEncodeMotion* pyEncodeMotion = (PyEncodeMotion*) PyObject_CallObject((PyObject*)&PyEncodeMotionType, NULL);
    PyObject* nameIndexerList;
    PyObject* inputObject;

    //引数1にモデルの名前をリストにして入れたものを代入（bytes）
    if(!PyArg_ParseTuple(args, "O", &inputObject)){
        printf("error\n");
        return NULL;
    }

    //入力された引数がListかmodelかを判定
    if(Py_IS_TYPE(inputObject, &PyList_Type)){
        nameIndexerList = inputObject;
    }else if(Py_IS_TYPE(inputObject, &PyModelType)){
        PyModel *pyModel = (PyModel*)inputObject;
        nameIndexerList = PyList_New(pyModel->model.bone_size);
        for (int i = 0; i < pyModel->model.bone_size; ++i){
            char* temp_name = word_decode(pyModel->model.bone[i].model_name_jp.byte, pyModel->model.bone[i].model_name_jp.byte_size, "SHIFT-JIS", pyModel->model.header.encode==0 ? "UTF-16":"UTF-8");
            //モデルのボーンをnameIndexerに追加
            PyList_SET_ITEM(nameIndexerList, i,
                          PyBytes_FromString(
                                  temp_name));
            free(temp_name);
        }
    }else{
        PyErr_SetString(PyExc_TypeError, "arg is not support type. support type is List or PyModel.");
        return NULL;
    }

    pyEncodeMotion->encodeMotionData.nameIndexer_size = PyList_GET_SIZE(nameIndexerList);
    pyEncodeMotion->encodeMotionData.encodeBoneFrame_size = 0;
    for (int i = 0; i < PyList_GET_SIZE(nameIndexerList); ++i){
        pyEncodeMotion->encodeMotionData.nameIndexer[i].index = i;
        pyEncodeMotion->encodeMotionData.nameIndexer[i].name_byte = PyBytes_GET_SIZE(PyList_GetItem(nameIndexerList, i)); //渡された文字バイトの長さを代入
        memcpy(pyEncodeMotion->encodeMotionData.nameIndexer[i].name, PyBytes_AsString(PyList_GetItem(nameIndexerList, i)),
               PyBytes_GET_SIZE(PyList_GetItem(nameIndexerList, i)));
    }

    pyEncodeMotion->encodeMotionData.encodeBoneFrame = calloc(sizeof(struct EncodeBoneFrame), FrameExpansionRate);
    for (int i = 0; i < FrameExpansionRate; ++i) {
        pyEncodeMotion->encodeMotionData.encodeBoneFrame[i] = calloc(sizeof(struct EncodeBoneFrame), pyEncodeMotion->encodeMotionData.nameIndexer_size);
    }
    pyEncodeMotion->encodeMotionData.encodeBoneFrame[FrameExpansionRate] = NULL;

    return (PyObject*)pyEncodeMotion;
}


static void PyEncodeMotionDealloc(PyEncodeMotion* self){
    EncodeMotionData encodeMotionData = self->encodeMotionData;

    for (int i = 0; encodeMotionData.encodeBoneFrame[i] != NULL; ++i) {
        free(encodeMotionData.encodeBoneFrame[i]);
    }

    free(encodeMotionData.encodeBoneFrame);
    encodeMotionData.encodeBoneFrame = NULL;

    Py_TYPE(self)->tp_free((PyObject*)self);
}

//MotionDataをエンコードする関数のwrapper
static PyObject* encodeMotion_wrapper(PyObject* self, PyObject* args){
    PyMotion* pyMotion;
    PyModel* pyModel;

    //1番目にPyMotion、2番目にPyModel
    if(!PyArg_ParseTuple(args, "OO", &pyMotion, &pyModel)) return NULL;

    EncodeMotionData encode = motionEncoder(pyModel->model, &pyMotion->motionData);
    PyEncodeMotion* pyEncodeMotion = (PyEncodeMotion*)PyObject_CallObject((PyObject*)&PyEncodeMotionType, NULL);

    pyEncodeMotion->encodeMotionData = encode;

    return (PyObject*) pyEncodeMotion;
}

//ボーン名からPyBoneFrameのインデックスを取得します。
static PyObject* getIndexFromBoneName(PyEncodeMotion* self, PyObject* args){
    PyObject *pyBoneName;
    char* boneName;

    if(!PyArg_ParseTuple(args, "S", &pyBoneName)) return NULL;
    boneName = PyBytes_AsString(pyBoneName);

    int boneNameSize = (int)strlen(boneName);
    for (int i = 0; i < self->encodeMotionData.nameIndexer_size; ++i){
        int size = self->encodeMotionData.nameIndexer[i].name_byte;
        if(size < boneNameSize) size = boneNameSize;
        if(strncmp(boneName, self->encodeMotionData.nameIndexer[i].name, size) == 0){
            return PyLong_FromLong(self->encodeMotionData.nameIndexer[i].index);
        }
    }

    PyErr_SetString(PyExc_IndexError, "bone name not found");
    return NULL;
}

static PyObject* getEncodeBoneFrame(PyEncodeMotion* self, PyObject* args) {
    int frame=-1, bone=-1;

    //1番目にフレーム番号、二番目にボーン番号
    if (!PyArg_ParseTuple(args, "|ii", &frame, &bone)) return NULL;


    if(frame!=-1 && bone!=-1) {
        PyEncodeBoneFrame *encodeBoneFrame = (PyEncodeBoneFrame *) PyObject_CallObject((PyObject *) &PyEncodeBoneFrameType,
                                                                                       NULL);
        encodeBoneFrame->x = (double) self->encodeMotionData.encodeBoneFrame[frame][bone].x;
        encodeBoneFrame->y = (double) self->encodeMotionData.encodeBoneFrame[frame][bone].y;
        encodeBoneFrame->z = (double) self->encodeMotionData.encodeBoneFrame[frame][bone].z;

        encodeBoneFrame->qx = (double) self->encodeMotionData.encodeBoneFrame[frame][bone].qx;
        encodeBoneFrame->qy = (double) self->encodeMotionData.encodeBoneFrame[frame][bone].qy;
        encodeBoneFrame->qz = (double) self->encodeMotionData.encodeBoneFrame[frame][bone].qz;
        encodeBoneFrame->qw = (double) self->encodeMotionData.encodeBoneFrame[frame][bone].qw;

        return (PyObject*) encodeBoneFrame;
    }else if(bone==-1 && frame!=-1){
        PyObject *encodeBoneFrameList = PyList_New(0);
        for (int i = 0; i < self->encodeMotionData.nameIndexer_size; ++i) {
            PyEncodeBoneFrame *encodeBoneFrame = (PyEncodeBoneFrame *) PyObject_CallObject((PyObject *) &PyEncodeBoneFrameType,
                                                                                           NULL);
            encodeBoneFrame->x = (double) self->encodeMotionData.encodeBoneFrame[frame][i].x;
            encodeBoneFrame->y = (double) self->encodeMotionData.encodeBoneFrame[frame][i].y;
            encodeBoneFrame->z = (double) self->encodeMotionData.encodeBoneFrame[frame][i].z;

            encodeBoneFrame->qx = (double) self->encodeMotionData.encodeBoneFrame[frame][i].qx;
            encodeBoneFrame->qy = (double) self->encodeMotionData.encodeBoneFrame[frame][i].qy;
            encodeBoneFrame->qz = (double) self->encodeMotionData.encodeBoneFrame[frame][i].qz;
            encodeBoneFrame->qw = (double) self->encodeMotionData.encodeBoneFrame[frame][i].qw;

            PyList_Append(encodeBoneFrameList, (PyObject*)encodeBoneFrame);
            Py_DECREF(encodeBoneFrame);
        }

        return encodeBoneFrameList;
    }else{
        PyObject *encodeBoneFrameList = PyList_New(0);
        for (int i = 0; i < self->encodeMotionData.encodeBoneFrame_size; ++i) {
            PyObject *boneList = PyList_New(0);
            for (int j = 0; j < self->encodeMotionData.nameIndexer_size; ++j) {
                PyEncodeBoneFrame *encodeBoneFrame = (PyEncodeBoneFrame *) PyObject_CallObject((PyObject *) &PyEncodeBoneFrameType,
                                                                                               NULL);
                encodeBoneFrame->x = (double) self->encodeMotionData.encodeBoneFrame[i][j].x;
                encodeBoneFrame->y = (double) self->encodeMotionData.encodeBoneFrame[i][j].y;
                encodeBoneFrame->z = (double) self->encodeMotionData.encodeBoneFrame[i][j].z;

                encodeBoneFrame->qx = (double) self->encodeMotionData.encodeBoneFrame[i][j].qx;
                encodeBoneFrame->qy = (double) self->encodeMotionData.encodeBoneFrame[i][j].qy;
                encodeBoneFrame->qz = (double) self->encodeMotionData.encodeBoneFrame[i][j].qz;
                encodeBoneFrame->qw = (double) self->encodeMotionData.encodeBoneFrame[i][j].qw;

                PyList_Append(boneList, (PyObject*)encodeBoneFrame);
                Py_DECREF(encodeBoneFrame);
            }
            PyList_Append(encodeBoneFrameList, boneList);
            Py_DECREF(boneList);
        }

        return encodeBoneFrameList;
    }
}

static PyObject* setEncodeBoneFrame(PyEncodeMotion* self, PyObject* args){
    int frame, bone;
    PyEncodeBoneFrame *pyEncodeBoneFrame;

    //1番目にフレーム番号、2番目にボーン番号、3番目に代入するpyBoneFrame
    if(!PyArg_ParseTuple(args, "iiO", &frame, &bone, &pyEncodeBoneFrame)) return NULL;

    //callocで確保されているメモリに入り切らなければreallocで再確保する
    if((self->encodeMotionData.encodeBoneFrame_size/FrameExpansionRate+1)*FrameExpansionRate <= frame){
        printf("memory reallocate %lu bytes -> %lu bytes\n", sizeof(struct EncodeBoneFrame)*((self->encodeMotionData.encodeBoneFrame_size/FrameExpansionRate+1)*FrameExpansionRate), sizeof(struct EncodeBoneFrame)*((self->encodeMotionData.encodeBoneFrame_size/FrameExpansionRate+1)*FrameExpansionRate+FrameExpansionRate));
        void* temp = realloc(self->encodeMotionData.encodeBoneFrame, sizeof(struct EncodeBoneFrame)*((self->encodeMotionData.encodeBoneFrame_size/FrameExpansionRate+1)*FrameExpansionRate+FrameExpansionRate));
        if(temp == NULL){
            PyErr_SetString(PyExc_MemoryError, "realloc missed");
            return NULL;
        }
        self->encodeMotionData.encodeBoneFrame = temp;

        //初期化
        for (int i = self->encodeMotionData.encodeBoneFrame_size+1; i <= self->encodeMotionData.encodeBoneFrame_size+FrameExpansionRate; ++i) {
            self->encodeMotionData.encodeBoneFrame[i] = calloc(sizeof(struct EncodeBoneFrame), self->encodeMotionData.nameIndexer_size);
        }
        self->encodeMotionData.encodeBoneFrame[self->encodeMotionData.encodeBoneFrame_size+FrameExpansionRate+1] = NULL;
    }

    if(self->encodeMotionData.encodeBoneFrame_size < frame) self->encodeMotionData.encodeBoneFrame_size = frame;

    //範囲外アクセスのエラー処理
    if(self->encodeMotionData.nameIndexer_size < bone) {
        PyErr_SetString(PyExc_IndexError, "bone out of range.");
        return NULL;
    }
    self->encodeMotionData.encodeBoneFrame[frame][bone].x = pyEncodeBoneFrame->x;
    self->encodeMotionData.encodeBoneFrame[frame][bone].y = pyEncodeBoneFrame->y;
    self->encodeMotionData.encodeBoneFrame[frame][bone].z = pyEncodeBoneFrame->z;

    self->encodeMotionData.encodeBoneFrame[frame][bone].qx = pyEncodeBoneFrame->qx;
    self->encodeMotionData.encodeBoneFrame[frame][bone].qy = pyEncodeBoneFrame->qy;
    self->encodeMotionData.encodeBoneFrame[frame][bone].qz = pyEncodeBoneFrame->qz;
    self->encodeMotionData.encodeBoneFrame[frame][bone].qw = pyEncodeBoneFrame->qw;

    long double norm = sqrtl(powl(pyEncodeBoneFrame->qx, 2) + powl(pyEncodeBoneFrame->qy, 2) + powl(pyEncodeBoneFrame->qz, 2) + powl(pyEncodeBoneFrame->qw, 2));
    if(!(norm > 0.99 && norm < 1.1)){
        char err[128];
        sprintf(err, "Bad quaternion norm. norm:%Lf", norm);
        PyErr_SetString(PyExc_TypeError, err);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* getNameIndexer(PyEncodeMotion* self, PyObject* args){
    PyObject* index = PyList_New(self->encodeMotionData.nameIndexer_size);
    for (int i = 0; i < self->encodeMotionData.nameIndexer_size; ++i){
        //終端文字の除去
        int size = self->encodeMotionData.nameIndexer[i].name_byte;
        PyList_SetItem(index,self->encodeMotionData.nameIndexer[i].index, PyBytes_FromStringAndSize(self->encodeMotionData.nameIndexer[i].name, size));
    }
    return index;
}

static PyMethodDef pyEncodeMotionMethods[] = {
        {"getBoneFrame", (PyCFunction) getEncodeBoneFrame, METH_VARARGS, "エンコードされたボーンフレームを取得します。 args:(int:フレーム番号, int:ボーン番号)"},
        {"setBoneFrame", (PyCFunction) setEncodeBoneFrame, METH_VARARGS, "エンコードされたボーンフレームセットします。 args:(int:フレーム番号, int:ボーン番号, PyEncodeBoneFrame:セットするボーンフレーム)"},
        {"getNameIndex", (PyCFunction) getNameIndexer, METH_NOARGS, "ボーンのNameIndexerを取得します。"},
        {"getIndexFromBoneName", (PyCFunction) getIndexFromBoneName, METH_VARARGS, "ボーン名からインデックスを取得します。"},
        {NULL}
};

static PyTypeObject PyEncodeMotionType = {
        .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "VMDConverter.PyEncodeMotion",
        .tp_doc = "Python encode motion object",
        .tp_basicsize = sizeof(PyEncodeMotion),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = PyType_GenericNew,
        .tp_dealloc = (destructor) PyEncodeMotionDealloc,
        .tp_methods = pyEncodeMotionMethods,
};

//PyEncodeBoneFrameに関する実装---------------------------------------------------------
static PyMemberDef PyEncodeBoneFrameMembers[] = {
        {"x", T_DOUBLE, offsetof(PyEncodeBoneFrame, x), 0, "x座標"},
        {"y", T_DOUBLE, offsetof(PyEncodeBoneFrame, y), 0, "y座標"},
        {"z", T_DOUBLE, offsetof(PyEncodeBoneFrame, z), 0, "z座標"},
        {"qx", T_DOUBLE, offsetof(PyEncodeBoneFrame, qx), 0, "クォータニオンx座標"},
        {"qy", T_DOUBLE, offsetof(PyEncodeBoneFrame, qy), 0, "クォータニオンy座標"},
        {"qz", T_DOUBLE, offsetof(PyEncodeBoneFrame, qz), 0, "クォータニオンz座標"},
        {"qw", T_DOUBLE, offsetof(PyEncodeBoneFrame, qw), 0, "クォータニオンw座標"},
        {NULL}
};

static void PyEncodeBoneFrameDealloc(PyEncodeBoneFrame* self){
    Py_TYPE(self)->tp_free((PyObject*) self);
}

//3軸ベクトルとクォータニオンを線形補間で計算する
static PyObject* moveComplement(PyObject* self, PyObject* args){
    PyEncodeBoneFrame* n1;
    PyEncodeBoneFrame* n2;
    double t;

    PyEncodeBoneFrame* ret = (PyEncodeBoneFrame*) PyObject_CallObject((PyObject*) &PyEncodeBoneFrameType, NULL);

    if(!PyArg_ParseTuple(args, "OOd", &n1, &n2, &t)) return NULL;

    if(!(t >= 0.0 && t <= 1.0)){
        PyErr_SetString(PyExc_ValueError, "time must be 0 to 1.");
        printf("time: %f\n", t);
        return NULL;
    }

    struct Quaternion q_n1, q_n2;
    q_n1.x = n1->qx;
    q_n1.y = n1->qy;
    q_n1.z = n1->qz;
    q_n1.w = n1->qw;

    q_n2.x = n2->qx;
    q_n2.y = n2->qy;
    q_n2.z = n2->qz;
    q_n2.w = n2->qw;

    struct Quaternion q_ret = SphericalLinearInterpolation(q_n1, q_n2, t);

    ret->qx = q_ret.x;
    ret->qy = q_ret.y;
    ret->qz = q_ret.z;
    ret->qw = q_ret.w;

    ret->x = n1->x - ((n1->x-n2->x)*t);
    ret->y = n1->y - ((n1->y-n2->y)*t);
    ret->z = n1->z - ((n1->z-n2->z)*t);

    printf("%f, %f, %f\n", n1->y, n2->y, n1->y - ((n1->y-n2->y)*t));

    return (PyObject*)ret;
}

static PyTypeObject PyEncodeBoneFrameType = {
        .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "VMDConverter.PyEncodeBoneFrame",
        .tp_doc = "Python encode bone frame object",
        .tp_basicsize = sizeof(PyEncodeBoneFrame),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = PyType_GenericNew,
        .tp_members=PyEncodeBoneFrameMembers,
        .tp_dealloc=(destructor) PyEncodeBoneFrameDealloc
};


//directパッケージの関数一覧
static PyMethodDef PyDirectMethods[] = {
        {"loadVMD", loadVMD_wrapper, METH_VARARGS},
        {"writeVMD", writeVMD_wrapper, METH_VARARGS, "VMDにメーションを書きこきます。args:(str:書き込み先パス, PyMotion:書き込むモーション)"},

        {"encodeMotion", encodeMotion_wrapper, METH_VARARGS, "読み込んだVMDをAIが解析できる形にエンコードします。args:(PyMotion:エンコードするモーション, PyModel:基準となるモデル)"},
        {"decodeMotion", decodeMotion_wrapper, METH_VARARGS, "エンコードしたモーションデータをVMD形式にデコードします。args:(PyEncodeMotion:デコードするモーション, PyModel:基準となるモデル, List<str>:デコードする際に必要なボーン))"},

        {"createEncodeMotion",  PyEncodeMotionNew, METH_VARARGS, "エンコードモーションデータを新規作成します。 args:(List<bytes>:ボーン名)"},
        {"linearInterpolation", moveComplement, METH_VARARGS, "エンコードモーションデータを線形補間します。"},
        {NULL}
};

//Docなどの詳細な設定
static PyModuleDef PyDirectModule = {
        .m_base = PyModuleDef_HEAD_INIT,
        .m_name = "VMDConverter",
        .m_doc = "辞書型を介さずに直接モーションを操作できるパッケージ",
        .m_size = -1,
        .m_methods = PyDirectMethods,
};

PyMODINIT_FUNC PyInit_VMDConverter(){
    printf("VMDConverter load\n");

    PyObject *m = PyModule_Create(&PyDirectModule);

    if(PyType_Ready(&PyMotionType) < 0 || PyType_Ready(&PyModelType) < 0 || PyType_Ready(&PyEncodeMotionType) < 0 ||
            PyType_Ready(&PyBoneFrameType) < 0 || PyType_Ready(&PyEncodeBoneFrameType) < 0){
        return NULL;
    }

    if(PyModule_AddObjectRef(m, "PyMotion", (PyObject*)&PyMotionType) < 0 || PyModule_AddObjectRef(m, "PyModel", (PyObject*)&PyModelType) ||
            PyModule_AddObjectRef(m, "PyEncodeMotion", (PyObject*)&PyEncodeMotionType) < 0 || PyModule_AddObjectRef(m, "PyEncodeBoneFrame", (PyObject*)&PyEncodeBoneFrameType) < 0 ){
        Py_DECREF(m);
        return NULL;
    }

    return m;
}