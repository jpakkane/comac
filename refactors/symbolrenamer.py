#!/usr/bin/env python3

import os, pathlib, itertools, subprocess

def rename_symbols():
    root_dir = pathlib.Path('.')
    for f in itertools.chain(root_dir.glob('**/*.c'), root_dir.glob('**/*.h')):
        lines = f.read_text().split('\n')
        out_lines = []
        for l in lines:
            l = l.replace('CAIRO', 'COMAC').replace('Cairo', 'Comac').replace('cairo', 'comac')
            out_lines.append(l)
        f.write_text('\n'.join(out_lines))

if __name__ == '__main__':
    rename_symbols()
