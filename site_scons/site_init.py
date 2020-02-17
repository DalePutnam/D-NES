'''Utility functions for the SConstruct file'''

import os
import subprocess
from functools import wraps

from SCons.Variables import PathVariable, EnumVariable


class LibraryRegistry:
    def __init__(self):
        self.dict = {}

    def has_library(self, libname):
        return libname in self.dict

    def add_library(self, libname, flags):
        self.dict[libname] = flags

    def get_library_build_flags(self, libname):
        if libname not in self.dict:
            return None
        else:
            return self.dict[libname]


def add_library_variables(vars, libname, root_only=False):
    '''Add variables for finding the given library'''
    libname = libname.lower()

    vars.Add(PathVariable('%s_lib_root' % libname,
                          'Path to %s install root' % libname,
                          None, PathVariable.PathIsDir))

    if not root_only:
        vars.Add(PathVariable('%s_lib' % libname,
                              'Path to %s library' % libname,
                              None, PathVariable.PathIsFile))

        vars.Add(PathVariable('%s_include_dir' % libname,
                              'Path to %s include directory' % libname,
                              None, PathVariable.PathIsDir))


def get_library_options(env, libname, lib_pkgconfig_name=None, root_only=False):
    '''Get library compile flags based on command line variables'''

    if not lib_pkgconfig_name:
        lib_pkgconfig_name = libname

    libname = libname.lower()

    var_root_path = '%s_lib_root' % libname
    var_lib_path = '%s_lib' % libname
    var_include_path = '%s_include_dir' % libname

    root_path = env[var_root_path] if var_root_path in env else None
    lib = None
    include = None

    pkgconfig_args = ['pkg-config', lib_pkgconfig_name, '--cflags', '--libs']

    if not root_only:
        lib = env[var_lib_path] if var_lib_path in env else None
        include = env[var_include_path] if var_include_path in env else None

    flags = None

    try:
        if root_path:
            pkgconfig_path = os.path.join(root_path, 'lib/pkgconfig')
            flags = env.ParseFlags(subprocess.check_output(
                pkgconfig_args,
                env=dict(os.environ, PKG_CONFIG_PATH=pkgconfig_path),
                stderr=subprocess.STDOUT))
        elif lib or include:
            flags = {}

            if lib:
                flags['LIBS'] = [File(lib)]

            if include:
                flags['CPPPATH'] = [include]
        else:
            flags = env.ParseFlags(subprocess.check_output(
                pkgconfig_args, stderr=subprocess.STDOUT))
    except:
        pass

    return flags


def get_wxwidgets_options(env):
    '''Get wxWidgets info from wx-config'''
    wxconfig = env['wx_config_path'] if 'wx_config_path' in env else None

    if not wxconfig:
        wxconfig = 'wx-config'

    version = None
    flags = None

    try:
        version = subprocess.check_output(
            [wxconfig, '--version'], stderr=subprocess.STDOUT)
        flags = env.ParseFlags(subprocess.check_output(
            [wxconfig, '--toolkit=gtk3', '--cflags', '--libs'], stderr=subprocess.STDOUT))
    except:
        pass

    return version, flags


def isolated_check(func):
    '''Allows running configuration checks without affecting outer environment'''
    @wraps(func)
    def isolate_check(context, *args, **kwargs):
        oldenv = context.sconf.env
        context.sconf.env = oldenv.Clone()

        result = func(context, *args, **kwargs)

        context.sconf.env = oldenv

        return result

    return isolate_check


_LINK_TEST_TEXT = '''
int main() {
    return 0;
}
'''

_HEADER_TEST_TEXT = '''
#include <%s>

int main() {
    return 0;
}
'''


@isolated_check
def check_lib_plus(context, lib, flags):
    '''Check if given library can be linked using the given flags'''
    context.env.MergeFlags(flags)

    context.Message("Checking for library %s..." % lib)

    if not context.TryLink(_LINK_TEST_TEXT, ".cpp"):
        context.Result(False)
        return False

    context.Result(True)
    return True


@isolated_check
def check_header_plus(context, header, flags):
    '''Check if given header can be compiled using the given flags'''
    context.env.MergeFlags(flags)

    context.Message("Checking for header %s..." % header)

    if not context.TryCompile(_HEADER_TEST_TEXT % header, ".cpp"):
        context.Result(False)
        return False

    context.Result(True)
    return True


@isolated_check
def check_wxwidgets(context, version, flags):
    '''Check for the correct version of wxWidgets and check that it can be linked'''
    context.Message("Checking for wxWidgets >= 3.0.4...")

    if not version or not flags:
        context.Result(False)
        return False

    major, minor, subminor = [int(v) for v in version.split('.')]

    if major != 3:
        context.Result(False)
        return False

    if (minor == 0 and subminor < 4):
        context.Result(False)
        return False

    context.env.MergeFlags(flags)

    if not context.TryLink(_LINK_TEST_TEXT, ".cpp"):
        context.Result(False)
        return False

    context.Result(True)
    return True
