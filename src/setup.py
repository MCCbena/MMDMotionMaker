from setuptools import setup, Extension
import sys
import pathlib

setup(
    ext_modules=[
        Extension(
            name="VMDConverter",
            sources=["wrapper.c", "vmd/vmdLoader.c", "pmx/modelLoader.c"],
            include_dirs=["vmd", "pmx"],
        ),
    ],
    data_files=[
        (
            ".",
            ["VMDConverter.pyi", "pyi.typed"]
        )
    ]
)