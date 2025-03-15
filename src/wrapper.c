#include <python3.11/Python.h>
#include <stddef.h>
#include <python3.11/structmember.h>
#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント
#include "vmd/indexlib.h"
#include "vmd/vmdStruct.h"
#include "pmx/pmxStruct.h"
#include <stdbool.h>

extern MotionData getMotion(const char*, bool);
extern void getModel(const char *, struct Model *);
extern EncodeMotionData motionEncoder(struct Model model, MotionData *motionData);
extern MotionData motionDecoder(EncodeMotionData encodeMotionData, struct Model model, struct Index need_bone);
extern void writeMotion(const char*, MotionData);


//MotionDataをPython Dictに変換する関数
static PyObject* convertVMDDict(MotionData motionData){
    PyObject* vmd = PyDict_New();

    PyObject* header = PyDict_New();
    PyObject* header_content = PyByteArray_FromStringAndSize(motionData.header.header, 30);
    PyDict_SetItemString(header, "header", header_content);
    Py_DECREF(header_content);

    PyObject* modelName_content = PyByteArray_FromStringAndSize(motionData.header.modelName, 20);
    PyDict_SetItemString(header, "modelName", modelName_content);
    Py_DECREF(modelName_content);

    PyDict_SetItemString(vmd, "header", header);
    Py_DECREF(header);

    PyObject* maxFrame_content = PyLong_FromLong(motionData.maxFrame.maxFrame);
    PyDict_SetItemString(vmd, "maxFrame", maxFrame_content);
    Py_DECREF(maxFrame_content);


    PyObject* bones = PyList_New(0);
    for(int i = 0; i < motionData.maxFrame.maxFrame; i++){
        PyObject* bone = PyDict_New();

        PyObject* name_content = PyBytes_FromStringAndSize(motionData.boneFrame[i].name, 15);
        PyDict_SetItemString(bone, "name", name_content);
        Py_DECREF(name_content);

        PyObject* frame_content = PyLong_FromLong(motionData.boneFrame[i].frame);
        PyDict_SetItemString(bone, "frame", frame_content);
        Py_DECREF(frame_content);

        PyObject* x_content = PyFloat_FromDouble(motionData.boneFrame[i].x);
        PyDict_SetItemString(bone, "x", x_content);
        Py_DECREF(x_content);

        PyObject* y_content = PyFloat_FromDouble(motionData.boneFrame[i].y);
        PyDict_SetItemString(bone, "y", y_content);
        Py_DECREF(y_content);

        PyObject* z_content = PyFloat_FromDouble(motionData.boneFrame[i].z);
        PyDict_SetItemString(bone, "z", z_content);
        Py_DECREF(z_content);

        PyObject* qx_content = PyFloat_FromDouble(motionData.boneFrame[i].qx);
        PyDict_SetItemString(bone, "qx", qx_content);
        Py_DECREF(qx_content);

        PyObject* qy_content = PyFloat_FromDouble(motionData.boneFrame[i].qy);
        PyDict_SetItemString(bone, "qy", qy_content);
        Py_DECREF(qy_content);

        PyObject* qz_content = PyFloat_FromDouble(motionData.boneFrame[i].qz);
        PyDict_SetItemString(bone, "qz", qz_content);
        Py_DECREF(qz_content);

        PyObject* qw_content = PyFloat_FromDouble(motionData.boneFrame[i].qw);
        PyDict_SetItemString(bone, "qw", qw_content);
        Py_DECREF(qw_content);

        PyObject* beziers = PyList_New(0);
        for(int j = 0; j < 64; j++){
            PyObject* bezier = PyLong_FromLong(motionData.boneFrame[i].bezier[j]);
            PyList_Append(beziers, bezier);
            Py_DECREF(bezier);
        }
        PyDict_SetItemString(bone, "bezier", beziers);
        Py_DECREF(beziers);
        PyList_Append(bones, bone);
        Py_DECREF(bone);
    }
    PyDict_SetItemString(vmd, "boneFrame", bones);
    Py_DECREF(bones);

    return vmd;
}
//モーションをjsonで吐き出すやつ
static PyObject* getMotion_wrapper(PyObject* self, PyObject* args)
{
    const char* path = NULL;
    int frame_completion = 0;

    if (! PyArg_ParseTuple(args, "s|i", &path, &frame_completion)){
        return NULL;
    }
    MotionData motionData;
    switch (frame_completion) {
        case 0:
            motionData = getMotion(path, false);
            break;
        case 1:
            motionData = getMotion(path, true);
            break;
        default:
            motionData = getMotion(path, false);
            break;
    }

    PyObject* vmd = convertVMDDict(motionData);

    Py_DECREF(args);
    free(motionData.boneFrame);
    return vmd;
}

//モデルを辞書型で吐き出すやつ
static PyObject* getModel_wrapper(PyObject* self, PyObject* args){
    const char* path = NULL;
    if (! PyArg_ParseTuple(args, "|s", &path)){
        return NULL;
    }
    PyObject* model_data = PyList_New(0);
    struct Model model;
    getModel(path, &model);
    printf("encode mode %d\n", model.header.encode);
    for(int i = 0; i < model.bone_size; i++){
        PyObject* bytes = PyBytes_FromStringAndSize(model.bone[i].model_name_jp.byte, model.bone[i].model_name_jp.byte_size);
        PyList_Append(model_data, bytes);
        //printf("char: %s| byte: %d\n", model.bone[i].model_name_jp.byte, model.bone[i].model_name_jp.byte_size);
        Py_DECREF(bytes);
    }

    return model_data;
}

//モーションを辞書型からVMDに書き込むやつ
static PyObject* writeMotion_wrapper(PyObject* self, PyObject* args){
    const char* output_path;
    PyObject* data;
    MotionData motionData;

    if(!PyArg_ParseTuple(args, "sO", &output_path, &data)){
        return NULL;
    }
    PyObject* header = PyDict_GetItemString(data, "header");
    memcpy(&motionData.header.header, PyByteArray_AsString(PyDict_GetItemString(header, "header")), 30);
    memcpy(&motionData.header.modelName, PyByteArray_AsString(PyDict_GetItemString(header, "modelName")), 20);


    motionData.maxFrame.maxFrame = (int) PyLong_AsLong(PyDict_GetItemString(data, "maxFrame"));

    PyObject* boneFrames = PyDict_GetItemString(data, "boneFrame");
    motionData.boneFrame = malloc(111*motionData.maxFrame.maxFrame);
    for(int i = 0; i < motionData.maxFrame.maxFrame; i++){
        PyObject* boneFrame = PyList_GetItem(boneFrames, i);
        motionData.boneFrame[i].frame = (int) PyLong_AsLong(PyDict_GetItemString(boneFrame, "frame"));

        motionData.boneFrame[i].x = (float) PyFloat_AsDouble(PyDict_GetItemString(boneFrame, "x"));
        motionData.boneFrame[i].y = (float) PyFloat_AsDouble(PyDict_GetItemString(boneFrame, "y"));
        motionData.boneFrame[i].z = (float) PyFloat_AsDouble(PyDict_GetItemString(boneFrame, "z"));

        motionData.boneFrame[i].qx = (float) PyFloat_AsDouble(PyDict_GetItemString(boneFrame, "qx"));
        motionData.boneFrame[i].qy = (float) PyFloat_AsDouble(PyDict_GetItemString(boneFrame, "qy"));
        motionData.boneFrame[i].qz = (float) PyFloat_AsDouble(PyDict_GetItemString(boneFrame, "qz"));
        motionData.boneFrame[i].qw = (float) PyFloat_AsDouble(PyDict_GetItemString(boneFrame, "qw"));


        memcpy(&motionData.boneFrame[i].name, PyBytes_AS_STRING(PyDict_GetItemString(boneFrame, "name")), 15);

        PyObject *beziers = PyDict_GetItemString(boneFrame, "bezier");

        for (int j = 0; j < 64; j++) {
            PyObject *bezier = PyList_GetItem(beziers, j);
            motionData.boneFrame[i].bezier[j] = (char) PyLong_AsLong(bezier);
        }
    }
    writeMotion(output_path, motionData);
    free(motionData.boneFrame);
    return Py_None;
}

static PyObject* motionEncode_wrapper(PyObject* self, PyObject* args){

    char *motion_path, *pmx_path;
    if(!PyArg_ParseTuple(args, "ss", &motion_path, &pmx_path)){
        return NULL;
    }

    struct Model model;
    getModel(pmx_path, &model);
    MotionData motionData = getMotion(motion_path, true);
    if(motionData.maxFrame.maxFrame == -1){
        return Py_None;
    }

    EncodeMotionData encodeMotionData = motionEncoder(model, &motionData);

    //辞書型の作成
    PyObject* main_dict = PyDict_New();
    PyObject* name_indexer = PyList_New(0);
    PyObject* encode_boneframe = PyList_New(0);

    //nameIndexerの辞書化
    for(int i = 0; i < encodeMotionData.nameIndexer_size; i++){
        PyObject* temp = PyDict_New();

        PyObject* name_content = PyByteArray_FromStringAndSize(encodeMotionData.nameIndexer[i].name, encodeMotionData.nameIndexer[i].name_byte);
        PyDict_SetItemString(temp, "name", name_content);
        Py_DECREF(name_content);

        PyObject* index_content = PyLong_FromLong(encodeMotionData.nameIndexer[i].index);
        PyDict_SetItemString(temp, "index", index_content);
        Py_DECREF(index_content);

        PyList_Append(name_indexer, temp);
        Py_DECREF(temp);
    }

    //ボーンフレームの登録
    for(int i0 = 0; i0 < encodeMotionData.encodeBoneFrame_size; i0++){//最大フレームのループ
        PyObject* encode_boneframe_temp_list = PyList_New(0);
        for(int i1 = 0; i1 < encodeMotionData.nameIndexer_size; i1++){//ボーンの数ループ
            struct EncodeBoneFrame encodeBoneFrame = encodeMotionData.encodeBoneFrame[i0][i1];
            PyObject* temp = PyDict_New();

            PyObject* x_content = PyFloat_FromDouble((double)encodeBoneFrame.x);
            PyDict_SetItemString(temp, "x", x_content);
            Py_DECREF(x_content);

            PyObject* y_content = PyFloat_FromDouble((double)encodeBoneFrame.y);
            PyDict_SetItemString(temp, "y", y_content);
            Py_DECREF(y_content);

            PyObject* z_content = PyFloat_FromDouble((double)encodeBoneFrame.z);
            PyDict_SetItemString(temp, "z", z_content);
            Py_DECREF(z_content);

            PyObject* qx_content = PyFloat_FromDouble((double)encodeBoneFrame.qx);
            PyDict_SetItemString(temp, "qx", qx_content);
            Py_DECREF(qx_content);

            PyObject* qy_content = PyFloat_FromDouble((double)encodeBoneFrame.qy);
            PyDict_SetItemString(temp, "qy", qy_content);
            Py_DECREF(qy_content);

            PyObject* qz_content = PyFloat_FromDouble((double)encodeBoneFrame.qz);
            PyDict_SetItemString(temp, "qz", qz_content);
            Py_DECREF(qz_content);

            PyObject* qw_content = PyFloat_FromDouble((double)encodeBoneFrame.qw);
            PyDict_SetItemString(temp, "qw", qw_content);
            Py_DECREF(qw_content);

            PyList_Append(encode_boneframe_temp_list, temp);
            Py_DECREF(temp);
        }
        PyList_Append(encode_boneframe, encode_boneframe_temp_list);
        Py_DECREF(encode_boneframe_temp_list);
    }

    //配列の結合
    PyDict_SetItemString(main_dict, "nameIndexer", name_indexer);
    Py_DECREF(name_indexer);
    PyDict_SetItemString(main_dict, "encodeBoneFrame", encode_boneframe);
    Py_DECREF(encode_boneframe);

    //使い終わった変数のfree
    Py_DECREF(args);
    free(motionData.boneFrame);
    for(int i = 0; i < encodeMotionData.encodeBoneFrame_size; i++){
        free(encodeMotionData.encodeBoneFrame[i]);
    }
    free(encodeMotionData.encodeBoneFrame);
    for(int i = 0; i < model.bone_size; i++) free(model.bone[i].child_bones);
    free(model.bone);
    free(model.texture);
    free(model.surface);
    free(model.topData);
    free(model.material);

    return main_dict;
}

static PyObject* motionDecode_wrapper(PyObject* self, PyObject* args){


    char *pmx_path;
    PyObject* encodeMotionDataDict;
    PyObject* needBonePyList;
    printf("start\n");

    if(!PyArg_ParseTuple(args, "OOs", &encodeMotionDataDict, &needBonePyList, &pmx_path)){
        return NULL;
    }

    struct Model model;
    getModel(pmx_path, &model);
    printf("model loaded\n");

    EncodeMotionData encodeMotionData;

    //nameIndexerに関する代入操作
    PyObject* nameIndexer = PyDict_GetItemString(encodeMotionDataDict, "nameIndexer");
    printf("nameIndexer load\n");
    encodeMotionData.nameIndexer_size = PyList_GET_SIZE(nameIndexer);
    printf("nameIndexer get size %d\n", encodeMotionData.nameIndexer_size);

    for (int i = 0; i < encodeMotionData.nameIndexer_size; ++i) {
        PyObject* nameIndexerOne = PyList_GetItem(nameIndexer, i);
        printf("%s\n", word_decode(PyByteArray_AsString(PyDict_GetItemString(nameIndexerOne, "name")), 15, "UTF-8", "SHIFT-JIS"));
        memcpy(encodeMotionData.nameIndexer[i].name, PyByteArray_AsString(PyDict_GetItemString(nameIndexerOne, "name")), 15);
        encodeMotionData.nameIndexer[i].index = (int)PyLong_AsLong(PyDict_GetItemString(nameIndexerOne, "index"));
        encodeMotionData.nameIndexer[i].name_byte = 15; //MMDのvmdファイルではボーン名が15バイト固定なためここでもそれをもちいる。
    }

    printf("nameIndexer loaded\n");


    //ボーンフレームに関する代入操作
    encodeMotionData.encodeBoneFrame_size = PyList_GET_SIZE(PyDict_GetItemString(encodeMotionDataDict, "encodeBoneFrame"));
    encodeMotionData.encodeBoneFrame = calloc(sizeof(struct EncodeBoneFrame), encodeMotionData.encodeBoneFrame_size);
    printf("get size %d\n", encodeMotionData.encodeBoneFrame_size);

    for (int i = 0; i < encodeMotionData.encodeBoneFrame_size; ++i) {
        PyObject* boneFramePy = PyList_GetItem(PyDict_GetItemString(encodeMotionDataDict, "encodeBoneFrame"), i);
        encodeMotionData.encodeBoneFrame[i] = calloc(sizeof(struct EncodeBoneFrame), PyList_GET_SIZE(boneFramePy));
        struct EncodeBoneFrame* encodeBoneFrame = encodeMotionData.encodeBoneFrame[i];

        for (int j = 0; j < PyList_GET_SIZE(boneFramePy); ++j) {
            struct EncodeBoneFrame encodeBoneFrameBone = encodeBoneFrame[j];
            PyObject* boneFramePyBone = PyList_GetItem(boneFramePy, j);

            encodeBoneFrameBone.qx = PyFloat_AsDouble(PyDict_GetItemString(boneFramePyBone, "qx"));
            encodeBoneFrameBone.qy = PyFloat_AsDouble(PyDict_GetItemString(boneFramePyBone, "qy"));
            encodeBoneFrameBone.qz = PyFloat_AsDouble(PyDict_GetItemString(boneFramePyBone, "qz"));
            encodeBoneFrameBone.qw = PyFloat_AsDouble(PyDict_GetItemString(boneFramePyBone, "qw"));

            if(i == 0) printf("%Lf, %Lf, %Lf, %Lf\n", encodeBoneFrameBone.qx, encodeBoneFrameBone.qy, encodeBoneFrameBone.qz, encodeBoneFrameBone.qw);

            encodeBoneFrameBone.x = PyFloat_AsDouble(PyDict_GetItemString(boneFramePyBone, "x"));
            encodeBoneFrameBone.y = PyFloat_AsDouble(PyDict_GetItemString(boneFramePyBone, "y"));
            encodeBoneFrameBone.z = PyFloat_AsDouble(PyDict_GetItemString(boneFramePyBone, "z"));
        }
    }

    printf("needBone load\n");
    //必要なボーンを割り出すneedBoneに関する代入操作
    struct Index needBone = makeIndex(400, 15);
    for (int i = 0; i < PyList_GET_SIZE(needBonePyList); ++i) {
        char* boneName = PyBytes_AsString(PyList_GetItem(needBonePyList, i));
        if(getnIndex(needBone, boneName, 15) == -1){
            addIndex(&needBone,boneName, 15);
        }
    }

    MotionData motionData = motionDecoder(encodeMotionData, model, needBone);

    PyObject* vmd = convertVMDDict(motionData);

    //motionのfree
    free(motionData.boneFrame);

    //modelのfree
    free(model.bone);
    free(model.texture);
    free(model.surface);
    free(model.topData);
    free(model.material);

    //引数のfree
    Py_DECREF(args);

    printf("all finish\n");
    return vmd;
}


// メソッドを登録
static PyMethodDef vmd_methods[] = {
        {"getModel", getModel_wrapper, METH_VARARGS, "get Model to Dict. args:(str:ModelPath, int:mode)"},
        {"getMotion", getMotion_wrapper, METH_VARARGS, "get Motion to Dict. args:(str:MotionPath)" },
        {"writeMotion", writeMotion_wrapper, METH_VARARGS, "write Dict motion in VMD. args:(str:OutputPath, Dict:MotionData)"},
        {"motionEncode", motionEncode_wrapper, METH_VARARGS, "get EncodeMotion to Dict. args:(str:MotionPath, str:ModelPath)"},
        {"motionDecode", motionDecode_wrapper, METH_VARARGS, "decode EncodeMotion. args:(Dict:EncodeMotion, List<str>:NeedBone, str:ModelPath)"},
        {NULL}
};

static struct PyModuleDef modules ={
        PyModuleDef_HEAD_INIT,
        "VMDConverter",
        PyDoc_STR("convert vmd,pmx to Dict"),
        0,
        vmd_methods,
};

//Dictを用いないdirectパッケージの実装------------------------------------------------------------------------------------------
typedef struct {
    PyObject_HEAD
    MotionData motionData;
}PyMotion;

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

typedef struct {
    PyObject_HEAD
    struct Model model;
}PyModel;


typedef struct {
    PyObject_HEAD
    EncodeMotionData encodeMotionData;
}PyEncodeMotion;


//PyBoneFrameに関する実装----------------------------------------------------------------------
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
        .tp_name = "VMDConverter.direct.PyBoneFrame",
        .tp_doc = "Python boneframe object",
        .tp_basicsize = sizeof(PyBoneFrame),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = PyType_GenericNew,
        .tp_members = PyBoneFrameMembers,
};

//PyMotionの実装--------------------------------------------------
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
        Py_DECREF(pyBoneFrame);
    }

    return pyBoneFrames;
}

static PyMethodDef PyMotionMethods[] = {
        {"getPyBoneFrame", (PyCFunction)getPyBoneFrame_wrapper, METH_NOARGS, "ボーンフレームをListとして取得できます。"},
        {NULL}
};

static PyTypeObject PyMotionType = {
        .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "VMDConverter.direct.PyMotion",
        .tp_doc = "Python motion object",
        .tp_basicsize = sizeof(PyMotion),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = PyType_GenericNew,
        .tp_methods = PyMotionMethods,
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

    return (PyObject*) pyMotion;
}

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

static PyTypeObject PyEncodeMotionType = {
        .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "VMDConverter.direct.PyEncodeMotion",
        .tp_doc = "Python encode motion object",
        .tp_basicsize = sizeof(PyEncodeMotion),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = PyType_GenericNew,
};

//PyEncodeMotionに関する実装------------------------------------------------------------
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

static PyTypeObject PyModelType = {
        .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "VMDConverter.direct.PyModel",
        .tp_doc = "Python model object",
        .tp_basicsize = sizeof(PyModel),
        .tp_itemsize = 0,
        .tp_flags = Py_TPFLAGS_DEFAULT,
        .tp_new = PyModelNew,
};

//directパッケージの関数一覧
static PyMethodDef PyDirectMethods[] = {
        {"loadVMD", (PyCFunction) loadVMD_wrapper, METH_VARARGS, "VMDからモーションデータを読み込みます。args:(str:モーションのパス, bool:フレーム補完を有効にするか)"},
        {"writeVMD", (PyCFunction) writeVMD_wrapper, METH_VARARGS, "VMDにメーションを書きこきます。args:(str:書き込み先パス, PyMotion:書き込むモーション)"},

        {"encodeMotion", (PyCFunction) encodeMotion_wrapper, METH_VARARGS, "読み込んだVMDをAIが解析できる形にエンコードします。args:(PyMotion:エンコードするモーション, PyModel:基準となるモデル)"},
        {"decodeMotion", (PyCFunction) decodeMotion_wrapper, METH_VARARGS, "エンコードしたモーションデータをVMD形式にデコードします。args:(PyEncodeMotion:デコードするモーション, PyModel:基準となるモデル, List<str>:デコードする際に必要なボーン))"},
        {NULL}
};

//Docなどの詳細な設定
static PyModuleDef PyDirectModule = {
        .m_base = PyModuleDef_HEAD_INIT,
        .m_name = "VMDConverter.direct",
        .m_doc = "辞書型を介さずに直接モーションを操作できるパッケージ",
        .m_size = -1,
        .m_methods = PyDirectMethods,
};

PyMODINIT_FUNC PyInit_VMDConverter(){
    printf("VMDConverter load\n");

    PyObject *m = PyModule_Create(&modules);

    PyObject *subm;
    if(PyType_Ready(&PyMotionType) < 0 || PyType_Ready(&PyModelType) < 0 || PyType_Ready(&PyEncodeMotionType) < 0 ||
            PyType_Ready(&PyBoneFrameType) < 0){
        return NULL;
    }

    subm = PyModule_Create(&PyDirectModule);
    if (subm == NULL){
        return NULL;
    }

    if(PyModule_AddObjectRef(subm, "PyMotion", (PyObject*)&PyMotionType) < 0 || PyModule_AddObjectRef(subm, "PyModel", (PyObject*)&PyModelType) ||
            PyModule_AddObject(subm, "PyEncodeMotion", (PyObject*)&PyEncodeMotionType) < 0 || PyType_Ready(&PyBoneFrameType) < 0){
        Py_DECREF(subm);
        return NULL;
    }

    PyModule_AddObject(m, "direct", subm);

    return m;
}