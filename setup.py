from distutils.core import setup, Extension

module = Extension('pydrone',
                    libraries = ['mozjs','pthread'],
                    include_dirs = ['/usr/local/include/mozjs', '/usr/include/mozjs'],
                    sources = ['pydrone.c'])

setup (name = 'pydrone',
       author = 'Unbit',
       author_email = 'info@unbit.it',
       version = '0.3',
       description = 'pydrone',
       ext_modules = [module])
