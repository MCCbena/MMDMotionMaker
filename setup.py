from setuptools import setup, Extension

setup(
    ext_modules=[
        Extension(
            name="VMDConverter",
            sources=["vmdLoader.c" ,"wrapper.c"],
            include_dirs=["./usr/include/json-c"],
            libraries=['json-c'],

        ),
    ]
)