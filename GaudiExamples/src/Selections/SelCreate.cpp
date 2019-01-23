// ============================================================================
// Include files
// ============================================================================
// GaudiKernel
// ============================================================================
#include "GaudiKernel/IRndmGenSvc.h"
#include "GaudiKernel/RndmGenerators.h"
// ============================================================================
// GaudiAlg
// ============================================================================
#include "GaudiAlg/GaudiAlgorithm.h"
// ============================================================================
// Local
// ============================================================================
#include "GaudiExamples/MyTrack.h"
// ============================================================================
#ifdef __ICC
// disable icc remark #2259: non-pointer conversion from "double" to "float" may lose significant bits
#pragma warning( disable : 2259 )
#elif defined( WIN32 )
// disable warning
//   C4244: 'argument' : conversion from 'double' to 'float', possible loss of data
#pragma warning( disable : 4244 )
#endif

namespace Gaudi
{
  namespace Examples
  {
    /** @class SelCreate
     *  Simple class to create few "containers" in TES
     *  @author Vanya BELYAEV Ivan.Belyaev@nikhef.nl
     *  @date 2008-07-23
     */
    class SelCreate : public GaudiAlgorithm
    {
      using Container = Gaudi::Examples::MyTrack::Container;
      DataObjectWriteHandle<Container> m_output{this, "Output", this->name(), "TES location of output container"};

    public:
      // ======================================================================
      /** standard constructor
       */
      SelCreate( const std::string& name, ISvcLocator* pSvcLocator ) : GaudiAlgorithm( name, pSvcLocator ) {}
      // using GaudiAlgorithm::GaudiAlgorithm;
      // ======================================================================
      /// the only one essential method
      StatusCode execute() override
      {
        // some random number generators, just to provide the numbers
        static Rndm::Numbers gauss( randSvc(), Rndm::Gauss( 0.0, 1.0 ) );
        static Rndm::Numbers flat( randSvc(), Rndm::Flat( 20.0, 100.0 ) );

        // create the data
        auto tracks = std::make_unique<Gaudi::Examples::MyTrack::Container>();

        for ( int i = 0; i < 100; ++i ) {
          // create new track
          auto track = std::make_unique<Gaudi::Examples::MyTrack>();

          // fill it with some "data"
          track->setPx( gauss() );
          track->setPy( gauss() );
          track->setPz( gauss() + flat() );

          // insert it into the container
          tracks->insert( track.release() );
        }

        // register the container in TES
        m_output.put( std::move( tracks ) );

        typedef Gaudi::NamedRange_<Gaudi::Examples::MyTrack::ConstVector> Range;
        if ( !exist<Range>( name() ) ) {
          err() << "No Range is available at location " << name() << endmsg;
        }

        // test "get-or-create":

        Range r1 = getOrCreate<Range, Gaudi::Examples::MyTrack::Container>( name() + "_1" );
        r1.empty(); // avoid icc remark #177: variable "X" was declared but never referenced
        Range r2 = getOrCreate<Range, Gaudi::Examples::MyTrack::Selection>( name() + "_2" );
        r2.empty(); // avoid icc remark #177: variable "X" was declared but never referenced

        getOrCreate<Gaudi::Examples::MyTrack::Container, Gaudi::Examples::MyTrack::Container>( name() + "_3" );
        getOrCreate<Gaudi::Examples::MyTrack::Selection, Gaudi::Examples::MyTrack::Selection>( name() + "_4" );

        return StatusCode::SUCCESS;
      }
    };
    // ========================================================================
  } // end of namespace Gaudi::Examples
} // end of namespace Gaudi
// ============================================================================
/// The factory (needed for instantiation)
using Gaudi::Examples::SelCreate;
DECLARE_COMPONENT( SelCreate )
// ============================================================================
// The END
// ============================================================================
