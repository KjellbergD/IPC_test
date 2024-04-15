project('pipeline', 'c')

m_dep = meson.get_compiler('c').find_library('m', required : false)
deps = [m_dep]

sender = executable('ipc_sender', files('src/ipc_sender.c'), 
            dependencies : deps,
            install : true,
            )

reviever = executable('ipc_receiver', files('src/ipc_receiver.c'), 
            dependencies : deps,
            install : true,
            )