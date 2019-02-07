// This is allow implementation of deprecated methods beginRun and endRun
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "GaudiKernel/IAlgManager.h"
#include <Gaudi/Sequence.h>

using Gaudi::Sequence;

namespace {
  template <StatusCode ( Gaudi::Algorithm::*f )(), typename C>
  bool for_algorithms( C& c ) {
    return std::accumulate( std::begin( c ), std::end( c ), true,
                            []( bool b, Gaudi::Algorithm* a ) { return ( a->*f )().isSuccess() && b; } );
  }
} // namespace

StatusCode Sequence::initialize() {
  if ( !Algorithm::initialize() ) return StatusCode::FAILURE;

  // initialize sub-algorithms
  if ( !for_algorithms<&Algorithm::sysInitialize>( m_subAlgms ) ) {
    error() << "error initializing one or several sub-algorithms of Sequence " << name() << endmsg;
    return StatusCode::FAILURE;
  }

  return StatusCode::SUCCESS;
}

StatusCode Sequence::finalize() {
  // Bypass the finalialization if the algorithm hasn't been initilized.
  // Note: this check is also in Gaudi::Algorithm::sysFinalize
  if ( Gaudi::StateMachine::CONFIGURED == FSMState() || !isEnabled() ) return StatusCode::SUCCESS;

  // finalize sub-algorithms
  if ( !for_algorithms<&Algorithm::sysFinalize>( m_subAlgms ) ) {
    error() << "error finalizing one or several sub-algorithms of Sequence " << name() << endmsg;
    return StatusCode::FAILURE;
  }

  // Release all sub-algorithms
  for ( auto& it : m_subAlgms ) it->release();
  m_subAlgms.clear();

  return Algorithm::finalize();
}

StatusCode Sequence::start() {
  if ( !Algorithm::start() ) return StatusCode::FAILURE;

  // start sub-algorithms
  if ( !for_algorithms<&Algorithm::sysStart>( m_subAlgms ) ) {
    error() << "error starting one or several sub-algorithms of Sequence " << name() << endmsg;
    return StatusCode::FAILURE;
  }

  return StatusCode::SUCCESS;
}

StatusCode Sequence::stop() {
  // stop sub-algorithms
  if ( !for_algorithms<&Algorithm::sysStop>( m_subAlgms ) ) {
    error() << "error stopping one or several sub-algorithms of Sequence " << name() << endmsg;
    return StatusCode::FAILURE;
  }

  return Algorithm::stop();
}

StatusCode Sequence::reinitialize() {
  // re-initialize sub-algorithms
  if ( !for_algorithms<&Algorithm::sysReinitialize>( m_subAlgms ) ) {
    error() << "error re-initializing one or several sub-algorithms of Sequence " << name() << endmsg;
    return StatusCode::FAILURE;
  }

  return Algorithm::reinitialize();
}

StatusCode Sequence::restart() {
  // re-start sub-algorithms
  if ( !for_algorithms<&Algorithm::sysRestart>( m_subAlgms ) ) {
    error() << "error re-restarting one or several sub-algorithms of Sequence " << name() << endmsg;
    return StatusCode::FAILURE;
  }

  return Algorithm::restart();
}

StatusCode Sequence::beginRun() {
  if ( !Algorithm::beginRun() ) return StatusCode::FAILURE;

  // call beginRun for sub-algorithms
  if ( !for_algorithms<&Algorithm::sysBeginRun>( m_subAlgms ) ) {
    error() << "error calling beginRun for one or several sub-algorithms of Sequence " << name() << endmsg;
    return StatusCode::FAILURE;
  }

  return StatusCode::SUCCESS;
}

StatusCode Sequence::endRun() {
  // call endRun for sub-algorithms
  if ( !for_algorithms<&Algorithm::sysEndRun>( m_subAlgms ) ) {
    error() << "error calling endRun for one or several sub-algorithms of Sequence " << name() << endmsg;
    return StatusCode::FAILURE;
  }

  return Algorithm::endRun();
}

void Sequence::acceptDHVisitor( IDataHandleVisitor* vis ) const {
  Algorithm::acceptDHVisitor( vis );

  // loop through sub-algs
  for ( auto alg : *subAlgorithms() ) vis->visit( alg );
}

const std::vector<Gaudi::Algorithm*>* Sequence::subAlgorithms() const { return &m_subAlgms; }

std::vector<Gaudi::Algorithm*>* Sequence::subAlgorithms() { return &m_subAlgms; }

StatusCode Sequence::createSubAlgorithm( const std::string& type, const std::string& name, Algorithm*& pSubAlgorithm ) {
  SmartIF<IAlgManager> am( serviceLocator() );
  if ( !am ) return StatusCode::FAILURE;

  // Maybe modify the AppMgr interface to return Algorithm* ??
  IAlgorithm* tmp;
  StatusCode  sc = am->createAlgorithm( type, name, tmp );
  if ( sc.isFailure() ) return StatusCode::FAILURE;

  try {
    pSubAlgorithm = dynamic_cast<Algorithm*>( tmp );
    m_subAlgms.push_back( pSubAlgorithm );
  } catch ( ... ) { sc = StatusCode::FAILURE; }
  return sc;
}