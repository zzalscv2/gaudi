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

#include "Gaudi/MonitoringHub.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/Service.h"

#include <boost/algorithm/string.hpp>

#include <fmt/core.h>
#include <fmt/format.h>

#include <deque>
#include <map>
#include <string_view>

namespace {

  /**
   * map of formating strings for the different Entities printed here
   * note that the format of each entry has to be something like {0:name|fmt} where :
   *  - 0 is mandatory and tells that all entries will use the same unique input (the json object)
   *  - name is the name of the entry in the json dictionnary to be printed
   *  - | only separates the name and the format part
   *  - fmt is the standard formater for your type, e.g. 9.7f or so
   * Note that fmt does only support 'd', 'g' and 'p' as the letter for the format at this time
   * and that 'p' means 'percentage', that is it's the same as 'g' where the value displayed is
   * first multiplies by 100.
   */
  static const auto registry = std::map<std::string, std::string>{
      {"counter:", "{0:nEntries|10d}"},   // all unknown counters, and default
      {"histogram:", "{0:nEntries|10d}"}, // all histograms
      {"counter:AveragingCounter", "{0:nEntries|10d} |{0:sum|11.7g} |{0:mean|#11.5g}"},
      {"counter:SigmaCounter", "{0:nEntries|10d} |{0:sum|11.7g} |{0:mean|#11.5g} |{0:standard_deviation|#11.5g}"},
      {"counter:StatCounter", "{0:nEntries|10d} |{0:sum|11.7g} |{0:mean|#11.5g} |{0:standard_deviation|#11.5g} "
                              "|{0:min|#12.5g} |{0:max|#12.5g}"},
      {"counter:BinomialCounter",
       "{0:nEntries|10d} |{0:nTrueEntries|11d} |({0:efficiency|#9.7p} +- {0:efficiencyErr|-#8.7p})%"},
  };
} // namespace

/**
 * fmt formatter function for json class
 * able to handle 2 types of formats :
 *   {} : in this case the type entry of json is used to deduce
 *        what to print, looking into the registry
 *   {:name|fmt} : in this case, the entry 'name' of the json
 *        will be printed in given format. See comment on registry
 *        for more details
 */
template <>
class fmt::formatter<nlohmann::json> {
public:
  template <typename ParseContext>
  constexpr auto parse( ParseContext& ctx ) {
    auto fmt_begin = ctx.begin();
    auto fmt_end   = std::find( fmt_begin, ctx.end(), '}' );
    if ( fmt_begin == fmt_end ) {
      // we are dealing with {}, only make sure currentFormat is empty
      currentFormat = "";
    } else {
      // non empty format, let's split name from format
      auto fmt_colon = std::find( fmt_begin, fmt_end, '|' );
      currentName    = std::string( fmt_begin, fmt_colon - fmt_begin );
      currentFormat  = std::string( fmt_colon + 1, fmt_end - fmt_colon - 1 );
    }
    return fmt_end;
  }
  template <typename FormatContext>
  auto format( const nlohmann::json& j, FormatContext& ctx ) {
    if ( currentFormat.size() == 0 ) {
      // dealing with {} format, let's find entry for our type in registry
      const auto type = j.at( "type" ).get<std::string>();
      // first looking for the entry, then for its category, finally taking "counter:" as default
      auto entry = registry.find( type );
      if ( entry == registry.end() ) {
        auto c = type.find( ":" );
        if ( c != std::string::npos ) entry = registry.find( type.substr( 0, c + 1 ) );
        if ( entry == registry.end() ) entry = registry.find( "counter:" );
        assert( entry != registry.end() );
      }
      // print the json string according to format found
      // This actually will call this formatter again a number of times
      return fmt::format_to( ctx.out(), entry->second, j );
    } else {
      // dealing with a {:name|fmt} format
      auto actualFormat = fmt::format( "{{:{}", currentFormat ) + "}";
      switch ( currentFormat.back() ) {
      case 'd':
        return fmt::format_to( ctx.out(), actualFormat, j.at( currentName ).template get<unsigned int>() );
      case 'g':
        return fmt::format_to( ctx.out(), actualFormat, j.at( currentName ).template get<double>() );
      case 'p':
        actualFormat[actualFormat.size() - 2] = 'g';
        return fmt::format_to( ctx.out(), actualFormat, j.at( currentName ).template get<double>() * 100 );
      default:
        return fmt::format_to( ctx.out(), "Unknown counter format : {}", currentFormat );
      }
    }
  }

private:
  std::string currentFormat;
  std::string currentName;
};

namespace {

  template <typename stream>
  stream printCounter( stream& log, const std::string& id, const nlohmann::json& j ) {
    const auto type = j.at( "type" ).get<std::string>();
    // binomial counters are slightly different ('*' character)
    const char* formatString = ( type == "counter:BinomialCounter" ) ? " |*{:48}|{} |" : " | {:48}|{} |";
    // for backward compatibility, we need to deal with statentity in a special way
    // this block should be dropped when StatEntities are gone
    if ( type == "statentity" ) {
      using boost::algorithm::icontains;
      bool isBinomial = icontains( id, "eff" ) || icontains( id, "acc" ) || icontains( id, "filt" ) ||
                        icontains( id, "fltr" ) || icontains( id, "pass" );
      auto nj    = j;
      nj["type"] = isBinomial ? "counter:BinomialCounter" : "counter:StatCounter";
      return log << fmt::format( isBinomial ? " |*{:48}|{} |" : " | {:48}|{} |", id, nj );
    }
    return log << fmt::format( formatString, id, j );
  }

} // namespace

namespace Gaudi::Monitoring {

  class MessageSvcSink : public Service, public Hub::Sink {

  public:
    using Service::Service;

    /// initialization, registers to Monitoring::Hub
    StatusCode initialize() override {
      return Service::initialize().andThen( [&] {
        // declare ourself as a monitoding sink
        serviceLocator()->monitoringHub().addSink( this );
      } );
    }

    /// stop method, handles the printing
    StatusCode stop() override;

    // Gaudi::Monitoring::Hub::Sink implementation
    void registerEntity( Gaudi::Monitoring::Hub::Entity ent ) override {
      if ( std::string_view( ent.type ).substr( 0, 8 ) == "counter:" || ent.type == "statentity" ||
           ent.type == "histogram" ) {
        m_monitoringEntities.emplace_back( std::move( ent ) );
      }
    }

  private:
    std::deque<Hub::Entity> m_monitoringEntities;
  };

  DECLARE_COMPONENT( MessageSvcSink )
} // namespace Gaudi::Monitoring

StatusCode Gaudi::Monitoring::MessageSvcSink::stop() {
  // We will try to mimic the old monitoring of counters, so we need to split
  // them per Algo. The algo name can be extracted form the id of the entity
  // as its format is "algoName/counterName"
  // This map groups entities per algoName. For each name, the submap gives
  // the counter name of each subentity and the associated json
  std::map<std::string, std::map<std::string, nlohmann::json>> sortedEntities;
  // fill the sorted map
  for ( auto& entity : m_monitoringEntities ) { sortedEntities[entity.component][entity.name] = entity.toJSON(); }
  // dump all counters
  for ( auto& [algoName, entityMap] : sortedEntities ) {
    // check first whether there is any counter to log
    unsigned int nbCounters =
        std::accumulate( begin( entityMap ), end( entityMap ), 0, []( const unsigned int& a, const auto& j ) {
          return a + ( j.second.at( "empty" ).template get<bool>() ? 0 : 1 );
        } );
    if ( 0 == nbCounters ) continue;
    MsgStream log{msgSvc(), algoName};
    log << MSG::INFO << "Number of counters : " << nbCounters << "\n"
        << " |    Counter                                      |     #     |   "
        << " sum     | mean/eff^* | rms/err^*  |     min     |     max     |";
    std::for_each( begin( entityMap ), end( entityMap ), [&log]( auto& p ) {
      // Do not print empty counters
      if ( !p.second.at( "empty" ).template get<bool>() ) {
        log << "\n";
        printCounter( log, p.first, p.second );
      }
    } );
    log << endmsg;
  }
  return Service::stop();
}
