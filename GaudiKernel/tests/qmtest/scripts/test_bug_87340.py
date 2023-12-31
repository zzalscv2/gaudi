#!/usr/bin/env python3
#####################################################################################
# (c) Copyright 1998-2019 CERN for the benefit of the LHCb and ATLAS collaborations #
#                                                                                   #
# This software is distributed under the terms of the Apache version 2 licence,     #
# copied verbatim in the file "LICENSE".                                            #
#                                                                                   #
# In applying this licence, CERN does not waive the privileges and immunities       #
# granted to it by virtue of its status as an Intergovernmental Organization        #
# or submit itself to any jurisdiction.                                             #
#####################################################################################
"""
Test that the there is no signature for MsgStream operator<< <char []> in
GaudiKernel.
See https://savannah.cern.ch/bugs?87340
"""
from __future__ import print_function

import os
import re
import sys
from subprocess import PIPE, Popen

# find libGaudiKernel.so in the LD_LIBRARY_PATH
libname = "libGaudiKernel.so"

searchpath = [os.path.curdir, "lib"]
# The day we can run the test on other platforms we can do this:
# varname = {'darwin': 'DYLD_LIBRARY_PATH',
#           'win32': 'PATH'}.get(sys.platform, 'LD_LIBRARY_PATH')
varname = "LD_LIBRARY_PATH"
searchpath.extend(os.environ.get(varname, "").split(os.pathsep))

try:
    lib = next(
        p for p in (os.path.join(n, libname) for n in searchpath) if os.path.exists(p)
    )
except StopIteration:
    print("FAILURE: Cannot find", repr(libname), "in", searchpath, file=sys.stderr)
    sys.exit(2)

nm = Popen(["nm", "-C", lib], stdout=PIPE)
output, _ = nm.communicate()
output = output.decode("utf-8")

if nm.returncode:
    print(output)
    print("FAILURE: nm call failed", file=sys.stderr)
    sys.exit(nm.returncode)

signature = re.compile(r"MsgStream&amp; operator&lt;&lt; &lt;char \[\d+\]&gt;")

lines = list(filter(signature.search, output.splitlines()))
if lines:
    print("\n".join(lines))
    print("FAILURE: found MsgStream operator<< specialization", file=sys.stderr)
    sys.exit(1)
else:
    print("SUCCESS: no MsgStream operator<< specialization found")
