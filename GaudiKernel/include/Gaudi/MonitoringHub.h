/*****************************************************************************\
* (c) Copyright 2020-2022 CERN for the benefit of the LHCb Collaboration      *
*                                                                             *
* This software is distributed under the terms of the GNU General Public      *
* Licence version 3 (GPL Version 3), copied verbatim in the file "COPYING".   *
*                                                                             *
* In applying this licence, CERN does not waive the privileges and immunities *
* granted to it by virtue of its status as an Intergovernmental Organization  *
* or submit itself to any jurisdiction.                                       *
\*****************************************************************************/
#pragma once

#include "GaudiKernel/detected.h"

#include <deque>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <typeindex>
#include <typeinfo>

namespace Gaudi::Monitoring {

  namespace details {

    template <typename T>
    using has_merge_and_reset_ = decltype( std::declval<T>().mergeAndReset( std::declval<T&&>() ) );
    template <typename T>
    inline constexpr bool has_merge_and_reset_v = Gaudi::cpp17::is_detected_v<has_merge_and_reset_, T>;
    template <typename T>
    using has_merge_from_json_ = decltype( std::declval<T>().mergeAndReset( nlohmann::json{} ) );
    template <typename T>
    inline constexpr bool has_merge_from_json_v = Gaudi::cpp17::is_detected_v<has_merge_from_json_, T>;
    template <typename T>
    using has_from_json_ = decltype( T::fromJSON( nlohmann::json{} ) );
    template <typename T>
    inline constexpr bool has_from_json_v = Gaudi::cpp17::is_detected_v<has_from_json_, T>;

    using MergeAndReset_t = void ( * )( void*, void* );

    template <typename T>
    MergeAndReset_t makeMergeAndResetFor() {
      if constexpr ( has_merge_and_reset_v<T> ) {
        return []( void* ptr, void* other ) {
          reinterpret_cast<T*>( ptr )->mergeAndReset( std::move( *reinterpret_cast<T*>( other ) ) );
        };
      } else {
        return []( void*, void* ) {};
      }
    }

    using MergeAndResetFromJSON_t = void ( * )( void*, const nlohmann::json& );

    template <typename T>
    MergeAndResetFromJSON_t makeMergeAndResetFromJSONFor() {
      if constexpr ( has_merge_from_json_v<T> ) {
        return []( void* ptr, const nlohmann::json& other ) { reinterpret_cast<T*>( ptr )->mergeAndReset( other ); };
      } else if constexpr ( has_merge_and_reset_v<T> && has_from_json_v<T> ) {
        return []( void* ptr, const nlohmann::json& other ) {
          reinterpret_cast<T*>( ptr )->mergeAndReset( T::fromJSON( other ) );
        };
      } else {
        return nullptr;
      }
    }

  } // namespace details

  struct ClashingEntityName : std::logic_error {
    using logic_error::logic_error;
  };

  /// default (empty) implementation of reset method for types stored into an entity
  template <typename T>
  void reset( T& ) {}

  /// default (empty) implementation of mergeAndReset method for types stored into an entity
  template <typename T>
  void mergeAndReset( T&, T& ) {}

  /// Central entity in a Gaudi application that manages monitoring objects (i.e. counters, histograms, etc.).
  ///
  /// The Gaudi::Monitoring::Hub delegates the actual reports to services implementing the Gaudi::Monitoring::Hub::Sink
  /// interface.
  struct Hub {
    /** Wrapper class for arbitrary monitoring objects.
     *
     * Mainly contains a pointer to the actual data with component, name and type metadata
     * Any object can be used as internal data and wrapped into an Entity as long as they can
     * be translated to json format. It is enough to make them aware to the nlohmann library
     * through a dedicated to_json method.
     *
     * another 2 free functions can be overloaded to adapt the behavior of an Entity for a
     * given type T :
     *   - void reset( T& t )
     *     called to reset the entity. The default provided implementation is empty
     *   - void mergeAndReset( T& ent, T&& other )
     *     called to merge other into entity and reset other. The default provided implementation is empty
     */
    class Entity {
    public:
      template <typename T>
      Entity( std::string component, std::string name, std::string type, T& ent )
          : component{ std::move( component ) }
          , name{ std::move( name ) }
          , type{ std::move( type ) }
          , m_ptr{ &ent }
          , m_typeIndex{ []( const void* ptr ) {
            return std::type_index( typeid( *reinterpret_cast<const T*>( ptr ) ) );
          } }
          , m_getJSON{ []( Entity const& e ) -> nlohmann::json { return *reinterpret_cast<const T*>( e.m_ptr ); } }
          , m_reset{ []( Entity& e ) { reset( *reinterpret_cast<T*>( e.m_ptr ) ); } }
          , m_mergeAndReset{ []( Entity& e, Entity& o ) {
            mergeAndReset( *reinterpret_cast<T*>( e.m_ptr ), *reinterpret_cast<T*>( o.m_ptr ) );
          } } {}
      /// name of the component owning the Entity
      std::string component;
      /// name of the entity
      std::string name;
      /// type of the entity, see comment above concerning its format and usage
      std::string type;
      /// function to get internal type
      std::type_index typeIndex() const { return ( *m_typeIndex )( m_ptr ); }
      /// conversion to json via nlohmann library
      friend void to_json( nlohmann::json& j, Gaudi::Monitoring::Hub::Entity const& e ) { j = ( *e.m_getJSON )( e ); }
      /// function resetting internal data
      friend void reset( Entity& e ) { ( *e.m_reset )( e ); }
      /**
       * function calling merge and reset on internal data with the internal data of another entity
       *
       * This function does not protect against usage with entities with different internal types
       * The user should ensure that entities are compatible before calling this function
       */
      friend void mergeAndReset( Entity& ent, Entity& other ) {
        if ( ent.typeIndex() != other.typeIndex() ) {
          throw std::runtime_error( std::string( "Entity: mergeAndReset called on different types: " ) +
                                    ent.typeIndex().name() + " and " + other.typeIndex().name() );
        }
        ( *ent.m_mergeAndReset )( ent, other );
      }
      /// operator== for comparison with raw pointer
      bool operator==( void* ent ) { return m_ptr == ent; }
      /// operator== for comparison with an entity
      bool operator==( Entity const& ent ) { return m_ptr == ent.m_ptr; }
      /// unique identifier, actually mapped to internal pointer
      void* id() const { return m_ptr; }

    private:
      /// pointer to the actual data inside this Entity
      void* m_ptr{ nullptr };
      // The next 4 members are needed for type erasure
      // indeed, their implementation is internal type dependant
      // (see Constructor above and the usage of T in the reinterpret_cast)
      /// function to get internal type.
      std::type_index ( *m_typeIndex )( const void* );
      /// function converting the internal data to json.
      nlohmann::json ( *m_getJSON )( Entity const& );
      /// function reseting internal data.
      void ( *m_reset )( Entity& );
      /// function calling merge and reset on internal data with the internal data of another entity
      void ( *m_mergeAndReset )( Entity&, Entity& );
    };

    /// Interface reporting services must implement.
    struct Sink {
      virtual void registerEntity( Entity ent )      = 0;
      virtual void removeEntity( Entity const& ent ) = 0;
      virtual ~Sink()                                = default;
    };

    Hub() { m_sinks.reserve( 5 ); }

    template <typename T>
    void registerEntity( std::string c, std::string n, std::string t, T& ent ) {
      registerEntity( { std::move( c ), std::move( n ), std::move( t ), ent } );
    }
    void registerEntity( Entity ent ) {
      std::for_each( begin( m_sinks ), end( m_sinks ), [ent]( auto sink ) { sink->registerEntity( ent ); } );
      m_entities.emplace( ent.id(), std::move( ent ) );
    }
    template <typename T>
    void removeEntity( T& ent ) {
      auto it = m_entities.find( &ent );
      if ( it != m_entities.end() ) {
        std::for_each( begin( m_sinks ), end( m_sinks ), [&it]( auto sink ) { sink->removeEntity( it->second ); } );
        m_entities.erase( it );
      }
    }

    void addSink( Sink* sink ) {
      std::for_each( begin( m_entities ), end( m_entities ),
                     [sink]( auto ent ) { sink->registerEntity( ent.second ); } );
      m_sinks.push_back( sink );
    }
    void removeSink( Sink* sink ) {
      auto it = std::find( begin( m_sinks ), end( m_sinks ), sink );
      if ( it != m_sinks.end() ) m_sinks.erase( it );
    }

  private:
    std::vector<Sink*>      m_sinks;
    std::map<void*, Entity> m_entities;
  };
} // namespace Gaudi::Monitoring
