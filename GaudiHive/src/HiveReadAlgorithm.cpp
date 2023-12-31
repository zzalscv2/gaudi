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
#include <GaudiKernel/Algorithm.h>
#include <GaudiKernel/DataStoreItem.h>
#include <GaudiKernel/ThreadLocalContext.h>

class GAUDI_API HiveReadAlgorithm : public Algorithm {
public:
  using Algorithm::Algorithm;
  StatusCode initialize() override;
  StatusCode execute() override;
};

DECLARE_COMPONENT( HiveReadAlgorithm )

StatusCode HiveReadAlgorithm::initialize() {
  StatusCode sc = Algorithm::initialize();
  if ( !sc ) return sc;
  return evtSvc()->addPreLoadItem( DataStoreItem( "/Event", 99 ) );
}

StatusCode HiveReadAlgorithm::execute() {
  info() << "Running now for event " << Gaudi::Hive::currentContext().evt() << endmsg;
  return evtSvc()->preLoad();
}
