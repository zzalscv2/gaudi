#include "FloatTool.h"
#include <GaudiAlg/Consumer.h>
#include <GaudiAlg/Producer.h>
#include <GaudiKernel/AnyDataHandle.h>
#include <GaudiKernel/MsgStream.h>

namespace Gaudi
{
  namespace Examples
  {
    using BaseClass_t = Gaudi::Functional::Traits::BaseClass_t<Algorithm>;

    struct THDataProducer : Gaudi::Functional::Producer<int(), BaseClass_t> {

      THDataProducer( const std::string& name, ISvcLocator* svcLoc )
          : Producer( name, svcLoc, KeyValue( "OutputLocation", "/Event/MyInt" ) )
      {
      }

      int operator()() const override
      {
        info() << "executing IntDataProducer, storing 7 into " << outputLocation() << endmsg;
        return 7;
      }
    };

    DECLARE_COMPONENT( THDataProducer )

    struct THDataProducer2 : Gaudi::Functional::Producer<float(), BaseClass_t> {

      THDataProducer2( const std::string& name, ISvcLocator* svcLoc )
          : Producer( name, svcLoc, KeyValue( "OutputLocation", "/Event/MyFloat" ) )
      {
      }

      float operator()() const override
      {
        info() << "executing IntDataProducer, storing 7.0 into " << outputLocation() << endmsg;
        return 7.0;
      }
    };

    DECLARE_COMPONENT( THDataProducer2 )

    struct THDataConsumer : Gaudi::Functional::Consumer<void( const int& ), BaseClass_t> {

      THDataConsumer( const std::string& name, ISvcLocator* svcLoc )
          : Consumer( name, svcLoc, KeyValue( "InputLocation", "/Event/MyInt" ) )
      {
      }

      void operator()( const int& input ) const override
      {
        info() << "executing IntDataConsumer, checking " << input << " from " << inputLocation() << " and "
               << m_floatTool->getFloat() << " from FloatTool are matching" << endmsg;
      }

      ToolHandle<FloatTool> m_floatTool{this, "FloatTool", "Gaudi::Examples::FloatTool"};
    };

    DECLARE_COMPONENT( THDataConsumer )
  }
}
