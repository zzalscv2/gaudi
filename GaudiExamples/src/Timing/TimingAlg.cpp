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
// ============================================================================
// Include files
// ============================================================================
// STD & STL
// ============================================================================
#include <cmath>
// ============================================================================
// GaudiKernel
// ============================================================================
#include "GaudiKernel/Chrono.h"
#include "GaudiKernel/IRndmGenSvc.h"
#include "GaudiKernel/RndmGenerators.h"
// ============================================================================
// GaudiAlg
// ============================================================================
#include "GaudiKernel/Algorithm.h"
// ============================================================================
// Boost
// ============================================================================
#include "boost/format.hpp"
// ============================================================================
namespace GaudiExamples {
  // ==========================================================================
  /** @class TimingAlg
   *
   *  The primitive class to illustrate the usage of
   *    IChronoSvc , ChronoEntity & Chrono objects
   *  @see IChronoSvc
   *  @see  ChronoStat
   *  @see  Chrono
   *  @author Vanya BELYAEV Ivan.Belyaev@nikhef.nl
   *  @date 2008-04-02
   */
  class TimingAlg : public Algorithm {
  public:
    // ========================================================================
    /// standard constructor
    using Algorithm::Algorithm;
    // ========================================================================
    /// the execution of the algorithm
    StatusCode execute() override; // the execution of the algorithm
    /// the finalization of the algorithm
    StatusCode finalize() override; // the finalization of the algorithm
    // ========================================================================
  protected:
    // ========================================================================
    /// do something CPU-intensive
    double doSomething();
    // ========================================================================
  private:
    // ========================================================================
    /// the length of the internal loops (property)
    Gaudi::Property<unsigned long> m_cycles{ this, "Cycles", 10000, "The number of cycles" };
    // ========================================================================
  private:
    // ========================================================================
    ChronoEntity m_chrono1;
    ChronoEntity m_chrono2;
    ChronoEntity m_chrono3;
    // ========================================================================
  };
  // ==========================================================================
} // end of namespace GaudiExamples
// ============================================================================
// do something CPU-intensive
// ============================================================================
double GaudiExamples::TimingAlg::doSomething() {
  double        result = 0;
  Rndm::Numbers gauss( randSvc(), Rndm::Gauss( 0.0, 1.0 ) );
  for ( unsigned long i = 0; i < m_cycles; ++i ) result += sin( gauss() );
  return result;
}
// ========================================================================
// the execution of the algorithm
// ============================================================================
StatusCode GaudiExamples::TimingAlg::execute() // the execution of the algorithm
{
  //
  { // make some cpu consuming evaluations:
    Chrono chrono( chronoSvc(), name() + ": the first loop" );
    debug() << "the result of the first  loop is " << doSomething() << endmsg;
  }
  //
  { // make some cpu consuming evaluations:
    Chrono chrono( name() + ": the second loop", chronoSvc() );
    debug() << "the result of the second loop is " << doSomething() << endmsg;
  }
  //
  { // make some cpu consuming evaluations:
    Chrono chrono( &m_chrono1 );
    debug() << "the result of the third  loop is " << doSomething() << endmsg;
  }
  //
  { // make some cpu consuming evaluations:
    Chrono chrono( m_chrono2 );
    debug() << "the result of the fourth loop is " << doSomething() << endmsg;
  }
  //
  { // make some cpu consuming evaluations:
    m_chrono3.start();
    debug() << "the result of the 5th    loop is " << doSomething() << endmsg;
    m_chrono3.stop();
  }
  //
  { // make some cpu consuming evaluations:
    chronoSvc()->chronoStart( name() + ": the 5th loop" );
    debug() << "the result of the 5th    loop is " << doSomething() << endmsg;
    chronoSvc()->chronoStop( name() + ": the 5th loop" );
  }
  //
  return StatusCode::SUCCESS;
}
// ============================================================================
// the finalization of the algorithm
// ============================================================================
StatusCode GaudiExamples::TimingAlg::finalize() // the finalization of the algorithm
{
  const std::string format = "| %1$-6d| %2$-12.5g| %3$12.5g+-%4$-12.5g|%6$12.5g/%7$-12.5g|";
  const std::string header = "|        |%1$=7.7s|%2$=13.13s|%3$13.13s+-%4$-12.12s|%6$12.12g/%7$-12.12s|";

  using namespace boost::io;
  boost::format hdr( header );
  hdr.exceptions( all_error_bits ^ ( too_many_args_bit | too_few_args_bit ) );

  hdr % "#" % "Total" % "Mean" % "RMS" % "ErrMean" % "Min" % "Max";

  always() << "The timing is (in us)" << std::endl
           << hdr.str() << std::endl
           << "|  (1U)  " << m_chrono1.outputUserTime( format, System::microSec ) << std::endl
           << "|  (2U)  " << m_chrono2.outputUserTime( format, System::microSec ) << std::endl
           << "|  (3U)  " << m_chrono3.outputUserTime( format, System::microSec ) << endmsg;

  return Algorithm::finalize();
}
// ============================================================================
// declare the factory (needed for instantiation)
using GaudiExamples::TimingAlg;
DECLARE_COMPONENT( TimingAlg )
// ============================================================================
// The END
// ============================================================================
