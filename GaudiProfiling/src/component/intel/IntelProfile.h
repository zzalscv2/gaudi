/***********************************************************************************\
* (c) Copyright 1998-2023 CERN for the benefit of the LHCb and ATLAS collaborations *
*                                                                                   *
* This software is distributed under the terms of the Apache version 2 licence,     *
* copied verbatim in the file "LICENSE".                                            *
*                                                                                   *
* In applying this licence, CERN does not waive the privileges and immunities       *
* granted to it by virtue of its status as an Intergovernmental Organization        *
* or submit itself to any jurisdiction.                                             *
\***********************************************************************************/
#pragma once

#include <GaudiKernel/Algorithm.h>
#include <ittnotify.h>

/** Algorithm to enable/disable the profiling by Intel at given events.
 *  Works with vtune and intel advisor. Not working with inspector.
 *
 *  @author Ben Couturier Sebastien Ponce
 */
class IntelProfile : public Algorithm {
public:
  using Algorithm::Algorithm;
  StatusCode execute() override; ///< Algorithm execution

private:
  Gaudi::Property<int> m_nStartFromEvent{ this, "StartFromEventN", 1, "After what event we start profiling." };
  Gaudi::Property<int> m_nStopAtEvent{
      this, "StopAtEventN", 0,
      "After what event we stop profiling. If 0 than we also profile finalization stage. Default = 0." };

  int m_eventNumber = 0; // Current event number
};
