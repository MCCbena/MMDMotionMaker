#include <python3.11/Python.h>
#pragma pack(1) // 構造体をきつくパッキングし、1バイトのアライメント
#include "vmdStruct.c"
#include <stdbool.h>

extern MotionData getMotion(const char*, bool);
extern char* getModel(const char*);
extern void writeMotion(const char*, MotionData);

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
    }

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
    Py_DECREF(args);
    free(motionData.boneFrame);
    return vmd;
}

//モデルをjsonで吐き出すやつ
static PyObject* getModel_wrapper(PyObject* self, PyObject* args)
{
    const char* path = NULL;
    if (! PyArg_ParseTuple(args, "|s", &path)){
        return NULL;
    }

    char* motion_json = getModel(path);
    return Py_BuildValue("s", motion_json);
}

//モーションをjsonからVMDに書き込むやつ
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


// メソッドを登録
static PyMethodDef vmd_methods[] = {
        {"getModel", getModel_wrapper, METH_VARARGS, "get Model to JSON"},
        { "getMotion", getMotion_wrapper, METH_VARARGS, "get Motion to JSON" },
        {"writeMotion", writeMotion_wrapper, METH_VARARGS, "write JSON motion in VMD"},
        {NULL}
};

static struct PyModuleDef modules ={
        PyModuleDef_HEAD_INIT,
        "VMDConverter",
        PyDoc_STR("convert vmd,pmx to JSON"),
        0,
        vmd_methods,
};

PyMODINIT_FUNC PyInit_VMDConverter(){
    return PyModule_Create(&modules);
}