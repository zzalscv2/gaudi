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
#ifndef GAUDIEXAMPLE_MYALGORITHM_H
#define GAUDIEXAMPLE_MYALGORITHM_H 1

// Include files
#include "GaudiAlg/GaudiAlgorithm.h"
#include "GaudiKernel/DataObject.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/ToolHandle.h"

// Forward references
class IMyTool;

/** Trivial Algorithm for tutorial purposes

    @author nobody
*/
class MyGaudiAlgorithm : public GaudiAlgorithm {
public:
  /// Constructor of this form must be provided
  MyGaudiAlgorithm( const std::string& name, ISvcLocator* pSvcLocator );

  /// Three mandatory member functions of any algorithm
  StatusCode initialize() override;
  StatusCode execute() override;
  StatusCode finalize() override;

  bool isClonable() const override { return true; }

private:
  Gaudi::Property<std::string> m_privateToolType{ this, "ToolWithName", "MyTool",
                                                  "Type of the tool to use (internal name is ToolWithName)" };

  IMyTool* m_privateTool  = nullptr;
  IMyTool* m_publicTool   = nullptr;
  IMyTool* m_privateGTool = nullptr;
  IMyTool* m_publicGTool  = nullptr;

  IMyTool* m_privateToolWithName = nullptr;

  IMyOtherTool* m_privateOtherInterface = nullptr;

  ToolHandle<IMyTool> m_legacyToolHandle{ "MyTool/LegacyToolHandle", this };

  ToolHandle<IMyTool>       m_myPrivToolHandle{ this, "PrivToolHandle", "MyTool/PrivToolHandle" };
  PublicToolHandle<IMyTool> m_myPubToolHandle{ this, "PubToolHandle", "MyTool/PubToolHandle" };

  PublicToolHandle<IAlgTool> m_myGenericToolHandle{ this, "GenericToolHandle", "MyTool/GenericToolHandle" };

  ToolHandle<IAlgTool> m_myUnusedToolHandle{ this, "UnusedToolHandle", "TestToolFailing/UnusedToolHandle" };

  ToolHandle<IMyTool> m_undefinedToolHandle{ this };
  ToolHandle<IMyTool> m_invalidToolHandle{ this, "InvalidToolHandle", "TestToolFailing" };

  ToolHandle<IWrongTool> m_wrongIfaceTool{ this, "WrongIfaceTool", "MyTool/WrongIfaceTool" };

  PublicToolHandle<const IMyTool> m_myConstToolHandle{ "MyTool/ConstGenericToolHandle" };

  PublicToolHandle<const IMyTool> m_myCopiedConstToolHandle;
  PublicToolHandle<const IMyTool> m_myCopiedConstToolHandle2;
  PublicToolHandle<IMyTool>       m_myCopiedToolHandle;

  PublicToolHandleArray<IMyTool> m_tha{ this,
                                        "MyPublicToolHandleArrayProperty",
                                        { "MyTool/AnotherConstGenericToolHandle", "MyTool/AnotherInstanceOfMyTool" } };

  DataObjectReadHandle<DataObject> m_tracks{ this, "tracks", "/Event/Rec/Tracks", "the tracks" };
  DataObjectReadHandle<DataObject> m_hits{ this, "hits", "/Event/Rec/Hits", "the hits" };
  DataObjectReadHandle<DataObject> m_raw{ this, "raw", "/Rec/RAW", "the raw stuff" };

  DataObjectWriteHandle<DataObject> m_selectedTracks{ this, "trackSelection", "/Event/MyAnalysis/Tracks",
                                                      "the selected tracks" };
};

#endif // GAUDIEXAMPLE_MYALGORITHM_H
