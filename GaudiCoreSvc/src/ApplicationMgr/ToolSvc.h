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
#ifndef GAUDISVC_TOOLSVC_H
#define GAUDISVC_TOOLSVC_H

// Include Files
#include "GaudiKernel/IHistorySvc.h"
#include "GaudiKernel/IToolSvc.h"
#include "GaudiKernel/Service.h"
#include <mutex>
#include <vector>

/** @class ToolSvc ToolSvc.h
 *  This service manages tools.
 *  Tools can be common, in which case a single instance
 *  can be shared by different algorithms, or private
 *  in which case it is necessary to specify the parent
 *  when requesting it.
 *  The parent of a tool can be an algortihm or a Service
 *  The environment of a tool is set by using that of the
 *  parent. A common tool is considered to belong to the
 *  ToolSvc itself.
 *
 *  @author G. Corti, P. Mato
 */
class ToolSvc : public extends<Service, IToolSvc> {

public:
  /// Standard Constructor.
  using extends::extends;

  /// Destructor.
  ~ToolSvc() override;

  /// Finalize the service.
  StatusCode finalize() override;

  // Start transition for tools
  StatusCode start() override;

  // Stop transition for tools
  StatusCode stop() override;

  /// Retrieve tool, create it by default as common tool if it does not already exist
  StatusCode retrieve( std::string_view type, const InterfaceID& iid, IAlgTool*& tool, const IInterface* parent,
                       bool createIf ) override;

  /// Retrieve tool, create it by default as common tool if it does not already exist
  StatusCode retrieve( std::string_view tooltype, std::string_view toolname, const InterfaceID& iid, IAlgTool*& tool,
                       const IInterface* parent, bool createIf ) override;

  /// Get names of all tool instances of a given type
  std::vector<std::string> getInstances( std::string_view toolType ) override;

  /// Get names of all tool instances
  std::vector<std::string> getInstances() const override;

  /// Get pointers to all tool instances
  std::vector<IAlgTool*> getTools() const override;

  /// Release tool
  StatusCode releaseTool( IAlgTool* tool ) override;

  /// Create Tool standard way with automatically assigned name
  StatusCode create( const std::string& type, const IInterface* parent, IAlgTool*& tool );

  /// Create Tool standard way with specified name
  StatusCode create( const std::string& type, const std::string& name, const IInterface* parent, IAlgTool*& tool );

  /// Check if the tool instance exists
  bool existsTool( std::string_view toolname ) const;

  /// Get Tool full name by combining nameByUser and "parent" part
  std::string nameTool( std::string_view nameByUser, const IInterface* parent );

  void registerObserver( IToolSvc::Observer* obs ) override;

private: // methods
  // helper functions
  //
  /** The total number of refCounts on all tools in the instancesTools list */
  unsigned long totalToolRefCount() const;
  /** The minimum number of refCounts of all tools */
  unsigned long minimumToolRefCount() const;

  /// Finalize the given tool, with exception handling
  StatusCode finalizeTool( IAlgTool* itool ) const;

private: // data
  Gaudi::Property<bool> m_checkNamedToolsConfigured{
      this, "CheckedNamedToolsConfigured", false,
      "Check that tools which do not have the default name have some explicit configuration." };

  Gaudi::Property<bool> m_showToolDataDeps{ this, "ShowDataDeps", false, "show the data dependencies of AlgTools" };

  /// Common Tools
  std::vector<IAlgTool*> m_instancesTools; // List of all instances of tools

  /// Pointer to HistorySvc
  IHistorySvc* m_pHistorySvc = nullptr;

  std::vector<IToolSvc::Observer*> m_observers;

  typedef std::recursive_mutex CallMutex;
  mutable CallMutex            m_mut;
};

#endif
