##### Build and instal i_player
i_player_sources = [ 'dispmanx_window.c',
                       'player_interface.c',
                       'player_standalone.c'
                      ]

i_player_deps = [egl_dep, glib_dep, gstreamer_dep, gstreamer_player_dep, misc_deps]

executable('i_player', i_player_sources, dependencies : i_player_deps, include_directories : i_player_includedir, install: true)
