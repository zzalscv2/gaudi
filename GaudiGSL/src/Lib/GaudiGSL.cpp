// ============================================================================
// local
#include "GaudiGSL/GaudiGSL.h"

// ============================================================================
/** @file 
 * 
 *  Implementation file for class : GaudiGSL
 * 
 *  @date 29/04/2002 
 *  @author Vanya Belyaev Ivan.Belyaev@itep.ru
 */
// ============================================================================

// ============================================================================
/** define the initial value for static variable 
 */
// ============================================================================
const IGslSvc* GaudiGSL::s_gslSvc = nullptr;
// ============================================================================

// ============================================================================
/** static accessor to Gaudi GSL Service 
 *  @return (const) pointer to Gaudi GSL Service 
 */
// ============================================================================
const IGslSvc* GaudiGSL::gslSvc () { return s_gslSvc ; }
// ============================================================================

// ============================================================================
/**set new value for static Gaudi GSL Service  
 *  @return (const) pointer to Gaudi GSL Service 
 */
// ============================================================================
const IGslSvc* GaudiGSL::setGslSvc ( const IGslSvc* value ) 
{ s_gslSvc = value ; return gslSvc() ; }
// ============================================================================

// ============================================================================
// The END 
// ============================================================================
