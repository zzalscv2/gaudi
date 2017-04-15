#include "AvalancheSchedulerSvc.h"
#include "AlgResourcePool.h"
#include "AlgoExecutionTask.h"
#include "PRGraphVisitors.h"
#include "IOBoundAlgTask.h"

// Framework includes
#include "GaudiAlg/GaudiAlgorithm.h"
#include "GaudiKernel/Algorithm.h" // will be IAlgorithm if context getter promoted to interface
#include "GaudiKernel/DataHandleHolderVisitor.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/IDataManagerSvc.h"
#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/ThreadLocalContext.h"
#include "GaudiKernel/ConcurrencyFlags.h"

// C++
#include <algorithm>
#include <map>
#include <queue>
#include <sstream>
#include <unordered_set>

// External libs
#include "boost/thread.hpp"
#include "boost/tokenizer.hpp"
#include "boost/algorithm/string.hpp"
// DP waiting for the TBB service
#include "tbb/task_scheduler_init.h"

std::mutex AvalancheSchedulerSvc::m_ssMut;
std::list<AvalancheSchedulerSvc::SchedulerState> AvalancheSchedulerSvc::m_sState;

// Instantiation of a static factory class used by clients to create instances of this service
DECLARE_SERVICE_FACTORY( AvalancheSchedulerSvc )

//===========================================================================
// Infrastructure methods

/**
 * Here, among some "bureaucracy" operations, the scheduler is activated,
 * executing the activate() function in a new thread.
 * In addition the algorithms list is acquired from the algResourcePool.
**/
StatusCode AvalancheSchedulerSvc::initialize() {

  // Initialise mother class (read properties, ...)
  StatusCode sc( Service::initialize() );
  if ( !sc.isSuccess() ) warning() << "Base class could not be initialized" << endmsg;

  // Get hold of the TBBSvc. This should initialize the thread pool
  m_threadPoolSvc = serviceLocator()->service( "ThreadPoolSvc" );
  if ( !m_threadPoolSvc.isValid() ) {
    fatal() << "Error retrieving ThreadPoolSvc" << endmsg;
    return StatusCode::FAILURE;
  }

  // Activate the scheduler in another thread.
  info() << "Activating scheduler in a separate thread" << endmsg;
  m_thread = std::thread( std::bind( &AvalancheSchedulerSvc::activate, this ) );

  while ( m_isActive != ACTIVE ) {
    if ( m_isActive == FAILURE ) {
      fatal() << "Terminating initialization" << endmsg;
      return StatusCode::FAILURE;
    } else {
      info() << "Waiting for AvalancheSchedulerSvc to activate" << endmsg;
      sleep( 1 );
    }
  }

  // Get the algo resource pool
  m_algResourcePool = serviceLocator()->service( "AlgResourcePool" );
  if ( !m_algResourcePool.isValid() ) {
    fatal() << "Error retrieving AlgoResourcePool" << endmsg;
    return StatusCode::FAILURE;
  }

  m_algExecStateSvc = serviceLocator()->service("AlgExecStateSvc");
  if (!m_algExecStateSvc.isValid()) {
    fatal() << "Error retrieving AlgExecStateSvc" << endmsg;
    return StatusCode::FAILURE;
  }

  // Get Whiteboard
  m_whiteboard = serviceLocator()->service( m_whiteboardSvcName );
  if ( !m_whiteboard.isValid() ) {
    fatal() << "Error retrieving EventDataSvc interface IHiveWhiteBoard." << endmsg;
    return StatusCode::FAILURE;
  }

  // Get dedicated scheduler for I/O-bound algorithms
  if ( m_useIOBoundAlgScheduler ) {
    m_IOBoundAlgScheduler = serviceLocator()->service( m_IOBoundAlgSchedulerSvcName );
    if ( !m_IOBoundAlgScheduler.isValid() )
      fatal() << "Error retrieving IOBoundSchedulerAlgSvc interface IAccelerator." << endmsg;
  }

  // Check the MaxEventsInFlight parameters and react
  // Deprecated for the moment
  size_t numberOfWBSlots = m_whiteboard->getNumberOfStores();
  if ( m_maxEventsInFlight != 0 ) {
    warning() << "Property MaxEventsInFlight was set. This works but it's deprecated. "
              << "Please migrate your code options files." << endmsg;

    if ( m_maxEventsInFlight != (int)numberOfWBSlots ) {
      warning() << "In addition, the number of events in flight (" << m_maxEventsInFlight
                << ") differs from the slots in the whiteboard (" << numberOfWBSlots
                << "). Setting the number of events in flight to " << numberOfWBSlots << endmsg;
    }
  }

  // Align the two quantities
  m_maxEventsInFlight = numberOfWBSlots;

  // Set the number of free slots
  m_freeSlots = m_maxEventsInFlight;

  // set global concurrency flags
  Gaudi::Concurrency::ConcurrencyFlags::setNumConcEvents( numberOfWBSlots );

  if ( m_algosDependencies.size() != 0 ) {
    warning() << " ##### Property AlgosDependencies is deprecated and ignored."
              << " FIX your job options #####" << endmsg;
  }

  // Get the list of algorithms
  const std::list<IAlgorithm*>& algos = m_algResourcePool->getFlatAlgList();
  const unsigned int algsNumber       = algos.size();
  info() << "Found " << algsNumber << " algorithms" << endmsg;

  /* Dependencies
   1) Look for handles in algo, if none
   2) Assume none are required
  */

  DataObjIDColl globalInp, globalOutp;

  // figure out all outputs
  for (IAlgorithm* ialgoPtr : algos) {
    Algorithm* algoPtr = dynamic_cast<Algorithm*>(ialgoPtr);
    if (!algoPtr) {
      fatal() << "Could not convert IAlgorithm into Algorithm: this will result in a crash." << endmsg;
    }
    for (auto id : algoPtr->outputDataObjs()) {
        auto r = globalOutp.insert(id);
        if (!r.second) {
           warning() << "multiple algorithms declare " << id << " as output! could be a single instance in multiple paths though, or control flow may guarantee only one runs...!" << endmsg;
        }
    }
  }
  info() << "outputs:\n" ;
  for (const auto& i : globalOutp ) {
      info() << i << '\n' ;
  }
  info() << endmsg;



  info() << "Data Dependencies for Algorithms:";

  std::vector<DataObjIDColl> m_algosDependencies;
  for ( IAlgorithm* ialgoPtr : algos ) {
    Algorithm* algoPtr = dynamic_cast<Algorithm*>( ialgoPtr );
    if ( nullptr == algoPtr )
      fatal() << "Could not convert IAlgorithm into Algorithm: this will result in a crash." << endmsg;

    info() << "\n  " << algoPtr->name();

    // FIXME
    DataObjIDColl algoDependencies;
    if ( !algoPtr->inputDataObjs().empty() || !algoPtr->outputDataObjs().empty() ) {
      for ( auto id : algoPtr->inputDataObjs() ) {
        info() << "\n    o INPUT  " << id;
        if (id.key().find(":")!=std::string::npos) {
            info() << " contains alternatives which require resolution... " << endmsg;
            auto tokens = boost::tokenizer<boost::char_separator<char>>{id.key(),boost::char_separator<char>{":"}};
            auto itok = std::find_if( tokens.begin(), tokens.end(),
                                     [&](const std::string& t) {
                return globalOutp.find( DataObjID{t} ) != globalOutp.end();
            } );
            if (itok!=tokens.end()) {
                info() << "found matching output for " << *itok << " -- updating scheduler info" << endmsg;
                id.updateKey(*itok);
            } else {
                error() << "failed to find alternate in global output list" << endmsg;
            }
        }
        algoDependencies.insert( id );
        globalInp.insert( id );
      }
      for ( auto id : algoPtr->outputDataObjs() ) {
        info() << "\n    o OUTPUT " << id;
        if (id.key().find(":")!=std::string::npos) {
            info() << " alternatives are NOT allowed for outputs..." << endmsg;
        }
      }
    } else {
      info() << "\n      none";
    }
    m_algosDependencies.emplace_back( algoDependencies );
  }
  info() << endmsg;

  // Fill the containers to convert algo names to index
  m_algname_vect.reserve( algsNumber );
  unsigned int index = 0;
  for ( IAlgorithm* algo : algos ) {
    const std::string& name   = algo->name();
    m_algname_index_map[name] = index;
    m_algname_vect.emplace_back( name );
    index++;
  }

  // Check if we have unmet global input dependencies
  if ( m_checkDeps ) {
    DataObjIDColl unmetDep;
    for ( auto o : globalInp ) {
      if ( globalOutp.find( o ) == globalOutp.end() ) {
        unmetDep.insert( o );
      }
    }

    if ( unmetDep.size() > 0 ) {
      fatal() << "The following unmet INPUT data dependencies were found: ";
      for ( auto& o : unmetDep ) {
        fatal() << "\n   o " << o << "    required by Algorithm: ";
        for ( size_t i = 0; i < m_algosDependencies.size(); ++i ) {
          if ( m_algosDependencies[i].find( o ) != m_algosDependencies[i].end() ) {
            fatal() << "\n       * " << m_algname_vect[i];
          }
        }
      }
      fatal() << endmsg;
      return StatusCode::FAILURE;
    } else {
      info() << "No unmet INPUT data dependencies were found" << endmsg;
    }
  }

  // prepare the control flow part
  const AlgResourcePool* algPool = dynamic_cast<const AlgResourcePool*>( m_algResourcePool.get() );
  sc = m_efManager.initialize( algPool->getPRGraph(), m_algname_index_map, m_eventSlots, m_optimizationMode );
  unsigned int controlFlowNodeNumber = m_efManager.getPrecedenceRulesGraph()->getControlFlowNodeCounter();

  // Shortcut for the message service
  SmartIF<IMessageSvc> messageSvc( serviceLocator() );
  if ( !messageSvc.isValid() ) error() << "Error retrieving MessageSvc interface IMessageSvc." << endmsg;

  m_eventSlots.assign( m_maxEventsInFlight,
                       EventSlot( m_algosDependencies, algsNumber, controlFlowNodeNumber, messageSvc ) );
  std::for_each( m_eventSlots.begin(), m_eventSlots.end(), []( EventSlot& slot ) { slot.complete = true; } );

  // Clearly inform about the level of concurrency
  info() << "Concurrency level information:" << endmsg;
  info() << " o Number of events in flight: " << m_maxEventsInFlight << endmsg;
  info() << " o Number of algorithms in flight: " << m_maxAlgosInFlight << endmsg;
  info() << " o TBB thread pool size: " << m_threadPoolSize << endmsg;

  // Simulating execution flow by only analyzing the graph topology and logic
  if ( m_simulateExecution ) {
    auto vis = concurrency::RunSimulator( m_eventSlots[0] );
    m_efManager.simulateExecutionFlow( vis );
  }

  return sc;
}
//---------------------------------------------------------------------------

/**
 * Here the scheduler is deactivated and the thread joined.
**/
StatusCode AvalancheSchedulerSvc::finalize() {

  StatusCode sc( Service::finalize() );
  if ( !sc.isSuccess() ) warning() << "Base class could not be finalized" << endmsg;

  sc = deactivate();
  if ( !sc.isSuccess() ) warning() << "Scheduler could not be deactivated" << endmsg;

  info() << "Joining Scheduler thread" << endmsg;
  m_thread.join();

  // Final error check after thread pool termination
  if ( m_isActive == FAILURE ) {
    error() << "problems in scheduler thread" << endmsg;
    return StatusCode::FAILURE;
  }

  // m_efManager.getPrecedenceRulesGraph()->dumpExecutionPlan();

  return sc;
}
//---------------------------------------------------------------------------
/**
 * Activate the scheduler. From this moment on the queue of actions is
 * checked. The checking will stop when the m_isActive flag is false and the
 * queue is not empty. This will guarantee that all actions are executed and
 * a stall is not created.
 * The TBB pool must be initialised in the thread from where the tasks are
 * launched (http://threadingbuildingblocks.org/docs/doxygen/a00342.html)
 * The scheduler is initialised here since this method runs in a separate
 * thread and spawns the tasks (through the execution of the lambdas)
 **/
void AvalancheSchedulerSvc::activate() {

  if (msgLevel(MSG::DEBUG))
    debug() << "AvalancheSchedulerSvc::activate()" << endmsg;

  if ( m_threadPoolSvc->initPool( m_threadPoolSize ).isFailure() ) {
    error() << "problems initializing ThreadPoolSvc" << endmsg;
    m_isActive = FAILURE;
    return;
  }

  // Wait for actions pushed into the queue by finishing tasks.
  action thisAction;
  StatusCode sc( StatusCode::SUCCESS );

  m_isActive = ACTIVE;

  // Continue to wait if the scheduler is running or there is something to do
  info() << "Start checking the actionsQueue" << endmsg;
  while ( m_isActive == ACTIVE or m_actionsQueue.size() != 0 ) {
    m_actionsQueue.pop( thisAction );
    sc = thisAction();
    if ( sc != StatusCode::SUCCESS )
      verbose() << "Action did not succeed (which is not bad per se)." << endmsg;
    else
      verbose() << "Action succeeded." << endmsg;
  }

  info() << "Terminating thread-pool resources" << endmsg;
  if ( m_threadPoolSvc->terminatePool().isFailure() ) {
    error() << "Problems terminating thread pool" << endmsg;
    m_isActive = FAILURE;
  }
}

//---------------------------------------------------------------------------

/**
 * Deactivates the scheduler. Two actions are pushed into the queue:
 *  1) Drain the scheduler until all events are finished.
 *  2) Flip the status flag m_isActive to false
 * This second action is the last one to be executed by the scheduler.
 */
StatusCode AvalancheSchedulerSvc::deactivate() {

  if ( m_isActive == ACTIVE ) {
    // Drain the scheduler
    m_actionsQueue.push( std::bind( &AvalancheSchedulerSvc::m_drain, this ) );
    // This would be the last action
    m_actionsQueue.push( [this]() -> StatusCode {
      m_isActive = INACTIVE;
      return StatusCode::SUCCESS;
    } );
  }

  return StatusCode::SUCCESS;
}

//===========================================================================

//===========================================================================
// Utils and shortcuts

inline const std::string& AvalancheSchedulerSvc::index2algname( unsigned int index ) {
  return m_algname_vect[index];
}

//---------------------------------------------------------------------------

inline unsigned int AvalancheSchedulerSvc::algname2index( const std::string& algoname ) {
  unsigned int index = m_algname_index_map[algoname];
  return index;
}

//===========================================================================
// EventSlot management
/**
 * Add event to the scheduler. There are two cases possible:
 *  1) No slot is free. A StatusCode::FAILURE is returned.
 *  2) At least one slot is free. An action which resets the slot and kicks
 * off its update is queued.
 */
StatusCode AvalancheSchedulerSvc::pushNewEvent( EventContext* eventContext ) {

  if ( m_first ) {
    m_first = false;
  }

  if ( !eventContext ) {
    fatal() << "Event context is nullptr" << endmsg;
    return StatusCode::FAILURE;
  }

  if ( m_freeSlots.load() == 0 ) {
    if ( msgLevel( MSG::DEBUG ) ) debug() << "A free processing slot could not be found." << endmsg;
    return StatusCode::FAILURE;
  }

  // no problem as push new event is only called from one thread (event loop manager)
  m_freeSlots--;

  auto action = [this, eventContext]() -> StatusCode {
    // Event processing slot forced to be the same as the wb slot
    const unsigned int thisSlotNum = eventContext->slot();
    EventSlot& thisSlot            = m_eventSlots[thisSlotNum];
    if ( !thisSlot.complete ) {
      fatal() << "The slot " << thisSlotNum << " is supposed to be a finished event but it's not" << endmsg;
      return StatusCode::FAILURE;
    }

    info() << "Executing event " << eventContext->evt() << " on slot " << thisSlotNum << endmsg;
    thisSlot.reset( eventContext );

    // promote to CR and DR the initial set of algorithms
    auto vis = concurrency::Supervisor( m_eventSlots[thisSlotNum] );
    m_efManager.touchReadyAlgorithms( vis );

    return this->updateStates( thisSlotNum );
  }; // end of lambda

  // Kick off the scheduling!
  if ( msgLevel( MSG::VERBOSE ) ) {
    verbose() << "Pushing the action to update the scheduler for slot " << eventContext->slot() << endmsg;
    verbose() << "Free slots available " << m_freeSlots.load() << endmsg;
  }
  m_actionsQueue.push( action );

  return StatusCode::SUCCESS;
}

//---------------------------------------------------------------------------
StatusCode AvalancheSchedulerSvc::pushNewEvents( std::vector<EventContext*>& eventContexts ) {
  StatusCode sc;
  for ( auto context : eventContexts ) {
    sc = pushNewEvent( context );
    if ( sc != StatusCode::SUCCESS ) return sc;
  }
  return sc;
}

//---------------------------------------------------------------------------
unsigned int AvalancheSchedulerSvc::freeSlots() {
  return std::max( m_freeSlots.load(), 0 );
}

//---------------------------------------------------------------------------
/**
 * Update the states for all slots until nothing is left to do.
*/
StatusCode AvalancheSchedulerSvc::m_drain() {

  unsigned int slotNum = 0;
  for ( auto& thisSlot : m_eventSlots ) {
    if ( not thisSlot.algsStates.allAlgsExecuted() and not thisSlot.complete ) {
      updateStates( slotNum );
    }
    slotNum++;
  }
  return StatusCode::SUCCESS;
}

//---------------------------------------------------------------------------
/**
* Get a finished event or block until one becomes available.
*/
StatusCode AvalancheSchedulerSvc::popFinishedEvent( EventContext*& eventContext ) {
  // debug() << "popFinishedEvent: queue size: " << m_finishedEvents.size() << endmsg;
  if ( m_freeSlots.load() == m_maxEventsInFlight or m_isActive == INACTIVE ) {
    // debug() << "freeslots: " << m_freeSlots << "/" << m_maxEventsInFlight
    //      << " active: " << m_isActive << endmsg;
    return StatusCode::FAILURE;
  } else {
    // debug() << "freeslots: " << m_freeSlots << "/" << m_maxEventsInFlight
    //      << " active: " << m_isActive << endmsg;
    m_finishedEvents.pop( eventContext );
    m_freeSlots++;
    if (msgLevel(MSG::DEBUG))
      debug() << "Popped slot " << eventContext->slot() << "(event "
              << eventContext->evt() << ")" << endmsg;
    return StatusCode::SUCCESS;
  }
}

//---------------------------------------------------------------------------
/**
* Try to get a finished event, if not available just return a failure
*/
StatusCode AvalancheSchedulerSvc::tryPopFinishedEvent( EventContext*& eventContext ) {
  if ( m_finishedEvents.try_pop( eventContext ) ) {
    if ( msgLevel( MSG::DEBUG ) )
      debug() << "Try Pop successful slot " << eventContext->slot() << "(event " << eventContext->evt() << ")"
              << endmsg;
    m_freeSlots++;
    return StatusCode::SUCCESS;
  }
  return StatusCode::FAILURE;
}

//---------------------------------------------------------------------------
/**
 * It can be possible that an event fails. In this case this method is called.
 * It dumps the state of the scheduler, drains the actions (without executing
 * them) and events in the queues and returns a failure.
*/
StatusCode AvalancheSchedulerSvc::eventFailed( EventContext* eventContext ) {

  // Set the number of slots available to an error code
  m_freeSlots.store( 0 );

  fatal() << "*** Event " << eventContext->evt() << " on slot "
          << eventContext->slot() << " failed! ***" << endmsg;

  std::ostringstream ost;
  m_algExecStateSvc->dump(ost, *eventContext);

  info() << "Dumping Alg Exec State for slot " << eventContext->slot()
         << ":\n" << ost.str() << endmsg;

  dumpSchedulerState(-1);

  // Empty queue and deactivate the service
  action thisAction;
  while ( m_actionsQueue.try_pop( thisAction ) ) {
  };
  deactivate();

  // Push into the finished events queue the failed context
  EventContext* thisEvtContext;
  while ( m_finishedEvents.try_pop( thisEvtContext ) ) {
    m_finishedEvents.push( thisEvtContext );
  };
  m_finishedEvents.push( eventContext );

  return StatusCode::FAILURE;
}

//===========================================================================

//===========================================================================
// States Management

/**
 * Update the state of the algorithms.
 * The oldest events are checked before the newest, in order to reduce the
 * event backlog.
 * To check if the event is finished the algorithm checks if:
 * * No algorithms have been signed off by the control flow
 * * No algorithms have been signed off by the data flow
 * * No algorithms have been scheduled
*/
StatusCode AvalancheSchedulerSvc::updateStates( int si, const std::string& algo_name ) {

  m_updateNeeded = true;

  StatusCode global_sc( StatusCode::FAILURE, true );

  // Sort from the oldest to the newest event
  // Prepare a vector of pointers to the slots to avoid copies
  std::vector<EventSlot*> eventSlotsPtrs;

  // Consider all slots if si <0 or just one otherwise
  if ( si < 0 ) {
    const int eventsSlotsSize( m_eventSlots.size() );
    eventSlotsPtrs.reserve( eventsSlotsSize );
    for ( auto slotIt = m_eventSlots.begin(); slotIt != m_eventSlots.end(); slotIt++ ) {
      if ( !slotIt->complete ) eventSlotsPtrs.push_back( &( *slotIt ) );
    }
    std::sort( eventSlotsPtrs.begin(), eventSlotsPtrs.end(),
               []( EventSlot* a, EventSlot* b ) { return a->eventContext->evt() < b->eventContext->evt(); } );
  } else {
    eventSlotsPtrs.push_back( &m_eventSlots[si] );
  }

  for ( EventSlot* thisSlotPtr : eventSlotsPtrs ) {
    int iSlot = thisSlotPtr->eventContext->slot();

    // Cache the states of the algos to improve readability and performance
    auto& thisSlot                      = m_eventSlots[iSlot];
    AlgsExecutionStates& thisAlgsStates = thisSlot.algsStates;

    // Take care of the control ready update
    if ( !algo_name.empty() )
      m_efManager.updateDecision( algo_name, iSlot, thisAlgsStates, thisSlot.controlFlowState );

    StatusCode partial_sc( StatusCode::FAILURE, true );
    // first update CONTROLREADY to DATAREADY

    // now update DATAREADY to SCHEDULED
    if ( !m_optimizationMode.empty() ) {
      auto comp_nodes = [this]( const uint& i, const uint& j ) {
        return ( m_efManager.getPrecedenceRulesGraph()->getAlgorithmNode( index2algname( i ) )->getRank() <
                 m_efManager.getPrecedenceRulesGraph()->getAlgorithmNode( index2algname( j ) )->getRank() );
      };
      std::priority_queue<uint, std::vector<uint>, std::function<bool( const uint&, const uint& )>> buffer(
          comp_nodes, std::vector<uint>() );
      for ( auto it = thisAlgsStates.begin( AlgsExecutionStates::State::DATAREADY );
            it != thisAlgsStates.end( AlgsExecutionStates::State::DATAREADY ); ++it )
        buffer.push( *it );
      /*std::stringstream s;
      auto buffer2 = buffer;
      while (!buffer2.empty()) {
        s << m_efManager.getPrecedenceRulesGraph()->getAlgorithmNode(index2algname(buffer2.top()))->getRank() << ", ";
        buffer2.pop();
      }
      info() << "DRBuffer is: [ " << s.str() << " ]  <--" << algo_name << " executed" << endmsg;*/

      /*while (!buffer.empty()) {
        partial_sc = promoteToScheduled(buffer.top(), iSlot);
        if (partial_sc.isFailure()) {
          if (msgLevel(MSG::VERBOSE))
            verbose() << "Could not apply transition from "
                      << AlgsExecutionStates::stateNames[AlgsExecutionStates::State::DATAREADY]
                      << " for algorithm " << index2algname(buffer.top()) << " on processing slot " << iSlot << endmsg;
          if (m_useIOBoundAlgScheduler) {
            partial_sc = promoteToAsyncScheduled(buffer.top(), iSlot);
            if (msgLevel(MSG::VERBOSE))
              if (partial_sc.isFailure())
                verbose() << "[Asynchronous] Could not apply transition from "
                          << AlgsExecutionStates::stateNames[AlgsExecutionStates::State::DATAREADY]
                          << " for algorithm " << index2algname(buffer.top()) << " on processing slot " << iSlot << endmsg;
          }
        }
        buffer.pop();
      }*/
      while ( !buffer.empty() ) {
        bool IOBound = false;
        if ( m_useIOBoundAlgScheduler )
          IOBound = m_efManager.getPrecedenceRulesGraph()->getAlgorithmNode( index2algname( buffer.top() ) )->isIOBound();

        if ( !IOBound )
          partial_sc = promoteToScheduled( buffer.top(), iSlot );
        else
          partial_sc = promoteToAsyncScheduled( buffer.top(), iSlot );

        if (msgLevel(MSG::VERBOSE))
          if (partial_sc.isFailure())
            verbose() << "Could not apply transition from "
                      << AlgsExecutionStates::stateNames[AlgsExecutionStates::State::DATAREADY]
                      << " for algorithm " << index2algname(buffer.top()) << " on processing slot " << iSlot << endmsg;

        buffer.pop();
      }

    } else {
      for ( auto it = thisAlgsStates.begin( AlgsExecutionStates::State::DATAREADY );
            it != thisAlgsStates.end( AlgsExecutionStates::State::DATAREADY ); ++it ) {
        uint algIndex = *it;

        bool IOBound = false;
        if ( m_useIOBoundAlgScheduler )
          IOBound = m_efManager.getPrecedenceRulesGraph()->getAlgorithmNode( index2algname( algIndex ) )->isIOBound();

        if ( !IOBound )
          partial_sc = promoteToScheduled( algIndex, iSlot );
        else
          partial_sc = promoteToAsyncScheduled( algIndex, iSlot );

        if (msgLevel(MSG::VERBOSE))
          if (partial_sc.isFailure())
            verbose() << "Could not apply transition from "
                      << AlgsExecutionStates::stateNames[AlgsExecutionStates::State::DATAREADY]
                      << " for algorithm " << index2algname(algIndex) << " on processing slot " << iSlot << endmsg;
      }
    }

    if (m_dumpIntraEventDynamics) {
      auto now = std::chrono::system_clock::now();
      std::stringstream s;
      s << algo_name << ", " << thisAlgsStates.sizeOfSubset(State::CONTROLREADY) << ", "
                     << thisAlgsStates.sizeOfSubset(State::DATAREADY) << ", "
                     << thisAlgsStates.sizeOfSubset(State::SCHEDULED) << ", "
                     << std::chrono::duration_cast<std::chrono::nanoseconds> (now - m_efManager.getPrecedenceRulesGraph()->getInitTime()).count()
                     << "\n";
      auto threads = (m_threadPoolSize != -1) ? std::to_string(m_threadPoolSize)
                                              : std::to_string(tbb::task_scheduler_init::default_num_threads());
      std::ofstream myfile;
      myfile.open( "IntraEventConcurrencyDynamics_" + threads + "T.csv", std::ios::app );
      myfile << s.str();
      myfile.close();
    }

    // Not complete because this would mean that the slot is already free!
    if ( !thisSlot.complete && m_efManager.rootDecisionResolved( thisSlot.controlFlowState ) &&
         !thisSlot.algsStates.algsPresent( AlgsExecutionStates::CONTROLREADY ) &&
         !thisSlot.algsStates.algsPresent( AlgsExecutionStates::DATAREADY ) &&
         !thisSlot.algsStates.algsPresent( AlgsExecutionStates::SCHEDULED ) ) {

      thisSlot.complete = true;
      // if the event did not fail, add it to the finished events
      // otherwise it is taken care of in the error handling already
      if(m_algExecStateSvc->eventStatus(*thisSlot.eventContext) == EventStatus::Success) {
        m_finishedEvents.push(thisSlot.eventContext);
        if (msgLevel(MSG::DEBUG))
          debug() << "Event " << thisSlot.eventContext->evt() << " finished (slot "
                  << thisSlot.eventContext->slot() << ")." << endmsg;
      }
      // now let's return the fully evaluated result of the control flow
      if ( msgLevel( MSG::DEBUG ) ) {
        std::stringstream ss;
        m_efManager.printEventState( ss, thisSlot.algsStates, thisSlot.controlFlowState, 0 );
        debug() << ss.str() << endmsg;
      }

      thisSlot.eventContext = nullptr;
    } else {
      StatusCode eventStalledSC = isStalled(iSlot);
      if (! eventStalledSC.isSuccess()) {
        m_algExecStateSvc->setEventStatus(EventStatus::AlgStall, *thisSlot.eventContext);
        eventFailed(thisSlot.eventContext).ignore();
      }
    }
  } // end loop on slots

  verbose() << "States Updated." << endmsg;

  return global_sc;
}

//---------------------------------------------------------------------------

/**
 * Check if we are in present of a stall condition for a particular slot.
 * This is the case when no actions are present in the actionsQueue,
 * no algorithm is in flight and no algorithm has all of its dependencies
 * satisfied.
*/
StatusCode AvalancheSchedulerSvc::isStalled( int iSlot ) {
  // Get the slot
  EventSlot& thisSlot = m_eventSlots[iSlot];

  if ( m_actionsQueue.empty() && m_algosInFlight == 0 && m_IOBoundAlgosInFlight == 0 &&
       ( !thisSlot.algsStates.algsPresent( AlgsExecutionStates::DATAREADY ) ) ) {

    info() << "About to declare a stall" << endmsg;
    fatal() << "*** Stall detected! ***\n" << endmsg;
    dumpSchedulerState( iSlot );
    // throw GaudiException ("Stall detected",name(),StatusCode::FAILURE);

    return StatusCode::FAILURE;
  }
  return StatusCode::SUCCESS;
}

//---------------------------------------------------------------------------

/**
 * Used for debugging purposes, the state of the scheduler is dumped on screen
 * in order to be inspected.
 * The dependencies of each algo are printed and the missing ones specified.
**/
void AvalancheSchedulerSvc::dumpSchedulerState( int iSlot ) {

  // To have just one big message
  std::ostringstream outputMessageStream;

  outputMessageStream << "============================== Execution Task State ============================="
                      << std::endl;
  dumpState( outputMessageStream );

  outputMessageStream << std::endl
                      << "============================== Scheduler State  ================================="
                      << std::endl;

  int slotCount = -1;
  for ( auto thisSlot : m_eventSlots ) {
    slotCount++;
    if ( thisSlot.complete ) continue;

    outputMessageStream << "-----------  slot: " << thisSlot.eventContext->slot()
                        << "  event: " << thisSlot.eventContext->evt() << " -----------" << std::endl;

    if ( 0 > iSlot or iSlot == slotCount ) {
      outputMessageStream << "Algorithms states:" << std::endl;

      const DataObjIDColl& wbSlotContent( thisSlot.dataFlowMgr.content() );
      for ( unsigned int algoIdx = 0; algoIdx < thisSlot.algsStates.size(); ++algoIdx ) {
        outputMessageStream << " o " << index2algname( algoIdx ) << " ["
                            << AlgsExecutionStates::stateNames[thisSlot.algsStates[algoIdx]] << "]  Data deps: ";
        DataObjIDColl deps( thisSlot.dataFlowMgr.dataDependencies( algoIdx ) );
        const int depsSize = deps.size();
        if ( depsSize == 0 ) outputMessageStream << " none";

        DataObjIDColl missing;
        for ( auto d : deps ) {
          outputMessageStream << d << " ";
          if ( wbSlotContent.find( d ) == wbSlotContent.end() ) {
            //      outputMessageStream << "[missing] ";
            missing.insert( d );
          }
        }

        if ( !missing.empty() ) {
          outputMessageStream << ". The following are missing: ";
          for ( auto d : missing ) {
            outputMessageStream << d << " ";
          }
        }

        outputMessageStream << std::endl;
      }

      // Snapshot of the WhiteBoard
      outputMessageStream << "\nWhiteboard contents: " << std::endl;
      for ( auto& product : wbSlotContent ) outputMessageStream << " o " << product << std::endl;

      // Snapshot of the ControlFlow
      outputMessageStream << "\nControl Flow:" << std::endl;
      std::stringstream cFlowStateStringStream;
      m_efManager.printEventState( cFlowStateStringStream, thisSlot.algsStates, thisSlot.controlFlowState, 0 );

      outputMessageStream << cFlowStateStringStream.str() << std::endl;
    }
  }

  outputMessageStream << "=================================== END ======================================" << std::endl;

  info() << "Dumping Scheduler State " << std::endl << outputMessageStream.str() << endmsg;
}

//---------------------------------------------------------------------------

StatusCode AvalancheSchedulerSvc::promoteToScheduled( unsigned int iAlgo, int si ) {

  if ( m_algosInFlight == m_maxAlgosInFlight ) return StatusCode::FAILURE;

  const std::string& algName( index2algname( iAlgo ) );
  IAlgorithm* ialgoPtr = nullptr;
  StatusCode sc( m_algResourcePool->acquireAlgorithm( algName, ialgoPtr ) );

  if ( sc.isSuccess() ) { // if we managed to get an algorithm instance try to schedule it
    EventContext* eventContext( m_eventSlots[si].eventContext );
    if ( !eventContext )
      fatal() << "Event context for algorithm " << algName << " is a nullptr (slot " << si << ")" << endmsg;

    ++m_algosInFlight;
    auto promote2ExecutedClosure = std::bind(&AvalancheSchedulerSvc::promoteToExecuted,
                                             this,
                                             iAlgo,
                                             eventContext->slot(),
                                             ialgoPtr,
                                             eventContext);
    // Avoid to use tbb if the pool size is 1 and run in this thread
    if (-100 != m_threadPoolSize) {

      // this parent task is needed to promote an Algorithm as EXECUTED,
      // it will be started as soon as the child task (see below) is completed
      tbb::task* triggerAlgoStateUpdate = new(tbb::task::allocate_root())
                                enqueueSchedulerActionTask(this, promote2ExecutedClosure);
      // setting parent's refcount to 1 is made here only for consistency
      // (in this case since it is not scheduled explicitly and there it has only one child task)
      triggerAlgoStateUpdate->set_ref_count(1);
      // the child task that executes an Algorithm
      tbb::task* algoTask = new(triggerAlgoStateUpdate->allocate_child())
                            AlgoExecutionTask(ialgoPtr, iAlgo, eventContext, serviceLocator(), m_algExecStateSvc);
      // schedule the algoTask
      tbb::task::enqueue( *algoTask);

    } else {
      AlgoExecutionTask theTask(ialgoPtr, iAlgo, eventContext, serviceLocator(), m_algExecStateSvc);
      theTask.execute();
      promote2ExecutedClosure();
    }

    if ( msgLevel( MSG::DEBUG ) )
      debug() << "Algorithm " << algName << " was submitted on event " << eventContext->evt() << " in slot " << si
              << ". Algorithms scheduled are " << m_algosInFlight << endmsg;

    StatusCode updateSc( m_eventSlots[si].algsStates.updateState( iAlgo, AlgsExecutionStates::SCHEDULED ) );

    if ( msgLevel( MSG::VERBOSE ) ) dumpSchedulerState( -1 );

    if (updateSc.isSuccess())
      if (msgLevel(MSG::VERBOSE))
        verbose() << "Promoting " << index2algname(iAlgo) << " to SCHEDULED on slot "
                  << si << endmsg;
    return updateSc;
  } else {
    if ( msgLevel( MSG::DEBUG ) )
      debug() << "Could not acquire instance for algorithm " << index2algname( iAlgo ) << " on slot " << si << endmsg;
    return sc;
  }
}

//---------------------------------------------------------------------------

StatusCode AvalancheSchedulerSvc::promoteToAsyncScheduled( unsigned int iAlgo, int si ) {

  if ( m_IOBoundAlgosInFlight == m_maxIOBoundAlgosInFlight ) return StatusCode::FAILURE;

  // bool IOBound = m_efManager.getPrecedenceRulesGraph()->getAlgorithmNode(algName)->isIOBound();

  const std::string& algName( index2algname( iAlgo ) );
  IAlgorithm* ialgoPtr = nullptr;
  StatusCode sc( m_algResourcePool->acquireAlgorithm( algName, ialgoPtr ) );

  if ( sc.isSuccess() ) { // if we managed to get an algorithm instance try to schedule it
    EventContext* eventContext( m_eventSlots[si].eventContext );
    if ( !eventContext )
      fatal() << "[Asynchronous] Event context for algorithm " << algName << " is a nullptr (slot " << si << ")"
              << endmsg;

    ++m_IOBoundAlgosInFlight;
    // Can we use tbb-based overloaded new-operator for a "custom" task (an algorithm wrapper, not derived from tbb::task)? it seems it works..
    IOBoundAlgTask* theTask = new( tbb::task::allocate_root() )
      IOBoundAlgTask(ialgoPtr, iAlgo, eventContext, serviceLocator(), m_algExecStateSvc);
    m_IOBoundAlgScheduler->push(*theTask);

    if (msgLevel(MSG::DEBUG))
      debug() << "[Asynchronous] Algorithm " << algName << " was submitted on event "
              << eventContext->evt() << " in slot " << si
              << ". algorithms scheduled are " << m_IOBoundAlgosInFlight << endmsg;

    StatusCode updateSc( m_eventSlots[si].algsStates.updateState( iAlgo, AlgsExecutionStates::SCHEDULED ) );

    if (updateSc.isSuccess())
      if (msgLevel(MSG::VERBOSE))
        verbose() << "[Asynchronous] Promoting " << index2algname(iAlgo)
                  << " to SCHEDULED on slot " << si << endmsg;
    return updateSc;
  } else {
    if ( msgLevel( MSG::DEBUG ) )
      debug() << "[Asynchronous] Could not acquire instance for algorithm " << index2algname( iAlgo ) << " on slot "
              << si << endmsg;
    return sc;
  }
}

//---------------------------------------------------------------------------
/**
 * The call to this method is triggered only from within the AlgoExecutionTask.
*/
StatusCode AvalancheSchedulerSvc::promoteToExecuted( unsigned int iAlgo, int si, IAlgorithm* algo,
                                                    EventContext* eventContext ) {
  // Put back the instance
  Algorithm* castedAlgo = dynamic_cast<Algorithm*>( algo ); // DP: expose context getter in IAlgo?
  if ( !castedAlgo ) fatal() << "The casting did not succeed!" << endmsg;
  //  EventContext* eventContext = castedAlgo->getContext();

  // Check if the execution failed
  if (m_algExecStateSvc->eventStatus(*eventContext) != EventStatus::Success)
    eventFailed(eventContext).ignore();

  Gaudi::Hive::setCurrentContext(eventContext);
  StatusCode sc = m_algResourcePool->releaseAlgorithm( algo->name(), algo );

  if ( !sc.isSuccess() ) {
    error() << "[Event " << eventContext->evt() << ", Slot " << eventContext->slot() << "] "
            << "Instance of algorithm " << algo->name() << " could not be properly put back." << endmsg;
    return StatusCode::FAILURE;
  }

  m_algosInFlight--;

  EventSlot& thisSlot = m_eventSlots[si];

  if ( msgLevel( MSG::DEBUG ) )
    debug() << "Algorithm " << algo->name() << " executed in slot " << si << ". Algorithms scheduled are "
            << m_algosInFlight << endmsg;

  // Schedule an update of the status of the algorithms
  auto updateAction = std::bind( &AvalancheSchedulerSvc::updateStates, this, -1, algo->name() );
  m_actionsQueue.push( updateAction );
  m_updateNeeded = false;

  if ( msgLevel( MSG::DEBUG ) )
    debug() << "Trying to handle execution result of " << index2algname( iAlgo ) << " on slot " << si << endmsg;
  State state;
  if ( algo->filterPassed() ) {
    state = State::EVTACCEPTED;
  } else {
    state = State::EVTREJECTED;
  }

  sc = thisSlot.algsStates.updateState( iAlgo, state );

  if (sc.isSuccess())
    if (msgLevel(MSG::VERBOSE))
      verbose() << "Promoting " << index2algname(iAlgo) << " on slot " << si << " to "
                << AlgsExecutionStates::stateNames[state] << endmsg;

  return sc;
}

//---------------------------------------------------------------------------
/**
 * The call to this method is triggered only from within the IOBoundAlgTask.
*/
StatusCode AvalancheSchedulerSvc::promoteToAsyncExecuted( unsigned int iAlgo, int si, IAlgorithm* algo,
                                                         EventContext* eventContext ) {
  // Put back the instance
  Algorithm* castedAlgo = dynamic_cast<Algorithm*>( algo ); // DP: expose context getter in IAlgo?
  if ( !castedAlgo ) fatal() << "[Asynchronous] The casting did not succeed!" << endmsg;
  // EventContext* eventContext = castedAlgo->getContext();

  // Check if the execution failed
  if (m_algExecStateSvc->eventStatus(*eventContext) != EventStatus::Success)
    eventFailed(eventContext).ignore();

  StatusCode sc = m_algResourcePool->releaseAlgorithm( algo->name(), algo );

  if ( !sc.isSuccess() ) {
    error() << "[Asynchronous]  [Event " << eventContext->evt() << ", Slot " << eventContext->slot() << "] "
            << "Instance of algorithm " << algo->name() << " could not be properly put back." << endmsg;
    return StatusCode::FAILURE;
  }

  m_IOBoundAlgosInFlight--;

  EventSlot& thisSlot = m_eventSlots[si];

  if (msgLevel(MSG::DEBUG))
    debug() << "[Asynchronous] Algorithm " << algo->name() << " executed in slot " << si
            << ". Algorithms scheduled are " << m_IOBoundAlgosInFlight << endmsg;

  // Schedule an update of the status of the algorithms
  auto updateAction = std::bind( &AvalancheSchedulerSvc::updateStates, this, -1, algo->name() );
  m_actionsQueue.push( updateAction );
  m_updateNeeded = false;

  if (msgLevel(MSG::DEBUG))
    debug() << "[Asynchronous] Trying to handle execution result of "
            << index2algname(iAlgo) << " on slot " << si << endmsg;
  State state;
  if ( algo->filterPassed() ) {
    state = State::EVTACCEPTED;
  } else {
    state = State::EVTREJECTED;
  }

  sc = thisSlot.algsStates.updateState( iAlgo, state );

  if (sc.isSuccess())
    if (msgLevel(MSG::VERBOSE))
      verbose() << "[Asynchronous] Promoting " << index2algname(iAlgo) << " on slot "
                << si << " to " << AlgsExecutionStates::stateNames[state] << endmsg;

  return sc;
}

//===========================================================================
void AvalancheSchedulerSvc::addAlg( Algorithm* a, EventContext* e, pthread_t t )
{

  std::lock_guard<std::mutex> lock( m_ssMut );
  m_sState.push_back( SchedulerState( a, e, t ) );
}

//===========================================================================
bool AvalancheSchedulerSvc::delAlg( Algorithm* a )
{

  std::lock_guard<std::mutex> lock( m_ssMut );

  for ( std::list<SchedulerState>::iterator itr = m_sState.begin(); itr != m_sState.end(); ++itr ) {
    if ( *itr == a ) {
      m_sState.erase( itr );
      return true;
    }
  }

  error() << "could not find Alg " << a->name() << " in Scheduler!" << endmsg;
  return false;
}

//===========================================================================
void AvalancheSchedulerSvc::dumpState( std::ostringstream& ost )
{

  std::lock_guard<std::mutex> lock( m_ssMut );

  for ( auto it : m_sState ) {
    ost << "  " << it << std::endl;
  }
}

//===========================================================================
void AvalancheSchedulerSvc::dumpState()
{

  std::lock_guard<std::mutex> lock( m_ssMut );

  std::ostringstream ost;
  ost << "dumping Executing Threads: [" << m_sState.size() << "]" << std::endl;
  dumpState( ost );

  info() << ost.str() << endmsg;
}