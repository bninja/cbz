from distutils.core import setup
from distutils.extension import Extension

from Cython.Distutils import build_ext


cbz = Extension(
    'cbz',
    libraries=['cbz'],
    sources=['cbz.pyx'],
    )

setup(
  name='cbz',
  cmdclass={
      'build_ext': build_ext
      },
  ext_modules=[cbz],
  )
