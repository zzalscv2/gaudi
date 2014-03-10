#ifndef GAUDIHIVE_ROUNDROBINSCHEDULERSVC_H
#define GAUDIHIVE_ROUNDROBINSCHEDULERSVC_H

// Framework include files
#include "GaudiKernel/IScheduler.h"
#include "GaudiKernel/IRunable.h" 
#include "GaudiKernel/Service.h" 
#include "GaudiKernel/IAlgResourcePool.h"

#include "AlgResourcePool.h"
#include "ControlFlowManager.h"
#include "DataFlowManager.h"


// C++ include files
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <thread>

// External libs
#include "tbb/concurrent_queue.h"


//---------------------------------------------------------------------------

/**@class RoundRobinSchedulerSvc RoundRobinSchedulerSvc.h 
 * 
 * The RoundRobinSchedulerSvc implements the IScheduler interface.
 * It deals with  multiple events and tries to handle all events
 * algorithm type by algorithm type, using one single thread. 
 * It serves as simple implementation against the concurrent state machine
 * and provides a test for instruction cache locality 
 * 
 *  @author  Benedikt Hegner 
 *  @version 1.0
 */
class RoundRobinSchedulerSvc: public extends1<Service, IScheduler> {
public:
  /// Constructor
  RoundRobinSchedulerSvc( const std::string& name, ISvcLocator* svc );

  /// Destructor
  ~RoundRobinSchedulerSvc();

  /// Initialise
  virtual StatusCode initialize();
  
  /// Finalise
  virtual StatusCode finalize();  

  /// Make an event available to the scheduler
  virtual StatusCode pushNewEvent(EventContext* eventContext);

  // Make multiple events available to the scheduler
  virtual StatusCode pushNewEvents(std::vector<EventContext*>& eventContexts);
  
  /// Blocks until an event is availble
  virtual StatusCode popFinishedEvent(EventContext*& eventContext);  

  /// Try to fetch an event from the scheduler
  virtual StatusCode tryPopFinishedEvent(EventContext*& eventContext);  

  /// Get free slots number
  virtual unsigned int freeSlots();


private:
  
  StatusCode processEvents();

  /// Decide if the top alglist or its flat version has to be used
  bool m_useTopAlgList;
  SmartIF<IAlgResourcePool> m_algResourcePool;
  
  //control flow manager
  concurrency::ControlFlowManager m_controlFlow;

  /// Vector to bookkeep the information necessary to the index2name conversion
  std::vector<std::string> m_algname_vect;

  /// Map to bookkeep the information necessary to the name2index conversion
  std::unordered_map<std::string,unsigned int> m_algname_index_map;

  /// Ugly, will disappear when the deps are declared only within the C++ code of the algos.
  std::vector<std::vector<std::string>> m_algosDependencies;

  /// Cache the list of algs to be executed
  std::list<IAlgorithm*> m_algList;
  
  /// Queue of finished events
  tbb::concurrent_bounded_queue<EventContext*> m_finishedEvents;
  
  /// The number of free slots (0 or 1)
  int m_freeSlots;
  std::vector<EventContext*> m_evtCtx_buffer;
  
};

#endif // GAUDIHIVE_ROUNDROBINSCHEDULERSVC_H