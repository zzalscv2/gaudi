#!/usr/bin/env python
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
"""Determine critical path for a given precedence trace."""
from __future__ import print_function

__author__ = "Illya Shapoval"

import sys
import argparse

# FIXME: workaround for the old version of networkx in LCG 100
import warnings
warnings.filterwarnings(
    "ignore", message='"is" with a literal', category=SyntaxWarning)

import networkx as nx


def get_critical_path(path_to_trace_file):
    """Find critical path, print algorithms on it and its length."""

    assert (tuple(map(int, nx.__version__.split("."))) >=
            (2, 0)), "This script requires Networkx version 2.0 or higher"

    trace = nx.read_graphml(path_to_trace_file)

    for inNode, outNode, edge_attrs in trace.in_edges(data=True):
        edge_attrs['Runtime'] = nx.get_node_attributes(trace,
                                                       'Runtime')[outNode]

    cpath = nx.algorithms.dag.dag_longest_path(trace, weight='Runtime')

    print("Algorithms on the critical path (%i): " % len(cpath))

    print("  {:<40} Runtime (ns)".format("Name"))
    print("  -----------------------------------------------------")
    for node_id in cpath:
        print("  {:<40}: {}".format(trace.node[node_id].get('Name'),
                                    trace.node[node_id].get('Runtime')))

    print("\nTotal critical path time: ",
          nx.algorithms.dag.dag_longest_path_length(trace, weight='Runtime'),
          "ns")


def main():

    parser = argparse.ArgumentParser(
        description=
        "Determine critical path for a precedence trace generated by the Avalanche Scheduler."
    )
    parser.add_argument(
        "path_to_trace_file",
        help="Path to GRAPHML precedence trace file.",
        type=str)
    args = parser.parse_args()

    get_critical_path(args.path_to_trace_file)


if __name__ == '__main__':
    sys.exit(main())
