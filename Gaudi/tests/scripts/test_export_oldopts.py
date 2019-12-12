#!/usr/bin/env python

from __future__ import print_function
import os
import sys
import tempfile
from subprocess import Popen, PIPE
from hashlib import sha1


def which(file, path=None):
    from os import environ, pathsep
    from os.path import join, isfile
    if path is None:
        path = environ.get("PATH", "")
    path = path.split(pathsep)
    for p in path:
        if isfile(join(p, file)):
            return join(p, file)
    return None


retcode = 0
try:

    if len(sys.argv) > 1:
        optfiles = sys.argv[1:]
    else:
        optfiles = ["main.py"]
    hash = sha1(str(optfiles).encode('utf-8')).hexdigest()
    outname = 'out-{0}'.format(hash[:8])

    # parse the option file and cache the configuration (python only)
    cmd = [
        "python",
        which("gaudirun.py"), "-n", "-v", "--output", outname + ".1.py"
    ] + optfiles
    proc = Popen(cmd, stdout=PIPE)
    print("==========================================")
    print("======== First pass (python only) ========")
    print("==========================================")
    print("= cmd:", " ".join(cmd))
    out, err = proc.communicate()
    print(out.decode('utf-8'))
    if err:
        print("=== stderr: ===")
        print(err.decode('utf-8'))
    expected = eval(open(outname + ".1.py").read())

    # parse the option file, export old options, parse again
    cmd = [
        "python",
        which("gaudirun.py"), "-n", "-v", "--old-opts", "--output",
        outname + '.opts'
    ] + optfiles
    proc = Popen(cmd, stdout=PIPE, stderr=PIPE)
    print("")
    print("==========================================")
    print("========== Second pass (export) ==========")
    print("==========================================")
    print("= cmd:", " ".join(cmd))
    out, err = proc.communicate()
    print(out.decode('utf-8'))
    if err:
        print("=== stderr: ===")
        print(err.decode('utf-8'))

    cmd = [
        "python",
        which("gaudirun.py"), "-n", "-v", "--output", outname + ".2.py",
        outname + '.opts'
    ]
    proc = Popen(cmd, stdout=PIPE, stderr=PIPE)
    print("")
    print("==========================================")
    print("========== Second pass (import) ==========")
    print("==========================================")
    print("= cmd:", " ".join(cmd))
    out, err = proc.communicate()
    print(out.decode('utf-8'))
    if err:
        print("=== stderr: ===")
        print(err.decode('utf-8'))
    result = eval(open(outname + ".2.py").read())

    if result != expected:
        print("Configuration from old options differs from the python one")
        retcode = 1

except RuntimeError as x:
    print(x)
    retcode = 1

sys.exit(retcode)
