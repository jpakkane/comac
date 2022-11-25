#!/usr/bin/env python3
#
# cairo version.py
#
# Extracts the version from comac-version.h for the meson build files.
#
import os
import sys

if __name__ == '__main__':
    srcroot = os.path.dirname(__file__)

    version_major = None
    version_minor = None
    version_micro = None

    f = open(os.path.join(srcroot, 'src', 'comac-version.h'), 'r', encoding='utf-8')
    for line in f:
        if line.startswith('#define COMAC_VERSION_MAJOR '):
            version_major = line[28:].strip()
        if line.startswith('#define COMAC_VERSION_MINOR '):
            version_minor = line[28:].strip()
        if line.startswith('#define COMAC_VERSION_MICRO '):
            version_micro = line[28:].strip()
    f.close()

    if not (version_major and version_minor and version_micro):
       print('ERROR: Could not extract comac version from comac-version.h in', srcroot, file=sys.stderr)
       sys.exit(-1)

    print('{0}.{1}.{2}'.format(version_major, version_minor, version_micro))
