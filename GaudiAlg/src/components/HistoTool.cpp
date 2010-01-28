// $Id: HistoTool.cpp,v 1.3 2006/01/10 19:53:58 hmd Exp $
// ============================================================================
// Include files 
// ============================================================================
#include "GaudiKernel/ToolFactory.h" 
// ============================================================================
// local 
// ============================================================================
#include "HistoTool.h"
// ============================================================================

/** @file HistoTool.cpp
 *
 *  Implementation file for class : HistoTool
 *  @date 2004-06-28 
 *  @author Vanya  BELYAEV Ivan.Belyaev@itep.ru
 */

// ============================================================================
// Declaration of the Tool Factory
// ============================================================================
DECLARE_TOOL_FACTORY(HistoTool)
// ============================================================================


// ============================================================================
// Standard constructor
// ============================================================================
HistoTool::HistoTool( const std::string& type,
                      const std::string& name,
                      const IInterface* parent )
  : GaudiHistoTool ( type, name , parent )
{
  declareInterface<IHistoTool>(this);  
}
// ============================================================================


// ============================================================================
// protected virtual destructor 
// ============================================================================
HistoTool::~HistoTool() {}
// ============================================================================
