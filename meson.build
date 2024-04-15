project('pipeline', 'c')

dirs = include_directories('src/include')
m_dep = meson.get_compiler('c').find_library('m', required : false)
deps = [m_dep]

sender = executable('ipc_sender', files('src/ipc_sender.c'), 
            include_directories : dirs,
            dependencies : deps,
            install : true,
            )

reviever = executable('ipc_receiver', files('src/ipc_receiver.c'), 
            include_directories : dirs,
            dependencies : deps,
            install : true,
            )