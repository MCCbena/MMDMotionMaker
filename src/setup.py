from setuptools import setup, Extension

setup(
    ext_modules=[
        Extension(
            name="VMDConverter",
            sources=["wrapper.c", "vmdLoader.c", "modelLoader.c"],
            include_dirs=["./usr/include/json-c"],
            libraries=['json-c'],

        ),
    ]
)