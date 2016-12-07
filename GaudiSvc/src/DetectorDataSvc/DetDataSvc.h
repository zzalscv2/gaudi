#ifndef DETECTORDATASVC_DETDATASVC_H
#define DETECTORDATASVC_DETDATASVC_H

// Base classes
//#include "GaudiKernel/DataSvc.h"
// D. Piparo: Change to the new thread safe version
#include "GaudiKernel/IDetDataSvc.h"
#include "GaudiKernel/IIncidentListener.h"
#include "GaudiKernel/IRegistry.h"
#include "GaudiKernel/Time.h"
#include "GaudiKernel/TsDataSvc.h"


// Forward declarations
class StatusCode;
class IAddressCreator;

///---------------------------------------------------------------------------
/** @class DetDataSvc DetDataSvc.h DetectorDataSvc/DetDataSvc.h

    A DataSvc specialized in detector data.
    Now inherits from TsDataSvc, a thread safe version of the DataSvc, which
    allows concurrent retrieval of DataObjects.

    @author Marco Clemencic (previous author unknown)
    @author Danilo Piparo

*/ //--------------------------------------------------------------------------

class DetDataSvc : public extends<TsDataSvc, IDetDataSvc, IIncidentListener>
{

  // unhides DataSvc updateObject methods
  using TsDataSvc::updateObject;

public:
  // Overloaded DataSvc methods

  /// Initialize the service
  StatusCode initialize() override;

  /// Initialize the service
  StatusCode reinitialize() override;

  /// Finalize the service
  StatusCode finalize() override;

  /// Remove all data objects in the data store.
  StatusCode clearStore() override;

  /// Update object
  StatusCode updateObject( DataObject* toUpdate ) override;

  /// Standard Constructor
  DetDataSvc( const std::string& name, ISvcLocator* svc );

  /// Standard Destructor
  ~DetDataSvc() override = default;

private:
  /// Deal with Detector Description initialization
  StatusCode setupDetectorDescription();

  using TsDataSvc::loadObject;
  StatusCode loadObject( IConversionSvc* pLoader, IRegistry* pNode ) override final {
    if ( LIKELY( m_allowLoadInRunning || serviceLocator().as<IStateful>()->FSMState() != Gaudi::StateMachine::RUNNING ) ) {
      return TsDataSvc::loadObject(pLoader, pNode);
    }
    error() << "Trying to load " << pNode->identifier() << " while RUNNING" << endmsg;
    return StatusCode::FAILURE;
  }

public:
  // Implementation of the IDetDataSvc interface

  /// Check if the event time has been set.
  /// Kept for backward compatibility, returns always true.
  bool validEventTime() const override;

  /// Get the event time
  const Gaudi::Time& eventTime() const override;

  /// Set the new event time
  void setEventTime( const Gaudi::Time& time ) override;

public:
  // Implementation of the IIncidentListener interface

  /// Inform that a new incident has occured
  void handle( const Incident& ) override;

private:
  Gaudi::Property<int> m_detStorageType{this, "DetStorageType", XML_StorageType,
                                        "Detector Data Persistency Storage type"};
  Gaudi::Property<std::string> m_detDbLocation{this, "DetDbLocation", "empty",
                                               "location of detector Db (filename,URL)"};
  Gaudi::Property<std::string> m_detDbRootName{this, "DetDbRootName", "dd", "name of the root node of the detector"};
  Gaudi::Property<bool> m_usePersistency{this, "UsePersistency", false, "control if the persistency is required"};
  Gaudi::Property<std::string> m_persistencySvcName{this, "PersistencySvc", "DetectorPersistencySvc",
                                                    "name of the persistency service"};

  Gaudi::Property<bool> m_allowLoadInRunning{this, "AllowLoadInRunning", true,
                                             "if set to false, no new object can be loaded while in running state "
                                             "(updates are still allowed), this forces preloading of the geometry"};

  /// Current event time
  Gaudi::Time m_eventTime = 0;

  /// Address Creator to be used
  SmartIF<IAddressCreator> m_addrCreator = nullptr;
};

#endif // DETECTORDATASVC_DETDATASVC_H
