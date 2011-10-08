from distutils.core import setup, Extension

module = Extension('pydrone',
                    libraries = ['mozjs'],
                    include_dirs = ['/usr/local/include/mozjs', '/usr/include/mozjs'],
                    sources = ['pydrone.c'])

setup (name = 'pydrone',
       version = '0.1',
       description = 'pydrone',
       ext_modules = [module])
