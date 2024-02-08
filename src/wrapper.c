#include <Python.h>

extern char* getMotion(const char*);
extern char* getModel(const char*);
extern void writeMotion(const char*, const char*);

//モーションをjsonで吐き出すやつ
static PyObject* getMotion_wrapper(PyObject* self, PyObject* args)
{
    const char* path = NULL;


    if (! PyArg_ParseTuple(args, "|s", &path)){
        return NULL;
    }

    char* model_json = getMotion(path);
    PyObject* pyObject = Py_BuildValue("s", model_json);
    free(model_json);
    return pyObject;
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
    const char* output_path = NULL;
    const char* json = NULL;

    if(!PyArg_ParseTuple(args, "|ss", &output_path, &json)){
        return NULL;
    }
    writeMotion(output_path, json);
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