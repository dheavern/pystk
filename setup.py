import os
import platform
import re
import shutil
import subprocess
import sys
from distutils.version import LooseVersion

from setuptools import Command
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from setuptools.command.build_py import build_py

ASSET_URL = "https://sourceforge.net/projects/supertuxkart/files/stk-assets-mobile/git/full-hd.zip"
this_directory = os.path.dirname(os.path.abspath(__file__))


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: " +
                ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)',
                                                   out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        for ext in self.extensions:
            if isinstance(ext, CMakeExtension):
                self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(
            os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(
                cfg.upper(),
                extdir)]
            if sys.maxsize > 2 ** 32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j10']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get('CXXFLAGS', ''),
            self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args,
                              cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args,
                              cwd=self.build_temp)


class BuildAndCopyData(build_py):
    description = "build_py and copy the supertuxkart assets"

    def run(self):
        super().run()
        for cmd_name in self.get_sub_commands():
            self.run_command(cmd_name)
        import shutil
        try:
            shutil.rmtree(os.path.join(self.build_lib, 'data'))
        except FileNotFoundError:
            pass
        data_files = []
        for base in ['data/']:
            base = os.path.join(this_directory, base)
            for root, dirs, files in os.walk(base):
                target_dir = os.path.join(self.build_lib, self.packages[0], 'data', root[len(base):])
                try:
                    os.makedirs(target_dir)
                except FileExistsError:
                    pass
                for f in files:
                    if '.py' not in f:
                        self.copy_file(os.path.join(root, f), os.path.join(target_dir, f))


with open("README.md", "r") as fh:
    long_description = fh.read()


def ignore(base, entries):
    return [e for e in entries if '.py' in e]


setup(
    name='PySuperTuxKart',
    version='1.0a0',
    author='Philipp Krähenbühl',
    author_email='philkr@utexas.edu',
    description='Python SuperTuxKart inferface',
    long_description=long_description,
    classifiers=[
        'Development Status :: 4 - Beta',
        'Environment :: Console',
        'Intended Audience :: Education',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Topic :: Scientific/Engineering',
        'Topic :: Scientific/Engineering :: Artificial Intelligence',
    ],
 
    # tell setuptools that all packages will be under the 'src' directory
    # and nowhere else
    packages=['pystk_data'],
    package_dir={'pystk_data': '.'},
    package_data={'pystk_data': []},
    # TODO: Add more
    #install_requires=['cmake'],
    python_requires='>=3.6',
    ext_modules=[CMakeExtension('pystk_cpp')],
    # add custom build_ext command
    cmdclass=dict(build_py=BuildAndCopyData, build_ext=CMakeBuild),
    zip_safe=False,
)
