import multiprocessing

cpus = multiprocessing.cpu_count()
SetOption('num_jobs', cpus)

SConscript(['src/Emulator/SConscript',
            'src/FrontEnd/SConscript'])