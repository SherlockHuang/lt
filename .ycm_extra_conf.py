import os

SOURCE_EXTENSIONS_C = [ '.cc', '.c' ]
SOURCE_EXTENSIONS_CPP = [ '.cpp', '.cxx' ]
SOURCE_EXTENSIONS_OC = [ '.m' ]
SOURCE_EXTENSIONS_OCPP = [ '.mm' ]
HEADER_EXTENSIONS = [ '.h', '.hpp', 'inl' ]

def ParserHeader(basename, ext, flags):
    for ext in SOURCE_EXTENSIONS_C:
        if os.path.exists(basename + ext):
            flags.extend(['-x', 'c'])
            return

    for ext in SOURCE_EXTENSIONS_CPP:
        if os.path.exists(basename + ext):
            flags.extend(['-x', 'c++'])
            return

    for ext in SOURCE_EXTENSIONS_OC:
        if os.path.exists(basename + ext):
            flags.extend(['-x', 'objc'])
            return

    for ext in SOURCE_EXTENSIONS_OCPP:
        if os.path.exists(basename + ext):
            flags.extend(['-x', 'objcpp'])
            return

    flags.extend(['-x', 'c++'])

def Settings(**kwargs):
    flags = []

    filename = kwargs['filename']
    basename = os.path.splitext(filename)[0]
    ext = os.path.splitext(filename)[1]

    if ext in HEADER_EXTENSIONS:
        ParserHeader(basename, ext, flags)
    elif ext in SOURCE_EXTENSIONS_CPP:
        flags.extend(['-x', 'c++'])
    elif ext in SOURCE_EXTENSIONS_OC:
        flags.extend(['-x', 'objc'])
    elif ext in SOURCE_EXTENSIONS_OCPP:
        flags.extend(['-x', 'objcpp'])
    else:
        flags.extend(['-x', 'c++'])

    cur_dir = os.path.dirname(os.path.abspath(__file__))
    flags.extend([
        '-Wall',
        '-I/usr/local/opt/llvm/include',
        '-I/usr/local/include',
        '-I' + cur_dir,
        '-std=c++17',
    ])


    return {
        'flags': flags
    }
