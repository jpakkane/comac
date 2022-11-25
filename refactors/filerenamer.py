#!/usr/bin/env python3

import os, pathlib, itertools, subprocess

def rename_files():
    root_dir = pathlib.Path('.')
    for f in itertools.chain(root_dir.glob('**/*.c'), root_dir.glob('**/*.h')):
        fname = f.name
        pathsegment = f.parent
        ifilename = str(f)
        ofilename = str(pathsegment / fname.replace('cairo', 'comac'))
        if ifilename != ofilename:
            subprocess.check_call(['git', 'mv', ifilename, ofilename])

def fix_includes():
    root_dir = pathlib.Path('.')
    for f in itertools.chain(root_dir.glob('**/*.c'), root_dir.glob('**/*.h')):
        lines = f.read_text().split('\n')
        out_lines = []
        for l in lines:
            if l.startswith('#include'):
                l = l.replace('cairo', 'comac')
            out_lines.append(l)
        f.write_text('\n'.join(out_lines))
    t = open('version.py').read().replace('cairo-version', 'comac-version')
    open('version.py', 'w').write(t)

def fix_build_files():
    root_dir = pathlib.Path('.')
    for f in root_dir.glob('**/meson.build'):
        lines = f.read_text().split('\n')
        out_lines = []
        for l in lines:
            l = l.replace('cairo-', 'comac-')
            out_lines.append(l)
        f.write_text('\n'.join(out_lines))

if __name__ == '__main__':
    rename_files()
    fix_includes()
    fix_build_files()
