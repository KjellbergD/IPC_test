project('pipeline', 'c')

c_args = []

sender = executable('sender', files('src/ipc_sender.c'), 
            install : true,
            c_args: c_args)

reviever = executable('reciever', files('src/ipc_reciever.c'), 
            install : true,
            c_args : c_args)