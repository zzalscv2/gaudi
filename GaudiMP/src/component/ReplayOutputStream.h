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
#include <GaudiKernel/HashMap.h>
#include <GaudiKernel/SmartIF.h>
#include <GaudiKernel/ThreadLocalContext.h>

class IAlgManager;
struct IDataManagerSvc;

/** @class ReplayOutputStream ReplayOutputStream.h component/ReplayOutputStream.h
 *
 *
 * @author Marco Clemencic
 * @date 30/08/2013
 */
class ReplayOutputStream : public Algorithm {
public:
  /// Inherited constructor
  using Algorithm::Algorithm;

  StatusCode initialize() override; ///< Algorithm initialization
  StatusCode start() override;      ///< Algorithm initialization
  StatusCode execute() override;    ///< Algorithm execution
  StatusCode stop() override;       ///< Algorithm finalization
  StatusCode finalize() override;   ///< Algorithm finalization

  /// Class used to hold the OutputStream instances
  typedef GaudiUtils::HashMap<std::string, SmartIF<IAlgorithm>> OutStreamsMapType;

private:
  /// Add a new algorithm to the list of OutputStreams
  void i_addOutputStream( const Gaudi::Utils::TypeNameString& outStream );

  /// Helper class to fill the internal map of OutputStreams.
  class OutStreamAdder {
  public:
    OutStreamAdder( ReplayOutputStream* ptr ) : m_ptr( ptr ) {}
    inline void operator()( const Gaudi::Utils::TypeNameString& outStream ) { m_ptr->i_addOutputStream( outStream ); }

  private:
    ReplayOutputStream* m_ptr;
  };

  /// Helper function to call the transition on the contained OutputStreams.
  /// Returns StatusCode::FAILURE if any of the OutputStreams returned a failure.
  template <Gaudi::StateMachine::Transition TR>
  StatusCode i_outStreamTransition();

  Gaudi::Property<std::vector<std::string>> m_outputStreamNames{
      this, "OutputStreams", {}, "OutputStream instances that can be called." };

  /// Internal storage for the OutputStreams to call.
  OutStreamsMapType m_outputStreams;

  SmartIF<IAlgManager>     m_algMgr;
  SmartIF<IDataManagerSvc> m_evtMgr;
};
