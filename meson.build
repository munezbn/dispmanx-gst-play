project('i_player', 'c',
        version : '1.0.0',
        meson_version: '>=0.40',
        license: 'MIT'
    )

i_player_compiler = meson.get_compiler('c')
i_player_build_type = get_option('build-type')

###############
# Directories #
###############

i_player_top_srcdir = meson.source_root()
i_player_includedir = include_directories('include')

#################### End Directories ####################

##################
# Module Imports #
##################

####################### End Module Imports #######################

#########################
# External Dependencies #
#########################

glib_dep = dependency('glib-2.0', version : '>= 2.26.0')
gstreamer_dep = dependency('gstreamer-1.0')
gstreamer_player_dep = dependency('gstreamer-player-1.0', version : '>= 1.7.1.1')
egl_dep = dependency('egl')

misc_deps = declare_dependency(link_args : ['-lpthread', '-lm'])

############################## End External Dependencies ##############################

###############################
# Compiler and Linker options #
###############################

i_player_linker_flag = ['-Wl,--as-needed', '-Wl,--no-undefined']

i_player_compiler_flag  = ['-std=gnu99']
i_player_compiler_flag += ['-rdynamic', '-fvisibility=hidden']
i_player_compiler_flag += ['-Wall', '-Werror=all']
i_player_compiler_flag += ['-Wfloat-equal', '-Wformat=2', '-Wunreachable-code', '-Wparentheses', '-Wundef', '-Wuninitialized', '-Winline', '-Wconversion', '-Wswitch']
i_player_compiler_flag += ['-Wunused', '-Wcast-align', '-Wimplicit', '-Wmissing-prototypes', '-Wshadow', '-Wmissing-braces', '-Wimplicit', '-Wimplicit-function-declaration']
i_player_compiler_flag += ['-Wconversion', '-Wsign-conversion', '-Wimplicit-function-declaration', '-Wpointer-to-int-cast', '-Wint-to-pointer-cast']

if i_player_compiler.has_argument('-Wcast-function-type')
i_player_compiler_flag += ['-Wcast-function-type']
endif

if i_player_build_type == 'debug'
i_player_compiler_flag += ['-g']
endif

add_project_arguments([i_player_compiler_flag, i_player_linker_flag], language : 'c')

#################################### End compiler and Linker options ####################################


#########
# Build #
#########

subdirs = [ 'src', 'include' ]

foreach n : subdirs
    subdir(n)
endforeach

############## End Build ##############
