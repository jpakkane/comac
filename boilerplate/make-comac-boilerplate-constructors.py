#!/usr/bin/env python3

# IMPORTANT: Keep in sync with make-cairo-boilerplate-constructors.sh
#            and test/make-cairo-test-constructors.py!
import argparse
import sys
import re

if __name__=='__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('output')
    parser.add_argument('input', nargs='+')
    args = parser.parse_args()

    boilerplate_names = []

    match_boilerplate_line = re.compile(r'^COMAC_BOILERPLATE.*')
    match_boilerplate_name = re.compile(r'^COMAC_BOILERPLATE.*\((.*),.*')

    for fname in args.input:
        with open(fname, 'r', encoding='utf-8') as f:
            for l in f.readlines():
                if match_boilerplate_line.match(l):
                    boilerplate_names.append(match_boilerplate_name.match(l).group(1))

    with open(args.output, 'w', encoding='utf-8') as f:
        f.write('/* WARNING: Autogenerated file - see %s! */\n\n' % sys.argv[0])
        f.write('#include "comac-boilerplate-private.h"\n\n')
        f.write('void _comac_boilerplate_register_all (void);\n\n')

        for boilerplate_name in boilerplate_names:
            f.write('extern void _register_%s (void);\n' % boilerplate_name)

        f.write('\nvoid\n')
        f.write('_comac_boilerplate_register_all (void)\n')
        f.write('{\n')

        for boilerplate_name in boilerplate_names:
            f.write('    _register_%s ();\n' % boilerplate_name)
        f.write('}\n')
