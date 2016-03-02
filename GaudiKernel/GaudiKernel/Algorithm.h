#ifndef GAUDIKERNEL_ALGORITHM_H
#define GAUDIKERNEL_ALGORITHM_H
// ============================================================================
// Include files
// ============================================================================
#include "GaudiKernel/IProperty.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/IService.h"
#include "GaudiKernel/PropertyMgr.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/IMessageSvc.h"
#include "GaudiKernel/IStateful.h"
#include "GaudiKernel/ITimelineSvc.h"

#include <string>
#include <vector>

// Extra include files (forward declarations should be sufficient)
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/IHiveWhiteBoard.h"
#include "GaudiKernel/IHistogramSvc.h"
#include "GaudiKernel/IConversionSvc.h"
#include "GaudiKernel/INTupleSvc.h"
#include "GaudiKernel/IChronoStatSvc.h"
#include "GaudiKernel/IRndmGenSvc.h"
#include "GaudiKernel/IAuditorSvc.h"
#include "GaudiKernel/IToolSvc.h"
#include "GaudiKernel/IMonitorSvc.h"
#include "GaudiKernel/IExceptionSvc.h"
#include "GaudiKernel/IAlgContextSvc.h"
#include "GaudiKernel/Property.h"
#include "GaudiKernel/System.h"
#include <Gaudi/PluginService.h>
#include "GaudiKernel/ToolHandle.h"
#include "GaudiKernel/CommonMessaging.h"

// For concurrency
#include "GaudiKernel/EventContext.h"
#include "GaudiKernel/DataHandle.h"
#include "GaudiKernel/IDataHandleHolder.h"

class IAlgTool;
class ToolHandleInfo;

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

/** @class Algorithm Algorithm.h GaudiKernel/Algorithm.h
 *
 *  Base class from which all concrete algorithm classes should
 *  be derived.
 *
 *  In order for a concrete algorithm class to do anything
 *  useful the methods initialize(), execute() and finalize()
 *  should be overridden.
 *
 *  The base class provides utility methods for accessing
 *  standard services (event data service etc.); for declaring
 *  properties which may be configured by the job options
 *  service; and for creating sub algorithms.
 *  The only base class functionality which may be used in the
 *  constructor of a concrete algorithm is the declaration of
 *  member variables as properties. All other functionality,
 *  i.e. the use of services and the creation of sub-algorithms,
 *  may be used only in initialize() and afterwards (see the
 *  Gaudi user guide).
 *
 *  @author Paul Maley
 *  @author Pere Mato
 *  @author David Quarrie
 *  @date   1998
 */
class GAUDI_API Algorithm: public CommonMessaging<implements4<IAlgorithm, IDataHandleHolder, IProperty, IStateful>> {
public:
#ifndef __REFLEX__
  typedef Gaudi::PluginService::Factory<IAlgorithm*,
                                        const std::string&,
                                        ISvcLocator*> Factory;
#endif

  /** Constructor
   *  @param name    The algorithm object's name
   *  @param svcloc  A pointer to a service location service
   */
  Algorithm( const std::string& name, ISvcLocator *svcloc,
             const std::string& version=PACKAGE_VERSION );
  /// Destructor
  ~Algorithm() override = default;

  /** Reinitialization method invoked by the framework. This method is responsible
   *  for any reinitialization required by the framework itself.
   *  It will in turn invoke the reinitialize() method of the derived algorithm,
   * and of any sub-algorithms which it creates.
   */
  StatusCode sysStart() override;

  /** Initialization method invoked by the framework. This method is responsible
   *  for any bookkeeping of initialization required by the framework itself.
   *  It will in turn invoke the initialize() method of the derived algorithm,
   * and of any sub-algorithms which it creates.
   */
  StatusCode sysInitialize() override;


  /** Reinitialization method invoked by the framework. This method is responsible
   *  for any reinitialization required by the framework itself.
   *  It will in turn invoke the reinitialize() method of the derived algorithm,
   * and of any sub-algorithms which it creates.
   */
  StatusCode sysReinitialize() override;

  /** Restart method invoked by the framework.
      It will in turn invoke the restart() method of the derived algorithm,
      and of any sub-algorithms which it creates.
  */
  StatusCode sysRestart() override;

  /** The actions to be performed by the algorithm on an event. This method is
   * invoked once per event for top level algorithms by the application
   *  manager.
   *  This method invokes execute() method.
   *  For sub-algorithms either the sysExecute() method or execute() method
   *  must be EXPLICITLY invoked by  the parent algorithm.
   */
  StatusCode sysExecute() override;

  /** System stop. This method invokes the stop() method of a concrete
      algorithm and the stop() methods of all of that algorithm's sub algorithms.
  */
  StatusCode sysStop() override;

  /** System finalization. This method invokes the finalize() method of a
   *  concrete algorithm and the finalize() methods of all of that algorithm's
   *  sub algorithms.
   */
  StatusCode sysFinalize() override;

  /** beginRun method invoked by the framework. This method is responsible
      for any beginRun actions required by the framework itself.
      It will in turn invoke the beginRun() method of the derived algorithm,
      and of any sub-algorithms which it creates.
  */
  StatusCode sysBeginRun( ) override;

  /** endRun method invoked by the framework. This method is responsible
      for any endRun actions required by the framework itself.
      It will in turn invoke the endRun() method of the derived algorithm,
      and of any sub-algorithms which it creates.
  */
  StatusCode sysEndRun( ) override;

  /** The identifying name of the algorithm object. This is the name of a
   *  particular instantiation of an algorithm object as opposed to the name
   *  of the algorithm itself, e.g. "LinearTrackFit" may be the name of a
   *  concrete algorithm class,
   *  whereas "ApproxTrackFit" and "BestTrackFit" may be two instantiations
   *  of the class configured to find tracks with different fit criteria.
   */
  const std::string& name() const override;
  const Gaudi::StringKey& nameKey() const override;

  /** The type of the algorithm object.
   */
  const std::string& type() const override { return m_type;}
  void setType(const std::string& type) override { m_type = type;} //BH, TODO: move to proper place

  const std::string& version() const override;

  unsigned int index() const override;

  /// Dummy implementation of IStateful::configure() method
  StatusCode configure() override { return StatusCode::SUCCESS ; }
  /// Dummy implementation of IStateful::terminate() method
  StatusCode terminate() override { return StatusCode::SUCCESS ; }

  /// the default (empty) implementation of IStateful::initialize() method
  StatusCode initialize() override { return StatusCode::SUCCESS ; }
  /// the default (empty) implementation of IStateful::start() method
  StatusCode start() override { return StatusCode::SUCCESS ; }
  /// the default (empty) implementation of IStateful::stop() method
  StatusCode stop() override { return StatusCode::SUCCESS ; }
  /// the default (empty) implementation of IStateful::finalize() method
  StatusCode finalize() override { return StatusCode::SUCCESS ; }
  /// the default (empty) implementation of IStateful::reinitialize() method
  StatusCode reinitialize() override;
  /// the default (empty) implementation of IStateful::restart() method
  StatusCode restart() override;
  /// returns the current state of the algorithm
  Gaudi::StateMachine::State FSMState() const override { return m_state; }
  /// returns the state the algorithm will be in after the ongoing transition
  Gaudi::StateMachine::State targetFSMState() const override { return m_targetState; }

  /// Has this algorithm been executed since the last reset?
  bool isExecuted( ) const override;

  /// Set the executed flag to the specified state
  void setExecuted( bool state ) override;

  /** Reset the executed state of the Algorithm for the duration
   *  of the current event.
   */
  void resetExecuted( ) override;

  /** Algorithm begin run. This method is called at the beginning
   *  of the event loop.
   */
  StatusCode beginRun() override;

  /// Algorithm end run. This method is called at the end of the event loop
  StatusCode endRun() override;


  /// Is this algorithm enabled or disabled?
  bool isEnabled( ) const override;

  /// Did this algorithm pass or fail its filter criterion for the last event?
  bool filterPassed( ) const override;

  /// Set the filter passed flag to the specified state
  void setFilterPassed( bool state ) override;

  /// Get the number of failures of the algorithm.
  inline int errorCount() const { return m_errorCount; }

  /// Access a service by name, creating it if it doesn't already exist.
  template <class T>
  StatusCode service
  ( const std::string& name, T*& psvc, bool createIf = true ) const {
    return service_i(name, createIf, T::interfaceID(), (void**)&psvc);
  }

  /// Access a service by name and type, creating it if it doesn't already exist.
  template <class T>
  StatusCode service( const std::string& svcType, const std::string& svcName,
                      T*& psvc) const {
    return service_i(svcType, svcName, T::interfaceID(), (void**)&psvc);
  }

  /// Return a pointer to the service identified by name (or "type/name")
  SmartIF<IService> service(const std::string& name, const bool createIf = true, const bool quiet = false) const;

  template <class T>
  SmartIF<T> service( const std::string& name, bool createIf = true, bool quiet = false ) const {
    return service(name,createIf,quiet).as<T>();
  }

  /** The standard auditor service.May not be invoked before sysInitialize()
   *  has been invoked.
   */
  SmartIF<IAuditorSvc>& auditorSvc() const;

  /** The standard Chrono & Stat service,
   *  Return a pointer to the service if present
   */
  SmartIF<IChronoStatSvc>& chronoSvc() const;
  /// Obsoleted name, kept due to the backwards compatibility
  SmartIF<IChronoStatSvc>& chronoStatService() const;

  /** The standard detector data service.
   *  May not be invoked before sysInitialize() has been invoked.
   */
  SmartIF<IDataProviderSvc>& detSvc() const;

  /// Obsoleted name, kept due to the backwards compatibility
  SmartIF<IDataProviderSvc>& detDataService() const;

  /** The standard detector data persistency conversion service.
   *  May not be invoked before sysInitialize() has been invoked.
   */
  SmartIF<IConversionSvc>& detCnvSvc() const;

  /// Obsoleted name, kept due to the backwards compatibility
  SmartIF<IConversionSvc>& detDataCnvService() const;

  /** The standard event data service.
   *  May not be invoked before sysInitialize() has been invoked.
   */
  SmartIF<IDataProviderSvc>& eventSvc() const;
  /// shortcut for  method eventSvc
  SmartIF<IDataProviderSvc>& evtSvc  () const { return eventSvc() ; }
  /// Obsoleted name, kept due to the backwards compatibility
  SmartIF<IDataProviderSvc>& eventDataService() const;

  /** The standard event data persistency conversion service.
   *  May not be invoked before sysInitialize() has been invoked.
   */
  SmartIF<IConversionSvc>&   eventCnvSvc() const;
  /// Obsoleted name, kept due to the backwards compatibility
  SmartIF<IConversionSvc>&   eventDataCnvService() const;

  /** The standard histogram service.
   *  May not be invoked before sysInitialize() has been invoked.
   */
  SmartIF<IHistogramSvc>& histoSvc() const;
  /// Obsoleted name, kept due to the backwards compatibility
  SmartIF<IHistogramSvc>& histogramDataService() const;

  /** The standard N tuple service.
   *  Returns a pointer to the N tuple service if present.
   */
  SmartIF<INTupleSvc>& ntupleSvc() const;

  /// Obsoleted name, kept due to the backwards compatibility
  SmartIF<INTupleSvc>& ntupleService() const;

  /** AIDA-based NTuple service
   *  Returns a pointer to the AIDATuple service if present.
   */
  // SmartIF<IAIDATupleSvc>& atupleSvc() const;


  /** The standard RandomGen service,
   *  Return a pointer to the service if present
   */
  SmartIF<IRndmGenSvc>& randSvc() const;

  /// The standard ToolSvc service, Return a pointer to the service if present
  SmartIF<IToolSvc>& toolSvc() const;

  /// Get the exception Service
  SmartIF<IExceptionSvc>& exceptionSvc() const;

  /// get Algorithm Context Service
  SmartIF<IAlgContextSvc>& contextSvc() const ;

  SmartIF<ITimelineSvc>& timelineSvc() const ;

  /** The standard service locator.
   *  Returns a pointer to the service locator service.
   *  This service may be used by an algorithm to request
   *  any services it requires in addition to those provided by default.
   */
  SmartIF<ISvcLocator>& serviceLocator() const override;
  /// shortcut for method serviceLocator
  SmartIF<ISvcLocator>& svcLoc        () const { return serviceLocator() ; }

  SmartIF<IHiveWhiteBoard>& whiteboard() const;

  /// register for Algorithm Context Service?
  bool registerContext() const { return m_registerContext ; }

  /** Create a sub algorithm.
   *  A call to this method creates a child algorithm object.
   *  Note that the returned pointer is to Algorithm
   *  (as opposed to IAlgorithm), and thus the methods of IProperty
   *  are also available for the direct setting of the sub-algorithm's
   *  properties. Using this mechanism instead of creating daughter
   *  algorithms directly via the new operator is prefered since then
   *  the framework may take care of all of the necessary book-keeping.
   *  @param type The concrete algorithm class of the sub algorithm
   *  @param name The name to be given to the sub algorithm
   *  @param pSubAlg Set to point to the newly created algorithm object
   */
  StatusCode createSubAlgorithm( const std::string& type,
                                 const std::string& name, Algorithm*& pSubAlg );

  /// List of sub-algorithms. Returns a pointer to a vector of (sub) Algorithms
  const std::vector<Algorithm*>* subAlgorithms() const;

  /// List of sub-algorithms. Returns a pointer to a vector of (sub) Algorithms
  std::vector<Algorithm*>* subAlgorithms() ;

  /// Implementation of IProperty::setProperty
  StatusCode setProperty( const Property& p ) override;
  /// Implementation of IProperty::setProperty
  StatusCode setProperty( const std::string& s ) override;
  /// Implementation of IProperty::setProperty
  StatusCode setProperty( const std::string& n, const std::string& v) override;
  /// Implementation of IProperty::getProperty
  StatusCode getProperty(Property* p) const override;
  /// Implementation of IProperty::getProperty
  const Property& getProperty( const std::string& name) const override;
  /// Implementation of IProperty::getProperty
  StatusCode getProperty( const std::string& n, std::string& v ) const override;
  /// Implementation of IProperty::getProperties
  const std::vector<Property*>& getProperties( ) const override;
  /// Implementation of IProperty::hasProperty
  bool hasProperty(const std::string& name) const override;

  inline PropertyMgr * getPropertyMgr() { return m_propertyMgr; }

  /** Set the algorithm's properties.
   *  This method requests the job options service
   *  to set the values of any declared properties.
   *  The method is invoked from within sysInitialize() by the framework
   *  and does not need to be explicitly
   *  called by a concrete algorithm.
   */
  StatusCode setProperties();
  // ==========================================================================
  /** Declare the named property
   *
   *
   *  @code
   *
   *  MyAlg ( const std::string& name ,
   *          ISvcLocator*       pSvc )
   *     : Algorithm ( name , pSvc )
   *     , m_property1   ( ... )
   *     , m_property2   ( ... )
   *   {
   *     // declare the property
   *     declareProperty( "Property1" , m_property1 , "Doc for property #1" ) ;
   *
   *     // declare the property and attach the handler to it
   *     declareProperty( "Property2" , m_property2 , "Doc for property #2" )
   *        -> declareUpdateHandler( &MyAlg::handler_2 ) ;
   *
   *   }
   *  @endcode
   *
   *  @see PropertyMgr
   *  @see PropertyMgr::declareProperty
   *
   *  @param name the property name
   *  @param property the property itself,
   *  @param doc      the documentation string
   *  @return the actual property objects
   */
  template <class T>
  Property* declareProperty
  ( const std::string& name              ,
    T&                 property          ,
    const std::string& doc      = "none" ) const
  {
    return m_propertyMgr->declareProperty(name, property, doc);
  }
  // ==========================================================================
  /// Declare remote named properties
  Property* declareRemoteProperty
  ( const std::string& name       ,
    IProperty*         rsvc       ,
    const std::string& rname = "" ) const
  {
    return m_propertyMgr -> declareRemoteProperty ( name , rsvc , rname );
  }

  // ==========================================================================
  // declare Tools to the Algorithms

  template <class T>
    Property* declareProperty(const std::string& name,
			      ToolHandle<T>& hndl,
			      const std::string& doc = "none" ) const {

    Algorithm* a = const_cast<Algorithm*>(this);
    a->declareTool(hndl).ignore();

    return m_propertyMgr->declareProperty(name, hndl, doc);

  }


  // ==========================================================================
  /** @brief Access the monitor service
   *
   *   @attention Note that this method will return a NULL pointer if no monitor service is
   *              configured to be present. You must take this possibility into account when
   *              using the pointer
   *   @return Pointer to the Monitor service
   *   @retval NULL No monitor service is present
   *   @retval non-NULL A monitor service is present and available to be used
   */
  inline SmartIF<IMonitorSvc>& monitorSvc() const
  {
    // If not already located try to locate it without forcing a creation
    if ( !m_pMonitorSvc ){
      m_pMonitorSvc = service(m_monitorSvcName, false, true); // do not create and be quiet
    }
    return m_pMonitorSvc;
  }

  /** Declare monitoring information
      @param name Monitoring information name known to the external system
      @param var  Monitoring Listener address (the item to monitor...)
      @param desc Textual description of the information being monitored
  */
  template <class T>
  void declareInfo( const std::string& name,
                    const T& var,
                    const std::string& desc ) const
  {
    IMonitorSvc* mS = monitorSvc().get();
    if ( mS ) mS->declareInfo(name, var, desc, this);
  }

  /** Declare monitoring information (special case)
      @param name Monitoring information name known to the external system
      @param format Format information
      @param var  Monitoring Listener address
      @param size Monitoring Listener address size
      @param desc Textual description of the information being monitored
  */
  void declareInfo( const std::string& name,
                    const std::string& format,
                    const void* var,
                    int size,
                    const std::string& desc ) const
  {
    IMonitorSvc* mS = monitorSvc().get();
    if ( mS ) mS->declareInfo(name, format, var, size, desc, this);
  }

  // ==========================================================================
public:
  // ==========================================================================
  /** set the property form the value
   *
   *  @code
   *
   *  std::vector<double> data = ... ;
   *
   *  setProperty( "Data" , data ) ;
   *
   *  std::map<std::string,double> cuts = ... ;
   *  setProperty( "Cuts" , cuts ) ;
   *
   *  std::map<std::string,std::string> dict = ... ;
   *  setProperty( "Dictionary" , dict ) ;
   *
   *  @endcode
   *
   *  Note: the interface IProperty allows setting of the properties either
   *        directly from other properties or from strings only
   *
   *  This is very convenient in resetting of the default
   *  properties in the derived classes.
   *  E.g. without this method one needs to convert
   *  everything into strings to use IProperty::setProperty
   *
   *  @code
   *
   *    setProperty ( "OutputLevel" , "1"    ) ;
   *    setProperty ( "Enable"      , "True" ) ;
   *    setProperty ( "ErrorMax"    , "10"   ) ;
   *
   *  @endcode
   *
   *  For simple cases it is more or less ok, but for complicated properties
   *  it is just ugly..
   *
   *  @param name      name of the property
   *  @param value     value of the property
   *  @see Gaudi::Utils::setProperty
   *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
   *  @date 2007-05-13
   */
  template <class TYPE>
  StatusCode setProperty
  ( const std::string& name  ,
    const TYPE&        value )
  { return Gaudi::Utils::setProperty ( m_propertyMgr.get() , name , value ) ; }
  // ==========================================================================

  // For concurrency
  /// get the context
  EventContext* getContext(){return m_event_context;}

  /// set the context
  void setContext(EventContext* context){m_event_context = context;}

  // From IDataHandleHolder:

 protected:
  virtual void declareInput(Gaudi::DataHandle* im) override {
    m_inputHandles.push_back(im);
  }
  virtual void declareOutput(Gaudi::DataHandle* im) override {
    m_outputHandles.push_back(im);
  }

 public:
  virtual std::vector<Gaudi::DataHandle*> inputHandles() const override { return m_inputHandles; }
  virtual std::vector<Gaudi::DataHandle*> outputHandles() const override { return m_outputHandles; }

  virtual const DataObjIDColl& extraInputDeps() const override {
    return m_extInputDataObjs;
  }
  virtual const DataObjIDColl& extraOutputDeps() const override {
    return m_extOutputDataObjs;
  }

  virtual void acceptDHVisitor(IDataHandleVisitor*) const override ;

  const DataObjIDColl& inputDataObjs() const { return m_inputDataObjs; }
  const DataObjIDColl& outputDataObjs() const { return m_outputDataObjs; }

  void commitHandles() override;

 private:
  std::vector<Gaudi::DataHandle*> m_inputHandles, m_outputHandles;
  DataObjIDColl m_inputDataObjs, m_outputDataObjs;

  DataObjIDColl m_extInputDataObjs, m_extOutputDataObjs;

 public:

  void registerTool(IAlgTool * tool) const;
  void deregisterTool(IAlgTool * tool) const;

  template<class T>
  StatusCode declareTool(ToolHandle<T> &handle,
		  std::string toolTypeAndName = "",
		  bool createIf = true) {

	  if (toolTypeAndName == "")
		  toolTypeAndName = handle.typeAndName();

	  StatusCode sc = handle.initialize(toolTypeAndName,
			  handle.isPublic() ? nullptr : this,
					  createIf);
	  if (UNLIKELY(!sc)) {
		  throw GaudiException{std::string{"Cannot create handle for "} +
			  (handle.isPublic() ? "public" : "private") +
			  " tool " + toolTypeAndName,
			  name(), sc};
	  }

	  m_toolHandles.push_back(&handle);

	  return sc;
  }

  const std::vector<IAlgTool *> & tools() const;

protected:

  std::vector<IAlgTool *> & tools();


  // // adds declared in- and outputs of subAlgorithms to own DOHs
  //  void addSubAlgorithmDataObjectHandles();

private:
  //place IAlgTools defined via ToolHandles in m_tools
  void initToolHandles() const;

public:

  /// Specifies the clonability of the algorithm
  bool isClonable () const override { return m_isClonable; }

  /// Return the cardinality
  unsigned int cardinality () const override { return m_cardinality; }

  const std::vector<std::string>& neededResources () const override { return m_neededResources; }

protected:

  /// Has the Algorithm already been initialized?
  bool isInitialized( ) const override { return Gaudi::StateMachine::INITIALIZED == m_state; }

  /// Has the Algorithm already been finalized?
  bool isFinalized( ) const  override{ return Gaudi::StateMachine::CONFIGURED == m_state; }

  /// Event specific data for multiple event processing
  EventContext* m_event_context;

  /// set instantiation index of Alg
  void setIndex(const unsigned int& idx) override;

private:

  Gaudi::StringKey m_name;       ///< Algorithm's name for identification
  std::string m_type;            ///< Algorithm's type
  std::string m_version;         ///< Algorithm's version
  unsigned int m_index;          ///< Algorithm's index
  std::vector<Algorithm *> m_subAlgms; ///< Sub algorithms

  //tools used by algorithm
  mutable std::vector<IAlgTool *> m_tools;
  mutable std::vector<BaseToolHandle *> m_toolHandles;


private:
  mutable SmartIF<IMessageSvc>      m_MS;       ///< Message service
  mutable SmartIF<IDataProviderSvc> m_EDS;      ///< Event data service
  mutable SmartIF<IHiveWhiteBoard>  m_WB;       ///< Event data service (whiteboard)
  mutable SmartIF<IConversionSvc>   m_ECS;      ///< Event conversion service
  mutable SmartIF<IDataProviderSvc> m_DDS;      ///< Detector data service
  mutable SmartIF<IConversionSvc>   m_DCS;      ///< Detector conversion service
  mutable SmartIF<IHistogramSvc>    m_HDS;      ///< Histogram data service
  mutable SmartIF<INTupleSvc>       m_NTS;      ///< N tuple service
  //  mutable SmartIF<IAIDATupleSvc>    m_ATS;      ///< AIDA tuple service
  mutable SmartIF<IChronoStatSvc>   m_CSS;      ///< Chrono & Stat Service
  mutable SmartIF<IRndmGenSvc>      m_RGS;      ///< Random Number Generator Service
  mutable SmartIF<IExceptionSvc>    m_EXS;      ///< Exception Handler Service
  mutable SmartIF<IAuditorSvc>      m_pAuditorSvc; ///< Auditor Service
  mutable SmartIF<IToolSvc>         m_ptoolSvc;    ///< ToolSvc Service
  mutable SmartIF<IMonitorSvc>      m_pMonitorSvc; ///< Online Monitoring Service
  mutable SmartIF<IAlgContextSvc>   m_contextSvc ; ///< Algorithm Context Service

  mutable SmartIF<ITimelineSvc>   m_timelineSvc ; ///< Timeline Service

  bool  m_registerContext = false ; ///< flag to register for Algorithm Context Service
  std::string               m_monitorSvcName; ///< Name to use for Monitor Service
  SmartIF<ISvcLocator>  m_pSvcLocator;      ///< Pointer to service locator service
 protected:
  SmartIF<PropertyMgr> m_propertyMgr;      ///< For management of properties

  /// Hook for for derived classes to provide a custom visitor for data handles.
  std::unique_ptr<IDataHandleVisitor> m_updateDataHandles;

 private:
  IntegerProperty m_outputLevel;   ///< Algorithm output level
  int          m_errorMax;         ///< Algorithm Max number of errors
  int          m_errorCount;       ///< Algorithm error counter
  BooleanProperty m_auditInit;     ///< global flag for auditors
  bool         m_auditorInitialize;///< flag for auditors in "initialize()"
  bool         m_auditorReinitialize;///< flag for auditors in "Reinitialize()"
  bool         m_auditorRestart;   ///< flag for auditors in "Restart()"
  bool         m_auditorExecute;   ///< flag for auditors in "execute()"
  bool         m_auditorFinalize;  ///< flag for auditors in "finalize()"
  bool         m_auditorBeginRun;  ///< flag for auditors in "beginRun()"
  bool         m_auditorEndRun;    ///< flag for auditors in "endRun()"
  bool         m_auditorStart;///< flag for auditors in "initialize()"
  bool         m_auditorStop;///< flag for auditors in "Reinitialize()"
  bool         m_filterPassed = true;     ///< Filter passed flag
  bool         m_isEnabled = true;        ///< Algorithm is enabled flag
  bool         m_isExecuted = false;      ///< Algorithm is executed flag
  mutable bool m_toolHandlesInit = false;  /// flag indicating whether ToolHandle tools have been added to m_tools
  Gaudi::StateMachine::State m_state = Gaudi::StateMachine::CONFIGURED;            ///< Algorithm has been initialized flag
  Gaudi::StateMachine::State m_targetState = Gaudi::StateMachine::CONFIGURED;      ///< Algorithm has been initialized flag
  bool         m_isFinalized;      ///< Algorithm has been finalized flag

  bool         m_doTimeline; // send events to TimelineSvc

  bool         m_isClonable; ///< The algorithm clonability of the algorithm
  unsigned int m_cardinality; ///< The maximum number of clones that can exist
  std::vector<std::string> m_neededResources; ///< The named resources needed during event looping

  /// implementation of service method
  StatusCode service_i(const std::string& svcName,
                       bool createIf,
                       const InterfaceID& iid,
                       void** ppSvc) const;
  StatusCode service_i(const std::string& svcType,
                       const std::string& svcName,
                       const InterfaceID& iid,
                       void** ppSvc) const;

  /// Private Copy constructor: NO COPY ALLOWED
  Algorithm(const Algorithm& a);

  /// Private assignment operator: NO ASSIGNMENT ALLOWED
  Algorithm& operator=(const Algorithm& rhs);
};

#ifndef GAUDI_NEW_PLUGIN_SERVICE
template <class T>
class AlgFactory {
public:
#ifndef __REFLEX__
  template <typename S, typename... Args>
  static typename S::ReturnType create(Args&&... args) {
    return new T(std::forward<Args>(args)...);
  }
#endif
};

// Macros to declare component factories
#define DECLARE_ALGORITHM_FACTORY(x) \
  DECLARE_FACTORY_WITH_CREATOR(x, AlgFactory< x >, Algorithm::Factory)
#define DECLARE_NAMED_ALGORITHM_FACTORY(x, n) \
  DECLARE_FACTORY_WITH_CREATOR_AND_ID(x, AlgFactory< x >, \
                                      #n, Algorithm::Factory)
#define DECLARE_NAMESPACE_ALGORITHM_FACTORY(n, x) \
  DECLARE_ALGORITHM_FACTORY(n::x)

#else

// Macros to declare component factories
#define DECLARE_ALGORITHM_FACTORY(x)              DECLARE_COMPONENT(x)
#define DECLARE_NAMED_ALGORITHM_FACTORY(x, n)     DECLARE_COMPONENT_WITH_ID(x, #n)
#define DECLARE_NAMESPACE_ALGORITHM_FACTORY(n, x) DECLARE_COMPONENT(n::x)

#endif

#endif //GAUDIKERNEL_ALGORITHM_H
