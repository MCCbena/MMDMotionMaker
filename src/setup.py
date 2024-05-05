from setuptools import setup, Extension

setup(
    ext_modules=[
        Extension(
            name="VMDConverter",
            sources=["wrapper.c", "vmd/vmdLoader.c", "pmx/modelLoader.c"],
            include_dirs=["vmd", "pmx"],
        ),
    ]
)