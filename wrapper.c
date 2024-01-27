#include <Python.h>

extern void getMotion(const char*);

//definition of out method
static PyObject* getMotion_wrapper(PyObject* self, PyObject* args, PyObject* kw)
{
    const char* path = NULL;
    static char* argnames[] = {"path"};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|s",
                                     argnames, &path))
        return NULL;
    getMotion(path);
    return Py_BuildValue("");
}

//definition of all methods of my module
static PyMethodDef vdmLoader_methods[] = {
        {"out", METH_VARARGS | METH_KEYWORDS},
        {NULL},
};

// hello module definition struct
static struct PyModuleDef hello = {
        PyModuleDef_HEAD_INIT,
        "hello",
        "Python3 C API Module(Sample 1)",
        -1,
        vdmLoader_methods
};

//module creator
PyMODINIT_FUNC PyInit_hello(void)
{
    return PyModule_Create(&hello);
}