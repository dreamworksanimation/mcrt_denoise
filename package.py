# Copyright 2024-2025 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

# -*- coding: utf-8 -*-
import os, sys

unittestflags = (['@run_all', '--unittest-xml']
                 if os.environ.get('BROKEN_CUSTOM_ARGS_UNITTESTS') else [])

name = 'mcrt_denoise'

if 'early' not in locals() or not callable(early):
    def early(): return lambda x: x

@early()
def version():
    """
    Increment the build in the version.
    """
    _version = '6.23'
    from rezbuild import earlybind
    return earlybind.version(this, _version)

description = 'mcrt_denoise package'

authors = [
    'PSW Rendering and Shading',
    'moonbase-dev@dreamworks.com'
]

help = ('For assistance, '
        "please contact the folio's owner at: moonbase-dev@dreamworks.com")

variants = [
    [   # variant 0
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2023.1',
        'gcc-11.x'
    ],
    [   # variant 1
        'os-rocky-9',
        'opt_level-debug',
        'refplat-vfx2023.1',
        'gcc-11.x'
    ],
    [   # variant 2
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2023.1',
        'clang-17.0.6.x'
    ],
    [   # variant 3
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2024.0',
        'gcc-11.x'
    ],
    [   # variant 4
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2025.0',
        'gcc-11.x'
    ],
    [   # variant 5
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-houdini21.0',
        'gcc-11.x'
    ],
    [   # variant 6
        'os-rocky-9',
        'opt_level-optdebug',
        'refplat-vfx2022.0',
        'gcc-9.3.x.1'
    ],
]

conf_rats_variants = variants[0:2]
conf_CI_variants = variants

# Add ephemeral package to each variant.
for i, variant in enumerate(variants):
    variant.insert(0, '.mcrt_denoise_variant-%d' % i)

requires = [
  'cuda-12.1.0.x',
  'openimagedenoise-2.3.3.x',
  'optix-7.6.0.x',
  'scene_rdl2-15.22'
]

private_build_requires = [
    'cmake_modules-1.0',
    'cppunit',
]

commandstr = lambda i: "cd build/"+os.path.join(*variants[i])+"; ctest -j $(nproc)"
testentry = lambda i: ("variant%d" % i,
                       { "command": commandstr(i),
                        "requires": ["cmake"],
                        "on_variants": {
                            "type": "requires",
                            "value": [".mcrt_denoise_variant-%d" % i],
                            },
                        "run_on": "explicit",
                        }, )
testlist = [testentry(i) for i in range(len(variants))]
tests = dict(testlist)

def commands():
    prependenv('CMAKE_MODULE_PATH', '{root}/lib64/cmake')
    prependenv('LD_LIBRARY_PATH', '{root}/lib64')
    prependenv('PATH', '{root}/bin')

uuid = '47ff6209-1bfc-4c6f-92f0-0067563ffcb8'

config_version = 0
