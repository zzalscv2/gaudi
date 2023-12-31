/***********************************************************************************\
* (c) Copyright 1998-2019 CERN for the benefit of the LHCb and ATLAS collaborations *
*                                                                                   *
* This software is distributed under the terms of the Apache version 2 licence,     *
* copied verbatim in the file "LICENSE".                                            *
*                                                                                   *
* In applying this licence, CERN does not waive the privileges and immunities       *
* granted to it by virtue of its status as an Intergovernmental Organization        *
* or submit itself to any jurisdiction.                                             *
\***********************************************************************************/
// Include files

// local
#include "ReadTES.h"

//-----------------------------------------------------------------------------
// Implementation file for class : ReadTES
//
// 2008-11-03 : Marco Cattaneo
//-----------------------------------------------------------------------------

// Declaration of the Algorithm Factory
DECLARE_COMPONENT( ReadTES )

//=============================================================================
// Initialization
//=============================================================================
StatusCode ReadTES::initialize() {
  StatusCode sc = Algorithm::initialize(); // must be executed first
  if ( sc.isFailure() ) return sc;         // error printed already by Algorithm

  if ( msgLevel( MSG::DEBUG ) ) debug() << "==> Initialize" << endmsg;

  if ( m_locations.empty() ) {
    error() << "You must define at least one TES Location" << endmsg;
    return StatusCode::FAILURE;
  }

  return StatusCode::SUCCESS;
}
//=============================================================================
// Main execution
//=============================================================================
StatusCode ReadTES::execute() {

  if ( msgLevel( MSG::DEBUG ) ) debug() << "==> Execute" << endmsg;

  for ( auto& loc : m_locations ) {
    DataObject* pTES = nullptr;
    eventSvc()->retrieveObject( loc, pTES ).ignore();
    info() << "Found object " << loc << " at " << pTES << endmsg;
  }

  return StatusCode::SUCCESS;
}

//=============================================================================
