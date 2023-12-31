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
#include <Gaudi/Functional/MergingTransformer.h>
#include <string>

using ints                     = Gaudi::Functional::vector_of_const_<int>;
using out_t                    = std::tuple<float, float>;
using is2ff_merger_base        = Gaudi::Functional::MergingMultiTransformer<out_t( ints const& )>;
using is2ff_merger_filter_base = Gaudi::Functional::MergingMultiTransformerFilter<out_t( ints const& )>;
using filter_out_t             = std::tuple<bool, float, float>;

struct is2ff_merger : public is2ff_merger_base {
  is2ff_merger( std::string const& name, ISvcLocator* pSvcLocator )
      : is2ff_merger_base( name, pSvcLocator, { "InputInts", { "firstInt", "secondInt" } },
                           { KeyValue{ "O1", "firstFloat" }, KeyValue{ "O2", "secondFloat" } } ) {}

  out_t operator()( ints const& is ) const override {
    float f1 = 1, f2 = 1;

    for ( auto i : is ) {
      info() << "i: " << i << " ";
      f1 *= i;
      f2 *= 1.f / i;
    }
    info() << endmsg;
    return { f1, f2 };
  }
};

DECLARE_COMPONENT( is2ff_merger )

struct is2ff_merger_filter : public is2ff_merger_filter_base {
  is2ff_merger_filter( std::string const& name, ISvcLocator* pSvcLocator )
      : is2ff_merger_filter_base( name, pSvcLocator, { "InputInts", { "firstInt", "secondInt" } },
                                  { KeyValue{ "O1", "firstFloat" }, KeyValue{ "O2", "secondFloat" } } ) {}

  filter_out_t operator()( ints const& is ) const override {
    float f1 = 1, f2 = 1;

    for ( auto i : is ) {
      info() << "i: " << i << " ";
      f1 *= i;
      f2 *= 1.f / i;
    }
    info() << endmsg;
    auto filter_passed = f1 > 10;
    info() << "Filter " << ( filter_passed ? "passed" : "failed" ) << endmsg;
    return { filter_passed, f1, f2 };
  }
};

DECLARE_COMPONENT( is2ff_merger_filter )
