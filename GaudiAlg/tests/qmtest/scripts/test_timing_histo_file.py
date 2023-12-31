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
Check the content of the ROOT file generated by the test 'gaudiexamples.timing_histos'.

The file must contain a directory called 'TIMER.TIMER' with 3 well defined histograms inside.
"""
from __future__ import print_function

import sys


def test():
    import ROOT

    filename = "timing_histos.root"

    toolname = "TIMER.TIMER"

    histograms = ["ElapsedTime", "CPUTime", "Count"]

    labels = [
        "EVENT LOOP                    ",
        " ParentAlg                    ",
        "  SubAlg1                     ",
        "  SubAlg2                     ",
        " StopperAlg                   ",
        " TopSequence                  ",
        "  Sequence1                   ",
        "   Prescaler1                 ",
        "   HelloWorld                 ",
        "   Counter1                   ",
        "  Sequence2                   ",
        "   Prescaler2                 ",
        "   Counter2                   ",
        " ANDSequence                  ",
        "  AND                         ",
        "  ANDCounter                  ",
        " ORSequence                   ",
        "  OR                          ",
        "  ORCounter                   ",
    ]

    f = ROOT.TFile.Open(filename)
    assert f, "Cannot open file %r" % filename

    d = f.Get(toolname)
    assert d, "Missing directory %r" % toolname

    for name in histograms:
        h = d.Get(name)
        assert h, "Missing histogram %r" % name
        l = list(h.GetXaxis().GetLabels())
        assert len(l) == len(
            labels
        ), "Wrong number of labels in histogram %r (found: %d, exp: %d)" % (
            name,
            len(l),
            len(labels),
        )
        for i, (expected, found) in enumerate(zip(labels, l)):
            assert (
                found == expected
            ), "Wrong label at position %d in histogram %r (found: %r, exp: %r)" % (
                i,
                name,
                found,
                expected,
            )


if __name__ == "__main__":
    try:
        test()
    except AssertionError as a:
        print("FAILURE:", a)
        sys.exit(1)
    print("SUCCESS")
