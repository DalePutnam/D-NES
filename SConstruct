import os
import multiprocessing


def setup_environment(env):
    # Add extra tools
    env.Tool('compilation_db')

    # Load some environment variables
    env_vars = ['CXX', 'CFLAGS', 'CXXFLAGS', 'LDFLAGS']
    for var in env_vars:
        if var in os.environ:
            env[var] = os.environ[var]

    # Add variables for controlling the configuration
    vars = Variables('scons.cfg')

    vars.Add(EnumVariable('build_type',
                          'Select the build type',
                          'release', allowed_values=['release', 'debug']))

    vars.Add('prefix', 'Application install prefix', '/usr/local')

    vars.Add(PathVariable('wx_config', 
                          'Path to the wx-config executable for wxWidgets',
                          None, PathVariable.PathIsFile))

    add_library_variables(vars, 'alsa')
    add_library_variables(vars, 'x11')
    add_library_variables(vars, 'gl')
    add_library_variables(vars, 'gtk3', root_only=True)

    vars.Update(env)
    vars.Save('scons.cfg', env)

    # scons.cfg will be cleaned if the config target is cleaned
    env.Clean(env.Alias('config'), 'scons.cfg')


    # Add an alias for the install prefix. Calling scons with this alias will
    # install to the location specified by prefix.
    env.Alias('install', env['prefix'])

    # Setup a debug or release build
    if env['build_type'] == 'debug':
        env.Append(CXXFLAGS=['-g'])
    else:
        env.Append(CXXFLAGS=['-O3'])

    # Add common flags
    env.Append(CXXFLAGS=['-Werror'])

    # Add compilation database target
    env.CompilationDatabase('compile_commands.json')
    env.Alias('compile_db', 'compile_commands.json')
    env.Default('compile_db')

    # Setup build help

    env.Help('\nD-NES SCons build system\n')
    env.Help('\nTargets: app, core, compile_db, install, config (defaults: app, core, compile_db)\n')

    env.Help('\n')
    env.Help('app         Build the D-NES application\n')
    env.Help('core        Build the dnes shared library\n')
    env.Help('compile_db  Generate compile_commands.json\n')
    env.Help('install     Install files to location specified by prefix\n')
    env.Help('config      Cleaning with this target will reset all configuration variables\n')

    env.Help('\nBuild Variables\n')

    env.Help(vars.GenerateHelpText(env))


def do_configuration(env, lib_registry):
    conf = Configure(env, custom_tests={'CheckLibPlus': check_lib_plus,
                                        'CheckHeaderPlus': check_header_plus,
                                        'CheckWxWidgets': check_wxwidgets})

    # Check that the compiler is installed and working
    if not conf.CheckProg(env['CXX']):
        print("Could not find specified compiler")
        Exit(1)

    if not conf.CheckCXX():
        print("No C++ compiler found")
        Exit(1)

    # Get the compile flags for each library
    alsa_flags = get_library_options(env, 'alsa')
    x11_flags = get_library_options(env, 'x11')
    gl_flags = get_library_options(env, 'gl')
    gtk3_flags = get_library_options(env, 'gtk3', lib_pkgconfig_name='gtk+-3.0', root_only=True)

    wx_version, wx_flags = get_wxwidgets_options(env)

    # Check that ALSA is installed
    if not conf.CheckLibPlus('asound', alsa_flags):
        print('Unable to find ALSA')
        Exit(1)

    if not conf.CheckHeaderPlus('alsa/asoundlib.h', alsa_flags):
        print("Unable to find ALSA headers")
        Exit(1)

    lib_registry.add_library('alsa', alsa_flags)

    # Check that X11 is installed
    if not conf.CheckLibPlus('X11', x11_flags):
        print('Unable to find X11')
        Exit(1)

    if not conf.CheckHeaderPlus('X11/Xlib.h', x11_flags):
        print("Unable to find X11 headers")
        Exit(1)

    lib_registry.add_library('X11', x11_flags)

    # Check that GL is installed
    if not conf.CheckLibPlus('GL', gl_flags):
        print('Unable to find GL')
        Exit(1)

    if not conf.CheckHeaderPlus('GL/gl.h', gl_flags) or not conf.CheckHeaderPlus('GL/glx.h', gl_flags):
        print("Unable to find GL headers")
        Exit(1)

    lib_registry.add_library('GL', gl_flags)

    # Check that GTK+3 is installed
    if not conf.CheckLibPlus('GTK+3', gtk3_flags):
        print("Unable to find GTK+-3.0")
        Exit(1)

    lib_registry.add_library('GTK+3', gtk3_flags)

    # Check that the correct version of wxWidgets is installed
    if not conf.CheckWxWidgets(wx_version, wx_flags):
        print('Unable to find wxWidgets 3.0.4 or later')
        Exit(1)

    lib_registry.add_library('wxWidgets', wx_flags)

    env = conf.Finish()


# Utilize all cores on the machine
SetOption('num_jobs', multiprocessing.cpu_count())

lib_registry = LibraryRegistry()
env = Environment()
setup_environment(env)

skip_config = GetOption('help') or GetOption('clean')

if not skip_config:
    print('Building in %s mode' % env['build_type'])
    do_configuration(env, lib_registry)

SConscript(['source/core/SConscript',
            'source/app/SConscript'],
            exports=['env', 'lib_registry'])
