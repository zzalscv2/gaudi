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
# GaudiConfiguration subdirectory

# Install python modules
gaudi_install(PYTHON)

# FIXME: with nosetests we were using --cover-min-percentage=100, but it is not available with gaudi_add_pytest
gaudi_add_pytest(tests/python)

# Note: The file tools/print_limits.cpp is a ROOT macro, so it is not meant to be compiled
