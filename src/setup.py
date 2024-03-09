from setuptools import setup, Extension

setup(
    ext_modules=[
        Extension(
            name="VMDConverter",
            sources=["wrapper.c", "vmd/vmdLoader.c", "modelLoader.c", "vmd/vmdStruct.c"],
            include_dirs=["vmd"],
            libraries=['vmd/indexlib.c', 'json-c'],
        ),
    ]
)