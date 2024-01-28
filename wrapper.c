#include <Python.h>

extern char* getMotion(const char*);

//definition of out method
static PyObject* getMotion_wrapper(PyObject* self, PyObject* args)
{
    const char* path = NULL;

    static char* argnames[] = {"path", NULL};

    if (! PyArg_ParseTuple(args, "|s", &path)){
        return NULL;
    }

    char* motion_json = getMotion(path);
    return Py_BuildValue("s", motion_json);
}

// メソッドを登録
static PyMethodDef vmd_methods[] = {
        { "getMotion", getMotion_wrapper, METH_VARARGS, "getMotion to JSON" },
        { NULL }
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