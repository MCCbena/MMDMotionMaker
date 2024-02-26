from setuptools import setup, Extension

setup(
    ext_modules=[
        Extension(
            name="VMDConverter",
            sources=["wrapper.c", "vmd/vmdLoader.c", "modelLoader.c", "vmd/vmdStruct.c"],
            include_dirs=["./usr/include/json-c", "vmd"],
            libraries=['json-c'],
        ),
    ]
)