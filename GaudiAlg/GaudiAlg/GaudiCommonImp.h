#ifndef GAUDIALG_GAUDICOMMONIMP_H
#define GAUDIALG_GAUDICOMMONIMP_H 1
// ============================================================================
// Include files
// ============================================================================
#include <algorithm>
// ============================================================================
// GaudiAlg
// ============================================================================
#include "GaudiAlg/GetData.h"
#include "GaudiAlg/GaudiCommon.h"
// ============================================================================
/** @file
 *  The implementation of inline/templated methods for class GaudiCommon
 *  @see    GaudiCommon
 *  @author Vanya BELYAEV Ivan.Belyaev@itep.ru
 *  @author Chris Jones   Christopher.Rob.Jones@cern.ch
 *  @date   2004-01-19
 */
// ============================================================================
// Returns the full correct event location given the rootInTes settings
// ============================================================================
template < class PBASE >
inline const std::string
GaudiCommon<PBASE>::fullTESLocation( const std::string & location,
                                     const bool useRootInTES ) const
{
  // The logic is:
  // if no R.I.T., give back location
  // if R.I.T., this is the mapping:
  // (note that R.I.T. contains a trailing '/')
  //  location       -> result
  //  -------------------------------------------------
  //  ""             -> R.I.T.[:-1]      ("rit")
  //  "/Event"       -> R.I.T.[:-1]      ("rit")
  //  "/Event/MyObj" -> R.I.T. + "MyObj" ("rit/MyObj")
  //  "MyObj"        -> R.I.T. + "MyObj" ("rit/MyObj")
  return ( !useRootInTES || rootInTES().empty() ?
           location
         :
           location.empty() || ( location == "/Event" ) ?
             rootInTES().substr(0,rootInTES().size()-1)
           :
             0 == location.find("/Event/") ?
               rootInTES() + location.substr(7)
             :
               rootInTES() + location );
}
// ============================================================================
// Templated access to the data in Gaudi Transient Store
// ============================================================================
template < class PBASE >
template < class TYPE  >
inline typename Gaudi::Utils::GetData<TYPE>::return_type
GaudiCommon<PBASE>::get( IDataProviderSvc*  service ,
                         const std::string& location,
                         const bool useRootInTES ) const
{
  // check the environment
  Assert( service ,    "get():: IDataProvider* points to NULL!"      ) ;
  // get the helper object:
  Gaudi::Utils::GetData<TYPE> getter ;
  return getter ( *this    ,
                  service  ,
                  fullTESLocation ( location , useRootInTES ) ) ;
}
// ============================================================================
// Templated access to the data in Gaudi Transient Store, no check on the content
// ============================================================================
template < class PBASE >
template < class TYPE  >
inline typename Gaudi::Utils::GetData<TYPE>::return_type
GaudiCommon<PBASE>::getIfExists( IDataProviderSvc*  service ,
                                 const std::string& location,
                                 const bool useRootInTES ) const
{
  // check the environment
  Assert( service ,    "get():: IDataProvider* points to NULL!"      ) ;
  // get the helper object:
  Gaudi::Utils::GetData<TYPE> getter ;
  return getter ( *this    ,
                  service  ,
                  fullTESLocation ( location , useRootInTES ),
                  false) ;
}
// ============================================================================
// check the existence of objects in Gaudi Transient Store
// ============================================================================
template < class PBASE >
template < class TYPE  >
inline bool GaudiCommon<PBASE>::exist( IDataProviderSvc*  service  ,
                                       const std::string& location ,
                                       const bool useRootInTES ) const
{
  // check the environment
  Assert( service , "exist():: IDataProvider* points to NULL!"      ) ;
  // check the data object
  Gaudi::Utils::CheckData<TYPE> checker ;
  return checker ( service,
                   fullTESLocation ( location , useRootInTES ) ) ;
}
// ============================================================================
// get the existing object from Gaudi Event Transient store
// or create new object register in in TES and return if object
// does not exist
// ============================================================================
template <class PBASE>
template <class TYPE,class TYPE2>
inline typename Gaudi::Utils::GetData<TYPE>::return_type
GaudiCommon<PBASE>::getOrCreate( IDataProviderSvc*  service  ,
                                 const std::string& location ,
                                 const bool useRootInTES  ) const
{
  // check the environment
  Assert ( service , "getOrCreate():: svc points to NULL!" ) ;
  // get the helper object
  Gaudi::Utils::GetOrCreateData<TYPE,TYPE2> getter ;
  return getter ( *this                                     ,
                  service                                   ,
                  fullTESLocation( location, useRootInTES ) ,
                  location                                  ) ;
}
// ============================================================================
// the useful method for location of tools.
// ============================================================================
template < class PBASE >
template < class TOOL  >
inline TOOL* GaudiCommon<PBASE>::tool( const std::string& type           ,
                                       const std::string& name           ,
                                       const IInterface*  parent         ,
                                       bool               create         ) const
{
  // for empty names delegate to another method
  if ( name.empty() ) return tool<TOOL>( type , parent , create ) ;
  Assert( this->toolSvc(), "tool():: IToolSvc* points to NULL!" ) ;
  // get the tool from Tool Service
  TOOL* Tool = nullptr ;
  const StatusCode sc =
    this->toolSvc()->retrieveTool ( type , name , Tool , parent , create ) ;
  if ( sc.isFailure() )
  { Exception("tool():: Could not retrieve Tool '" + type + "'/'" + name + "'", sc ) ; }
  if ( !Tool )
  { Exception("tool():: Could not retrieve Tool '" + type + "'/'" + name + "'"     ) ; }
  // add the tool into list of known tools to be properly released
  addToToolList( Tool );
  // return *VALID* located tool
  return Tool ;
}
// ============================================================================
// the useful method for location of tools.
// ============================================================================
template < class PBASE >
template < class TOOL  >
inline TOOL* GaudiCommon<PBASE>::tool( const std::string& type   ,
                                       const IInterface*  parent ,
                                       bool               create ) const
{
  // check the environment
  Assert ( PBASE::toolSvc(), "IToolSvc* points to NULL!" );
  // retrieve the tool from Tool Service
  TOOL* Tool = nullptr ;
  const StatusCode sc =
    this->toolSvc() -> retrieveTool ( type, Tool, parent , create   );
  if ( sc.isFailure() )
  { Exception("tool():: Could not retrieve Tool '" + type + "'", sc ) ; }
  if ( !Tool )
  { Exception("tool():: Could not retrieve Tool '" + type + "'"     ) ; }
  // add the tool into the list of known tools to be properly released
  addToToolList( Tool );
  // return *VALID* located tool
  return Tool ;
}
// ============================================================================
// the useful method for location of services
// ============================================================================
template < class PBASE   >
template < class SERVICE >
inline SmartIF<SERVICE> GaudiCommon<PBASE>::svc( const std::string& name   ,
                                                 const bool         create ) const
{
  Assert ( this->svcLoc(), "ISvcLocator* points to NULL!" );
  SmartIF<SERVICE> s;
  // check if we already have this service
  auto it = std::lower_bound( std::begin(m_services), std::end(m_services), name, svc_lt );
  if ( it != std::end(m_services) && svc_eq(*it,name) ) {
    // Try to get the requested interface
    s = *it;
    // check the results
    if ( !s.isValid() ) {
      Exception ("svc():: Could not retrieve Svc '" + name + "'", StatusCode::FAILURE);
    }
  } else {
    SmartIF<IService>& baseSvc = this->svcLoc()->service(name, create);
    // Try to get the requested interface
    s = baseSvc;
    // check the results
    if ( !baseSvc.isValid() || !s.isValid() ) {
      Exception ("svc():: Could not retrieve Svc '" + name + "'", StatusCode::FAILURE);
    }
    // add the tool into list of known tools, to be properly released
    addToServiceList(baseSvc);
  }
  // return *VALID* located service
  return s;
}
// ============================================================================
// Short-cut  to get a pointer to the UpdateManagerSvc
// ============================================================================
template <class PBASE>
inline IUpdateManagerSvc *
GaudiCommon<PBASE>::updMgrSvc() const
{
  if ( !m_updMgrSvc )
  { m_updMgrSvc = svc<IUpdateManagerSvc>("UpdateManagerSvc",true); }
  return m_updMgrSvc ;
}
// ============================================================================
// Assertion - throw exception, if condition is not fulfilled
// ============================================================================
template <class PBASE>
inline void GaudiCommon<PBASE>::Assert( const bool         ok  ,
                                        const std::string& msg ,
                                        const StatusCode   sc  ) const
{
  if (!ok) Exception( msg , sc );
}
// ============================================================================
// Delete the current message stream object
// ============================================================================
template <class PBASE>
inline void GaudiCommon<PBASE>::resetMsgStream() const
{
  m_msgStream.reset();
}
// ============================================================================
// Assertion - throw exception, if condition is not fulfilled
// ============================================================================
template <class PBASE>
inline void GaudiCommon<PBASE>::Assert( const bool        ok  ,
                                        const char*       msg ,
                                        const StatusCode  sc  ) const
{
  if (!ok) Exception( msg , sc );
}
// ============================================================================
/** @def ALG_ERROR
 *  Small and simple macro to add into error message the file name
 *  and the line number for easy location of the problematic lines.
 *
 *  @code
 *
 *  if ( a < 0 ) { return ALG_ERROR( "'a' is negative" , 301 ) ; }
 *
 *  @endcode
 *
 *  @author Vanya BELYAEV Ivan.Belyaev@itep.ru
 *  @date 2002-10-29
 */
// ============================================================================
#define ALG_ERROR( message , code )                                     \
  ( Error( message                                   +                  \
           std::string             ( " [ at line " ) +                  \
           std::to_string          (   __LINE__    ) +                  \
           std::string             ( " in file '"  ) +                  \
           std::string             (   __FILE__    ) + "']" , code ) )


// definition of helper class instances (see GAUDI-1081)
template <class PBASE>
constexpr const typename GaudiCommon<PBASE>::svc_eq_t GaudiCommon<PBASE>::svc_eq;
template <class PBASE>
constexpr const typename GaudiCommon<PBASE>::svc_lt_t GaudiCommon<PBASE>::svc_lt;
// ============================================================================
// The END
// ============================================================================
#endif // GAUDIALG_GAUDICOMMONIMP_H
