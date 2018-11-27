#include "AvalancheSchedulerSvc.h"

#include "AlgoExecutionTask.h"
#include "IOBoundAlgTask.h"

// Framework includes
#include "GaudiKernel/Algorithm.h" // can be removed ASA dynamic casts to Algorithm are removed
#include "GaudiKernel/ConcurrencyFlags.h"
#include "GaudiKernel/DataHandleHolderVisitor.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/IDataManagerSvc.h"
#include "GaudiKernel/ThreadLocalContext.h"

// C++
#include <algorithm>
#include <map>
#include <queue>
#include <sstream>
#include <unordered_set>

// External libs
#include "boost/algorithm/string.hpp"
#include "boost/thread.hpp"
#include "boost/tokenizer.hpp"
// DP waiting for the TBB service
#include "tbb/task_scheduler_init.h"

// Instantiation of a static factory class used by clients to create instances of this service
DECLARE_COMPONENT( AvalancheSchedulerSvc )

#define ON_DEBUG if ( msgLevel( MSG::DEBUG ) )
#define ON_VERBOSE if ( msgLevel( MSG::VERBOSE ) )

namespace
{
  struct DataObjIDSorter {
    bool operator()( const DataObjID* a, const DataObjID* b ) { return a->fullKey() < b->fullKey(); }
  };

  // Sort a DataObjIDColl in a well-defined, reproducible manner.
  // Used for making debugging dumps.
  std::vector<const DataObjID*> sortedDataObjIDColl( const DataObjIDColl& coll )
  {
    std::vector<const DataObjID*> v;
    v.reserve( coll.size() );
    for ( const DataObjID& id : coll ) v.push_back( &id );
    std::sort( v.begin(), v.end(), DataObjIDSorter() );
    return v;
  }

  bool subSlotAlgsInStates( const EventSlot& slot, std::initializer_list<AlgsExecutionStates::State> testStates )
  {
    return std::any_of( slot.allSubSlots.begin(), slot.allSubSlots.end(),
                        [testStates]( const EventSlot& ss ) { return ss.algsStates.containsAny( testStates ); } );
  }
}

//---------------------------------------------------------------------------

/**
 * Here, among some "bureaucracy" operations, the scheduler is activated,
 * executing the activate() function in a new thread.
 * In addition the algorithms list is acquired from the algResourcePool.
**/
StatusCode AvalancheSchedulerSvc::initialize()
{

  // Initialise mother class (read properties, ...)
  StatusCode sc( Service::initialize() );
  if ( sc.isFailure() ) warning() << "Base class could not be initialized" << endmsg;

  // Get hold of the TBBSvc. This should initialize the thread pool
  m_threadPoolSvc = serviceLocator()->service( "ThreadPoolSvc" );
  if ( !m_threadPoolSvc.isValid() ) {
    fatal() << "Error retrieving ThreadPoolSvc" << endmsg;
    return StatusCode::FAILURE;
  }

  // Activate the scheduler in another thread.
  info() << "Activating scheduler in a separate thread" << endmsg;
  m_thread = std::thread( [this]() { this->activate(); } );

  while ( m_isActive != ACTIVE ) {
    if ( m_isActive == FAILURE ) {
      fatal() << "Terminating initialization" << endmsg;
      return StatusCode::FAILURE;
    } else {
      ON_DEBUG debug() << "Waiting for AvalancheSchedulerSvc to activate" << endmsg;
      sleep( 1 );
    }
  }

  if ( m_enableCondSvc ) {
    // Get hold of the CondSvc
    m_condSvc = serviceLocator()->service( "CondSvc" );
    if ( !m_condSvc.isValid() ) {
      warning() << "No CondSvc found, or not enabled. "
                << "Will not manage CondAlgorithms" << endmsg;
      m_enableCondSvc = false;
    }
  }

  // Get the algo resource pool
  m_algResourcePool = serviceLocator()->service( "AlgResourcePool" );
  if ( !m_algResourcePool.isValid() ) {
    fatal() << "Error retrieving AlgoResourcePool" << endmsg;
    return StatusCode::FAILURE;
  }

  m_algExecStateSvc = serviceLocator()->service( "AlgExecStateSvc" );
  if ( !m_algExecStateSvc.isValid() ) {
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

  // Set the MaxEventsInFlight parameters from the number of WB stores
  m_maxEventsInFlight = m_whiteboard->getNumberOfStores();

  // Set the number of free slots
  m_freeSlots = m_maxEventsInFlight;

  // Get the list of algorithms
  const std::list<IAlgorithm*>& algos      = m_algResourcePool->getFlatAlgList();
  const unsigned int            algsNumber = algos.size();
  info() << "Found " << algsNumber << " algorithms" << endmsg;

  /* Dependencies
   1) Look for handles in algo, if none
   2) Assume none are required
  */

  DataObjIDColl globalInp, globalOutp;

  // figure out all outputs
  for ( IAlgorithm* ialgoPtr : algos ) {
    Algorithm* algoPtr = dynamic_cast<Algorithm*>( ialgoPtr );
    if ( !algoPtr ) {
      fatal() << "Could not convert IAlgorithm into Algorithm: this will result in a crash." << endmsg;
      return StatusCode::FAILURE;
    }
    for ( auto id : algoPtr->outputDataObjs() ) {
      auto r = globalOutp.insert( id );
      if ( !r.second ) {
        warning() << "multiple algorithms declare " << id << " as output! could be a single instance in multiple paths "
                                                             "though, or control flow may guarantee only one runs...!"
                  << endmsg;
      }
    }
  }

  std::ostringstream ostdd;
  ostdd << "Data Dependencies for Algorithms:";

  std::map<std::string, DataObjIDColl> algosDependenciesMap;
  for ( IAlgorithm* ialgoPtr : algos ) {
    Algorithm* algoPtr = dynamic_cast<Algorithm*>( ialgoPtr );
    if ( nullptr == algoPtr ) {
      fatal() << "Could not convert IAlgorithm into Algorithm for " << ialgoPtr->name()
              << ": this will result in a crash." << endmsg;
      return StatusCode::FAILURE;
    }

    ostdd << "\n  " << algoPtr->name();

    DataObjIDColl algoDependencies;
    if ( !algoPtr->inputDataObjs().empty() || !algoPtr->outputDataObjs().empty() ) {
      for ( const DataObjID* idp : sortedDataObjIDColl( algoPtr->inputDataObjs() ) ) {
        DataObjID id = *idp;
        ostdd << "\n    o INPUT  " << id;
        if ( id.key().find( ":" ) != std::string::npos ) {
          ostdd << " contains alternatives which require resolution...\n";
          auto tokens = boost::tokenizer<boost::char_separator<char>>{id.key(), boost::char_separator<char>{":"}};
          auto itok   = std::find_if( tokens.begin(), tokens.end(), [&]( const std::string& t ) {
            return globalOutp.find( DataObjID{t} ) != globalOutp.end();
          } );
          if ( itok != tokens.end() ) {
            ostdd << "found matching output for " << *itok << " -- updating scheduler info\n";
            id.updateKey( *itok );
          } else {
            error() << "failed to find alternate in global output list"
                    << " for id: " << id << " in Alg " << algoPtr->name() << endmsg;
            m_showDataDeps = true;
          }
        }
        algoDependencies.insert( id );
        globalInp.insert( id );
      }
      for ( const DataObjID* id : sortedDataObjIDColl( algoPtr->outputDataObjs() ) ) {
        ostdd << "\n    o OUTPUT " << *id;
        if ( id->key().find( ":" ) != std::string::npos ) {
          error() << " in Alg " << algoPtr->name() << " alternatives are NOT allowed for outputs! id: " << *id
                  << endmsg;
          m_showDataDeps = true;
        }
      }
    } else {
      ostdd << "\n      none";
    }
    algosDependenciesMap[algoPtr->name()] = algoDependencies;
  }

  if ( m_showDataDeps ) {
    info() << ostdd.str() << endmsg;
  }

  // Check if we have unmet global input dependencies, and, optionally, heal them
  // WARNING: this step must be done BEFORE the Precedence Service is initialized
  if ( m_checkDeps ) {
    DataObjIDColl unmetDep;
    for ( auto o : globalInp )
      if ( globalOutp.find( o ) == globalOutp.end() ) unmetDep.insert( o );

    if ( unmetDep.size() > 0 ) {

      std::ostringstream ost;
      for ( const DataObjID* o : sortedDataObjIDColl( unmetDep ) ) {
        ost << "\n   o " << *o << "    required by Algorithm: ";

        for ( const auto& p : algosDependenciesMap )
          if ( p.second.find( *o ) != p.second.end() ) ost << "\n       * " << p.first;
      }

      if ( !m_useDataLoader.empty() ) {

        // Find the DataLoader Alg
        IAlgorithm* dataLoaderAlg( nullptr );
        for ( IAlgorithm* algo : algos )
          if ( algo->name() == m_useDataLoader ) {
            dataLoaderAlg = algo;
            break;
          }

        if ( dataLoaderAlg == nullptr ) {
          fatal() << "No DataLoader Algorithm \"" << m_useDataLoader.value()
                  << "\" found, and unmet INPUT dependencies "
                  << "detected:\n"
                  << ost.str() << endmsg;
          return StatusCode::FAILURE;
        }

        info() << "Will attribute the following unmet INPUT dependencies to \"" << dataLoaderAlg->type() << "/"
               << dataLoaderAlg->name() << "\" Algorithm" << ost.str() << endmsg;

        // Set the property Load of DataLoader Alg
        Algorithm* dataAlg = dynamic_cast<Algorithm*>( dataLoaderAlg );
        if ( !dataAlg ) {
          fatal() << "Unable to dcast DataLoader \"" << m_useDataLoader.value() << "\" IAlg to Algorithm" << endmsg;
          return StatusCode::FAILURE;
        }

        for ( auto& id : unmetDep ) {
          ON_DEBUG debug() << "adding OUTPUT dep \"" << id << "\" to " << dataLoaderAlg->type() << "/"
                           << dataLoaderAlg->name() << endmsg;
          dataAlg->addDependency( id, Gaudi::DataHandle::Writer );
        }

      } else {
        fatal() << "Auto DataLoading not requested, "
                << "and the following unmet INPUT dependencies were found:" << ost.str() << endmsg;
        return StatusCode::FAILURE;
      }

    } else {
      info() << "No unmet INPUT data dependencies were found" << endmsg;
    }
  }

  // Get the precedence service
  m_precSvc = serviceLocator()->service( "PrecedenceSvc" );
  if ( !m_precSvc.isValid() ) {
    fatal() << "Error retrieving PrecedenceSvc" << endmsg;
    return StatusCode::FAILURE;
  }
  const PrecedenceSvc* precSvc = dynamic_cast<const PrecedenceSvc*>( m_precSvc.get() );
  if ( !precSvc ) {
    fatal() << "Unable to dcast PrecedenceSvc" << endmsg;
    return StatusCode::FAILURE;
  }

  // Fill the containers to convert algo names to index
  m_algname_vect.resize( algsNumber );
  for ( IAlgorithm* algo : algos ) {
    const std::string& name    = algo->name();
    auto               index   = precSvc->getRules()->getAlgorithmNode( name )->getAlgoIndex();
    m_algname_index_map[name]  = index;
    m_algname_vect.at( index ) = name;
  }

  // Shortcut for the message service
  SmartIF<IMessageSvc> messageSvc( serviceLocator() );
  if ( !messageSvc.isValid() ) error() << "Error retrieving MessageSvc interface IMessageSvc." << endmsg;

  m_eventSlots.reserve( m_maxEventsInFlight );
  for ( size_t i = 0; i < m_maxEventsInFlight; ++i ) {
    m_eventSlots.emplace_back( algsNumber, precSvc->getRules()->getControlFlowNodeCounter(), messageSvc );
    m_eventSlots.back().complete = true;
  }
  m_actionsCounts.assign( m_maxEventsInFlight, 0 );

  if ( m_threadPoolSize > 1 ) {
    m_maxAlgosInFlight = (size_t)m_threadPoolSize;
  }

  // Clearly inform about the level of concurrency
  info() << "Concurrency level information:" << endmsg;
  info() << " o Number of events in flight: " << m_maxEventsInFlight << endmsg;
  info() << " o TBB thread pool size: " << m_threadPoolSize << endmsg;

  if ( m_showControlFlow ) m_precSvc->dumpControlFlow();

  if ( m_showDataFlow ) m_precSvc->dumpDataFlow();

  // Simulate execution flow
  if ( m_simulateExecution ) m_precSvc->simulate( m_eventSlots[0] );

  return sc;
}
//---------------------------------------------------------------------------

/**
 * Here the scheduler is deactivated and the thread joined.
**/
StatusCode AvalancheSchedulerSvc::finalize()
{

  StatusCode sc( Service::finalize() );
  if ( sc.isFailure() ) warning() << "Base class could not be finalized" << endmsg;

  sc = deactivate();
  if ( sc.isFailure() ) warning() << "Scheduler could not be deactivated" << endmsg;

  info() << "Joining Scheduler thread" << endmsg;
  m_thread.join();

  // Final error check after thread pool termination
  if ( m_isActive == FAILURE ) {
    error() << "problems in scheduler thread" << endmsg;
    return StatusCode::FAILURE;
  }

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
void AvalancheSchedulerSvc::activate()
{

  ON_DEBUG debug() << "AvalancheSchedulerSvc::activate()" << endmsg;

  if ( m_threadPoolSvc->initPool( m_threadPoolSize ).isFailure() ) {
    error() << "problems initializing ThreadPoolSvc" << endmsg;
    m_isActive = FAILURE;
    return;
  }

  // Wait for actions pushed into the queue by finishing tasks.
  action     thisAction;
  StatusCode sc( StatusCode::SUCCESS );

  m_isActive = ACTIVE;

  // Continue to wait if the scheduler is running or there is something to do
  ON_DEBUG debug() << "Start checking the actionsQueue" << endmsg;
  while ( m_isActive == ACTIVE || m_actionsQueue.size() != 0 ) {
    m_actionsQueue.pop( thisAction );
    sc = thisAction();
    ON_VERBOSE
    {
      if ( sc != StatusCode::SUCCESS )
        verbose() << "Action did not succeed (which is not bad per se)." << endmsg;
      else
        verbose() << "Action succeeded." << endmsg;
    }
  }

  ON_DEBUG debug() << "Terminating thread-pool resources" << endmsg;
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
StatusCode AvalancheSchedulerSvc::deactivate()
{

  if ( m_isActive == ACTIVE ) {

    // Set the number of slots available to an error code
    m_freeSlots.store( 0 );

    // Empty queue
    action thisAction;
    while ( m_actionsQueue.try_pop( thisAction ) ) {
    };

    // This would be the last action
    m_actionsQueue.push( [this]() -> StatusCode {
      ON_VERBOSE verbose() << "Deactivating scheduler" << endmsg;
      m_isActive = INACTIVE;
      return StatusCode::SUCCESS;
    } );
  }

  return StatusCode::SUCCESS;
}

//---------------------------------------------------------------------------

// Utils and shortcuts

inline const std::string& AvalancheSchedulerSvc::index2algname( unsigned int index ) { return m_algname_vect[index]; }

//---------------------------------------------------------------------------

inline unsigned int AvalancheSchedulerSvc::algname2index( const std::string& algoname )
{
  unsigned int index = m_algname_index_map[algoname];
  return index;
}

//---------------------------------------------------------------------------

// EventSlot management
/**
 * Add event to the scheduler. There are two cases possible:
 *  1) No slot is free. A StatusCode::FAILURE is returned.
 *  2) At least one slot is free. An action which resets the slot and kicks
 * off its update is queued.
 */
StatusCode AvalancheSchedulerSvc::pushNewEvent( EventContext* eventContext )
{

  if ( !eventContext ) {
    fatal() << "Event context is nullptr" << endmsg;
    return StatusCode::FAILURE;
  }

  if ( m_freeSlots.load() == 0 ) {
    ON_DEBUG debug() << "A free processing slot could not be found." << endmsg;
    return StatusCode::FAILURE;
  }

  // no problem as push new event is only called from one thread (event loop manager)
  --m_freeSlots;

  auto action = [this, eventContext]() -> StatusCode {
    // Event processing slot forced to be the same as the wb slot
    const unsigned int thisSlotNum = eventContext->slot();
    EventSlot&         thisSlot    = m_eventSlots[thisSlotNum];
    if ( !thisSlot.complete ) {
      fatal() << "The slot " << thisSlotNum << " is supposed to be a finished event but it's not" << endmsg;
      return StatusCode::FAILURE;
    }

    ON_DEBUG debug() << "Executing event " << eventContext->evt() << " on slot " << thisSlotNum << endmsg;
    thisSlot.reset( eventContext );

    // Result status code:
    StatusCode result = StatusCode::SUCCESS;

    // promote to CR and DR the initial set of algorithms
    Cause cs = {Cause::source::Root, "RootDecisionHub"};
    if ( m_precSvc->iterate( thisSlot, cs ).isFailure() ) {
      error() << "Failed to call IPrecedenceSvc::iterate for slot " << thisSlotNum << endmsg;
      result = StatusCode::FAILURE;
    }

    if ( this->updateStates( thisSlotNum ).isFailure() ) {
      error() << "Failed to call AvalancheSchedulerSvc::updateStates for slot " << thisSlotNum << endmsg;
      result = StatusCode::FAILURE;
    }

    return result;
  }; // end of lambda

  // Kick off the scheduling!
  ON_VERBOSE
  {
    verbose() << "Pushing the action to update the scheduler for slot " << eventContext->slot() << endmsg;
    verbose() << "Free slots available " << m_freeSlots.load() << endmsg;
  }

  m_actionsQueue.push( action );

  return StatusCode::SUCCESS;
}

//---------------------------------------------------------------------------

StatusCode AvalancheSchedulerSvc::pushNewEvents( std::vector<EventContext*>& eventContexts )
{
  StatusCode sc;
  for ( auto context : eventContexts ) {
    sc = pushNewEvent( context );
    if ( sc != StatusCode::SUCCESS ) return sc;
  }
  return sc;
}

//---------------------------------------------------------------------------

unsigned int AvalancheSchedulerSvc::freeSlots() { return std::max( m_freeSlots.load(), 0 ); }

//---------------------------------------------------------------------------
/**
* Get a finished event or block until one becomes available.
*/
StatusCode AvalancheSchedulerSvc::popFinishedEvent( EventContext*& eventContext )
{
  // ON_DEBUG debug() << "popFinishedEvent: queue size: " << m_finishedEvents.size() << endmsg;
  if ( m_freeSlots.load() == (int)m_maxEventsInFlight || m_isActive == INACTIVE ) {
    // ON_DEBUG debug() << "freeslots: " << m_freeSlots << "/" << m_maxEventsInFlight
    //      << " active: " << m_isActive << endmsg;
    return StatusCode::FAILURE;
  } else {
    // ON_DEBUG debug() << "freeslots: " << m_freeSlots << "/" << m_maxEventsInFlight
    //      << " active: " << m_isActive << endmsg;
    m_finishedEvents.pop( eventContext );
    ++m_freeSlots;
    ON_DEBUG debug() << "Popped slot " << eventContext->slot() << " (event " << eventContext->evt() << ")" << endmsg;
    return StatusCode::SUCCESS;
  }
}

//---------------------------------------------------------------------------
/**
* Try to get a finished event, if not available just return a failure
*/
StatusCode AvalancheSchedulerSvc::tryPopFinishedEvent( EventContext*& eventContext )
{
  if ( m_finishedEvents.try_pop( eventContext ) ) {
    ON_DEBUG debug() << "Try Pop successful slot " << eventContext->slot() << "(event " << eventContext->evt() << ")"
                     << endmsg;
    ++m_freeSlots;
    return StatusCode::SUCCESS;
  }
  return StatusCode::FAILURE;
}

//--------------------------------------------------------------------------
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
StatusCode AvalancheSchedulerSvc::updateStates( int si, const int algo_index, const int sub_slot,
                                                const int source_slot )
{

  StatusCode global_sc( StatusCode::SUCCESS );

  // Sort from the oldest to the newest event
  // Prepare a vector of pointers to the slots to avoid copies
  std::vector<EventSlot*> eventSlotsPtrs;

  // Consider all slots if si <0 or just one otherwise
  if ( si < 0 ) {
    const int eventsSlotsSize( m_eventSlots.size() );
    eventSlotsPtrs.reserve( eventsSlotsSize );
    for ( auto slotIt = m_eventSlots.begin(); slotIt != m_eventSlots.end(); ++slotIt ) {
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
    auto&                thisSlot       = m_eventSlots[iSlot];
    AlgsExecutionStates& thisAlgsStates = thisSlot.algsStates;

    // Perform the I->CR->DR transitions
    if ( algo_index >= 0 ) {
      Cause cs = {Cause::source::Task, index2algname( algo_index )};

      // Run in whole-event context if there's no sub-slot index, or the sub-slot has a different parent
      if ( sub_slot == -1 || iSlot != source_slot ) {
        if ( m_precSvc->iterate( thisSlot, cs ).isFailure() ) {
          error() << "Failed to call IPrecedenceSvc::iterate for slot " << iSlot << endmsg;
          global_sc = StatusCode::FAILURE;
        }
      } else {
        if ( m_precSvc->iterate( thisSlot.allSubSlots[sub_slot], cs ).isFailure() ) {
          error() << "Failed to call IPrecedenceSvc::iterate for sub-slot " << sub_slot << " of " << iSlot << endmsg;
          global_sc = StatusCode::FAILURE;
        }
      }
    }

    StatusCode partial_sc( StatusCode::FAILURE, true );

    // Perform DR->SCHEDULED
    if ( !m_optimizationMode.empty() ) {
      auto comp_nodes = [this]( const uint& i, const uint& j ) {
        return ( m_precSvc->getPriority( index2algname( i ) ) < m_precSvc->getPriority( index2algname( j ) ) );
      };
      std::priority_queue<uint, std::vector<uint>, std::function<bool( const uint&, const uint& )>> buffer(
          comp_nodes, std::vector<uint>() );
      for ( auto it = thisAlgsStates.begin( AState::DATAREADY ); it != thisAlgsStates.end( AState::DATAREADY ); ++it )
        buffer.push( *it );
      while ( !buffer.empty() ) {
        bool IOBound                            = false;
        if ( m_useIOBoundAlgScheduler ) IOBound = m_precSvc->isBlocking( index2algname( buffer.top() ) );

        if ( !IOBound )
          partial_sc = promoteToScheduled( buffer.top(), iSlot, thisSlotPtr->eventContext.get() );
        else
          partial_sc = promoteToAsyncScheduled( buffer.top(), iSlot, thisSlotPtr->eventContext.get() );

        ON_VERBOSE if ( partial_sc.isFailure() ) verbose() << "Could not apply transition from " << AState::DATAREADY
                                                           << " for algorithm " << index2algname( buffer.top() )
                                                           << " on processing slot " << iSlot << endmsg;

        buffer.pop();
      }

    } else {
      for ( auto it = thisAlgsStates.begin( AState::DATAREADY ); it != thisAlgsStates.end( AState::DATAREADY ); ++it ) {
        uint algIndex = *it;

        bool IOBound                            = false;
        if ( m_useIOBoundAlgScheduler ) IOBound = m_precSvc->isBlocking( index2algname( algIndex ) );

        if ( !IOBound )
          partial_sc = promoteToScheduled( algIndex, iSlot, thisSlotPtr->eventContext.get() );
        else
          partial_sc = promoteToAsyncScheduled( algIndex, iSlot, thisSlotPtr->eventContext.get() );

        ON_VERBOSE if ( partial_sc.isFailure() ) verbose() << "Could not apply transition from " << AState::DATAREADY
                                                           << " for algorithm " << index2algname( algIndex )
                                                           << " on processing slot " << iSlot << endmsg;
      }
    }

    // Check for algorithms ready in sub-slots
    for ( auto& subslot : thisSlot.allSubSlots ) {
      auto& subslotStates = subslot.algsStates;
      for ( auto it = subslotStates.begin( AState::DATAREADY ); it != subslotStates.end( AState::DATAREADY ); ++it ) {
        uint algIndex{*it};
        partial_sc = promoteToScheduled( algIndex, iSlot, subslot.eventContext.get() );
        // The following verbosity is expensive when the number of sub-slots is high
        /*ON_VERBOSE if ( partial_sc.isFailure() ) verbose()
            << "Could not apply transition from " << AState::DATAREADY << " for algorithm " << index2algname( algIndex )
            << " on processing subslot " << subslot.eventContext->slot() << endmsg;*/
      }
    }

    if ( m_dumpIntraEventDynamics ) {
      std::stringstream s;
      s << index2algname( algo_index ) << ", " << thisAlgsStates.sizeOfSubset( AState::CONTROLREADY ) << ", "
        << thisAlgsStates.sizeOfSubset( AState::DATAREADY ) << ", " << thisAlgsStates.sizeOfSubset( AState::SCHEDULED )
        << ", " << std::chrono::high_resolution_clock::now().time_since_epoch().count() << "\n";
      auto threads = ( m_threadPoolSize != -1 ) ? std::to_string( m_threadPoolSize )
                                                : std::to_string( tbb::task_scheduler_init::default_num_threads() );
      std::ofstream myfile;
      myfile.open( "IntraEventConcurrencyDynamics_" + threads + "T.csv", std::ios::app );
      myfile << s.str();
      myfile.close();
    }

    // Not complete because this would mean that the slot is already free!
    if ( m_precSvc->CFRulesResolved( thisSlot ) &&
         !thisSlot.algsStates.containsAny( {AState::CONTROLREADY, AState::DATAREADY, AState::SCHEDULED} ) &&
         !subSlotAlgsInStates( thisSlot, {AState::CONTROLREADY, AState::DATAREADY, AState::SCHEDULED} ) &&
         !thisSlot.complete ) {

      thisSlot.complete = true;
      // if the event did not fail, add it to the finished events
      // otherwise it is taken care of in the error handling
      if ( m_algExecStateSvc->eventStatus( *thisSlot.eventContext ) == EventStatus::Success ) {
        ON_DEBUG debug() << "Event " << thisSlot.eventContext->evt() << " finished (slot "
                         << thisSlot.eventContext->slot() << ")." << endmsg;
        m_finishedEvents.push( thisSlot.eventContext.release() );
      }

      // now let's return the fully evaluated result of the control flow
      ON_DEBUG debug() << m_precSvc->printState( thisSlot ) << endmsg;

      thisSlot.eventContext.reset( nullptr );

    } else if ( isStalled( thisSlot ) ) {
      m_algExecStateSvc->setEventStatus( EventStatus::AlgStall, *thisSlot.eventContext );
      eventFailed( thisSlot.eventContext.get() ); // can't release yet
    }
  } // end loop on slots

  ON_VERBOSE verbose() << "States Updated." << endmsg;

  return global_sc;
}

//---------------------------------------------------------------------------

/**
 * Check if we are in present of a stall condition for a particular slot.
 * This is the case when a slot has no actions queued in the actionsQueue,
 * has no scheduled algorithms and has no algorithms with all of its dependencies
 * satisfied.
*/
bool AvalancheSchedulerSvc::isStalled( const EventSlot& slot ) const
{

  if ( m_actionsCounts[slot.eventContext->slot()] == 0 &&
       !slot.algsStates.containsAny( {AState::DATAREADY, AState::SCHEDULED} ) &&
       !subSlotAlgsInStates( slot, {AState::DATAREADY, AState::SCHEDULED} ) ) {

    error() << "*** Stall detected in slot " << slot.eventContext->slot() << "! ***" << endmsg;

    return true;
  }
  return false;
}

//---------------------------------------------------------------------------

/**
 * It can be possible that an event fails. In this case this method is called.
 * It dumps the state of the scheduler and marks the event as finished.
*/
void AvalancheSchedulerSvc::eventFailed( EventContext* eventContext )
{
  const uint slotIdx = eventContext->slot();

  error() << "Event " << eventContext->evt() << " on slot " << slotIdx << " failed" << endmsg;

  dumpSchedulerState( msgLevel( MSG::VERBOSE ) ? -1 : slotIdx );

  // dump temporal and topological precedence analysis (if enabled in the PrecedenceSvc)
  m_precSvc->dumpPrecedenceRules( m_eventSlots[slotIdx] );

  // Push into the finished events queue the failed context
  m_eventSlots[slotIdx].complete = true;
  m_finishedEvents.push( m_eventSlots[slotIdx].eventContext.release() );
}

//---------------------------------------------------------------------------

/**
 * Used for debugging purposes, the state of the scheduler is dumped on screen
 * in order to be inspected.
**/
void AvalancheSchedulerSvc::dumpSchedulerState( int iSlot )
{

  // To have just one big message
  std::ostringstream outputMS;

  outputMS << "Dumping scheduler state\n"
           << "=========================================================================================\n"
           << "++++++++++++++++++++++++++++++++++++ SCHEDULER STATE ++++++++++++++++++++++++++++++++++++\n"
           << "=========================================================================================\n\n";

  //===========================================================================

  outputMS << "------------------ Last schedule: Task/Event/Slot/Thread/State Mapping "
           << "------------------\n\n";

  // Figure if TimelineSvc is available (used below to detect threads IDs)
  auto timelineSvc = serviceLocator()->service<ITimelineSvc>( "TimelineSvc", false );
  if ( !timelineSvc.isValid() || !timelineSvc->isEnabled() ) {
    outputMS << "WARNING Enable TimelineSvc in record mode (RecordTimeline = True) to trace the mapping\n";
  } else {

    // Figure optimal printout layout
    size_t indt( 0 );
    for ( auto& slot : m_eventSlots )
      for ( auto it = slot.algsStates.begin( AState::SCHEDULED ); it != slot.algsStates.end( AState::SCHEDULED ); ++it )
        if ( index2algname( (uint)*it ).length() > indt ) indt = index2algname( (uint)*it ).length();

    // Figure the last running schedule across all slots
    for ( auto& slot : m_eventSlots ) {
      for ( auto it = slot.algsStates.begin( AState::SCHEDULED ); it != slot.algsStates.end( AState::SCHEDULED );
            ++it ) {

        const std::string algoName{index2algname( (uint)*it )};

        outputMS << "  task: " << std::setw( indt ) << algoName << " evt/slot: " << slot.eventContext->evt() << "/"
                 << slot.eventContext->slot();

        // Try to get POSIX threads IDs the currently running tasks are scheduled to
        if ( timelineSvc.isValid() ) {
          TimelineEvent te{};
          te.algorithm = algoName;
          te.slot      = slot.eventContext->slot();
          te.event     = slot.eventContext->evt();

          if ( timelineSvc->getTimelineEvent( te ) )
            outputMS << " thread.id: 0x" << std::hex << te.thread << std::dec;
          else
            outputMS << " thread.id: [unknown]"; // this means a task has just
                                                 // been signed off as SCHEDULED,
                                                 // but has not been assigned to a thread yet
                                                 // (i.e., not running yet)
        }
        outputMS << " state: [" << m_algExecStateSvc->algExecState( algoName, *( slot.eventContext ) ) << "]\n";
      }
    }
  }

  //===========================================================================

  outputMS << "\n---------------------------- Task/CF/FSM Mapping "
           << ( 0 > iSlot ? "[all slots] --" : "[target slot] " ) << "--------------------------\n\n";

  int slotCount = -1;
  for ( auto& slot : m_eventSlots ) {
    ++slotCount;
    if ( slot.complete ) continue;

    outputMS << "[ slot: "
             << ( slot.eventContext->valid() ? std::to_string( slot.eventContext->slot() ) : "[ctx invalid]" )
             << "  event: "
             << ( slot.eventContext->valid() ? std::to_string( slot.eventContext->evt() ) : "[ctx invalid]" )
             << " ]:\n\n";

    if ( 0 > iSlot || iSlot == slotCount ) {

      // Snapshot of the Control Flow and FSM states
      outputMS << m_precSvc->printState( slot ) << "\n";

      // Mention sub slots (this is expensive if the number of sub-slots is high)
      /*if ( !slot.allSubSlots.empty() ) {
        outputMS << "\nNumber of sub-slots: " << slot.allSubSlots.size() << "\n\n";
        auto slotID = slot.eventContext->valid() ? std::to_string( slot.eventContext->slot() ) : "[ctx invalid]";
        for ( auto& ss : slot.allSubSlots ) {
          outputMS << "[ slot: " << slotID << " sub-slot entry: " << ss.entryPoint << "  event: "
                   << ( ss.eventContext->valid() ? std::to_string( ss.eventContext->evt() ) : "[ctx invalid]" )
                   << " ]:\n\n";
          outputMS << m_precSvc->printState( ss ) << "\n";
        }
      }*/
    }
  }

  //===========================================================================

  if ( 0 <= iSlot ) {
    outputMS << "\n------------------------------ Algorithm Execution States -----------------------------\n\n";
    m_algExecStateSvc->dump( outputMS, *( m_eventSlots[iSlot].eventContext ) );
  }

  outputMS << "\n=========================================================================================\n"
           << "++++++++++++++++++++++++++++++++++++++ END OF DUMP ++++++++++++++++++++++++++++++++++++++\n"
           << "=========================================================================================\n\n";

  info() << outputMS.str() << endmsg;
}

//---------------------------------------------------------------------------

StatusCode AvalancheSchedulerSvc::promoteToScheduled( unsigned int iAlgo, int si, EventContext* eventContext )
{

  if ( m_algosInFlight == m_maxAlgosInFlight ) return StatusCode::FAILURE;

  const std::string& algName( index2algname( iAlgo ) );
  IAlgorithm*        ialgoPtr = nullptr;
  StatusCode         sc( m_algResourcePool->acquireAlgorithm( algName, ialgoPtr ) );

  if ( sc.isSuccess() ) { // if we managed to get an algorithm instance try to schedule it

    ++m_algosInFlight;
    auto promote2ExecutedClosure = [this, iAlgo, ialgoPtr, eventContext]() {
      this->m_actionsQueue.push( [this, iAlgo, ialgoPtr, eventContext]() {
        return this->AvalancheSchedulerSvc::promoteToExecuted( iAlgo, eventContext->slot(), ialgoPtr, eventContext );
      } );
      return StatusCode::SUCCESS;
    };

    // Avoid to use tbb if the pool size is 1 and run in this thread
    if ( -100 != m_threadPoolSize ) {
      // the child task that executes an Algorithm
      tbb::task* algoTask = new ( tbb::task::allocate_root() )
          AlgoExecutionTask( ialgoPtr, *eventContext, serviceLocator(), m_algExecStateSvc, promote2ExecutedClosure );
      // schedule the algoTask
      tbb::task::enqueue( *algoTask );

    } else {
      AlgoExecutionTask theTask( ialgoPtr, *eventContext, serviceLocator(), m_algExecStateSvc,
                                 promote2ExecutedClosure );
      theTask.execute();
    }

    ON_DEBUG debug() << "Algorithm " << algName << " was submitted on event " << eventContext->evt() << " in slot "
                     << si << ". Algorithms scheduled are " << m_algosInFlight << endmsg;

    // Update states in the appropriate event slot
    StatusCode updateSc;
    EventSlot& thisSlot = m_eventSlots[si];
    if ( eventContext->usesSubSlot() ) {
      // Sub-slot
      size_t const subSlotIndex = eventContext->subSlot();
      updateSc                  = thisSlot.allSubSlots[subSlotIndex].algsStates.set( iAlgo, AState::SCHEDULED );
    } else {
      // Event level (standard behaviour)
      updateSc = thisSlot.algsStates.set( iAlgo, AState::SCHEDULED );
    }

    ON_VERBOSE dumpSchedulerState( -1 );

    if ( updateSc.isSuccess() )
      ON_VERBOSE verbose() << "Promoting " << algName << " to SCHEDULED on slot " << si << endmsg;
    return updateSc;
  } else {
    ON_DEBUG debug() << "Could not acquire instance for algorithm " << index2algname( iAlgo ) << " on slot " << si
                     << endmsg;
    return sc;
  }
}

//---------------------------------------------------------------------------

StatusCode AvalancheSchedulerSvc::promoteToAsyncScheduled( unsigned int iAlgo, int si, EventContext* eventContext )
{

  if ( m_IOBoundAlgosInFlight == m_maxIOBoundAlgosInFlight ) return StatusCode::FAILURE;

  // bool IOBound = m_precSvc->isBlocking(algName);

  const std::string& algName( index2algname( iAlgo ) );
  IAlgorithm*        ialgoPtr = nullptr;
  StatusCode         sc( m_algResourcePool->acquireAlgorithm( algName, ialgoPtr ) );

  if ( sc.isSuccess() ) { // if we managed to get an algorithm instance try to schedule it

    ++m_IOBoundAlgosInFlight;
    auto promote2ExecutedClosure = [this, iAlgo, ialgoPtr, eventContext]() {
      this->m_actionsQueue.push( [this, iAlgo, ialgoPtr, eventContext]() {
        return this->AvalancheSchedulerSvc::promoteToAsyncExecuted( iAlgo, eventContext->slot(), ialgoPtr,
                                                                    eventContext );
      } );
      return StatusCode::SUCCESS;
    };
    // Can we use tbb-based overloaded new-operator for a "custom" task (an algorithm wrapper, not derived from
    // tbb::task)? it seems it works..
    IOBoundAlgTask* theTask = new ( tbb::task::allocate_root() )
        IOBoundAlgTask( ialgoPtr, *eventContext, serviceLocator(), m_algExecStateSvc, promote2ExecutedClosure );
    m_IOBoundAlgScheduler->push( *theTask );

    ON_DEBUG debug() << "[Asynchronous] Algorithm " << algName << " was submitted on event " << eventContext->evt()
                     << " in slot " << si << ". algorithms scheduled are " << m_IOBoundAlgosInFlight << endmsg;

    // Update states in the appropriate event slot
    StatusCode updateSc;
    EventSlot& thisSlot = m_eventSlots[si];
    if ( eventContext->usesSubSlot() ) {
      // Sub-slot
      size_t const subSlotIndex = eventContext->subSlot();
      updateSc                  = thisSlot.allSubSlots[subSlotIndex].algsStates.set( iAlgo, AState::SCHEDULED );
    } else {
      // Event level (standard behaviour)
      updateSc = thisSlot.algsStates.set( iAlgo, AState::SCHEDULED );
    }

    ON_VERBOSE if ( updateSc.isSuccess() ) verbose() << "[Asynchronous] Promoting " << algName
                                                     << " to SCHEDULED on slot " << si << endmsg;
    return updateSc;
  } else {
    ON_DEBUG debug() << "[Asynchronous] Could not acquire instance for algorithm " << index2algname( iAlgo )
                     << " on slot " << si << endmsg;
    return sc;
  }
}

//---------------------------------------------------------------------------

/**
 * The call to this method is triggered only from within the AlgoExecutionTask.
*/
StatusCode AvalancheSchedulerSvc::promoteToExecuted( unsigned int iAlgo, int si, IAlgorithm* algo,
                                                     EventContext* eventContext )
{
  Gaudi::Hive::setCurrentContext( eventContext );
  StatusCode sc = m_algResourcePool->releaseAlgorithm( algo->name(), algo );

  if ( sc.isFailure() ) {
    error() << "[Event " << eventContext->evt() << ", Slot " << eventContext->slot() << "] "
            << "Instance of algorithm " << algo->name() << " could not be properly put back." << endmsg;
    return StatusCode::FAILURE;
  }

  --m_algosInFlight;

  EventSlot& thisSlot = m_eventSlots[si];

  ON_DEBUG debug() << "Trying to handle execution result of " << algo->name() << " on slot " << si << endmsg;

  const AlgExecState& algstate = m_algExecStateSvc->algExecState( algo, *eventContext );
  AState              state    = algstate.execStatus().isSuccess()
                     ? ( algstate.filterPassed() ? AState::EVTACCEPTED : AState::EVTREJECTED )
                     : AState::ERROR;

  // Update states in the appropriate slot
  int subSlotIndex = -1;
  if ( eventContext->usesSubSlot() ) {
    // Sub-slot
    subSlotIndex = eventContext->subSlot();
    sc           = thisSlot.allSubSlots[subSlotIndex].algsStates.set( iAlgo, state );
  } else {
    // Event level (standard behaviour)
    sc = thisSlot.algsStates.set( iAlgo, state );
  }

  ON_VERBOSE if ( sc.isSuccess() ) verbose() << "Promoting " << algo->name() << " on slot " << si << " to " << state
                                             << endmsg;

  ON_DEBUG debug() << "Algorithm " << algo->name() << " executed in slot " << si << ". Algorithms scheduled are "
                   << m_algosInFlight << endmsg;

  // Schedule an update of the status of the algorithms
  ++m_actionsCounts[si];
  m_actionsQueue.push( [this, si, iAlgo, subSlotIndex]() {
    --this->m_actionsCounts[si]; // no bound check needed as decrements/increments are balanced in the current setup
    return this->updateStates( -1, iAlgo, subSlotIndex, si );
  } );

  return sc;
}

//---------------------------------------------------------------------------

/**
 * The call to this method is triggered only from within the IOBoundAlgTask.
*/
StatusCode AvalancheSchedulerSvc::promoteToAsyncExecuted( unsigned int iAlgo, int si, IAlgorithm* algo,
                                                          EventContext* eventContext )
{
  Gaudi::Hive::setCurrentContext( eventContext );
  StatusCode sc = m_algResourcePool->releaseAlgorithm( algo->name(), algo );

  if ( sc.isFailure() ) {
    error() << "[Asynchronous]  [Event " << eventContext->evt() << ", Slot " << eventContext->slot() << "] "
            << "Instance of algorithm " << algo->name() << " could not be properly put back." << endmsg;
    return StatusCode::FAILURE;
  }

  --m_IOBoundAlgosInFlight;

  EventSlot& thisSlot = m_eventSlots[si];

  ON_DEBUG debug() << "[Asynchronous] Trying to handle execution result of " << algo->name() << " on slot " << si
                   << endmsg;

  const AlgExecState& algstate = m_algExecStateSvc->algExecState( algo, *eventContext );
  AState              state    = algstate.execStatus().isSuccess()
                     ? ( algstate.filterPassed() ? AState::EVTACCEPTED : AState::EVTREJECTED )
                     : AState::ERROR;

  // Update states in the appropriate slot
  int subSlotIndex = -1;
  if ( eventContext->usesSubSlot() ) {
    // Sub-slot
    subSlotIndex = eventContext->subSlot();
    sc           = thisSlot.allSubSlots[subSlotIndex].algsStates.set( iAlgo, state );
  } else {
    // Event level (standard behaviour)
    sc = thisSlot.algsStates.set( iAlgo, state );
  }

  ON_VERBOSE if ( sc.isSuccess() ) verbose() << "[Asynchronous] Promoting " << algo->name() << " on slot " << si
                                             << " to " << state << endmsg;

  ON_DEBUG debug() << "[Asynchronous] Algorithm " << algo->name() << " executed in slot " << si
                   << ". Algorithms scheduled are " << m_IOBoundAlgosInFlight << endmsg;

  // Schedule an update of the status of the algorithms
  ++m_actionsCounts[si];
  m_actionsQueue.push( [this, si, iAlgo, subSlotIndex]() {
    --this->m_actionsCounts[si]; // no bound check needed as decrements/increments are balanced in the current setup
    return this->updateStates( -1, iAlgo, subSlotIndex, si );
  } );

  return sc;
}

//---------------------------------------------------------------------------

// Method to inform the scheduler about event views

StatusCode AvalancheSchedulerSvc::scheduleEventView( const EventContext* sourceContext, const std::string& nodeName,
                                                     std::unique_ptr<EventContext> viewContext )
{
  //  Prevent view nesting
  if ( sourceContext->usesSubSlot() ) {
    fatal() << "Attempted to nest EventViews at node " << nodeName << ": this is not supported" << endmsg;
    return StatusCode::FAILURE;
  }

  ON_VERBOSE verbose() << "Queuing a view for [" << viewContext.get() << "]" << endmsg;

  // It's not possible to create an std::functional from a move-capturing lambda
  // So, we have to release the unique pointer
  auto action =
      [ this, slotIndex = sourceContext->slot(), viewContextPtr = viewContext.release(), &nodeName ]()->StatusCode
  {

    // Attach the sub-slot to the top-level slot
    EventSlot& topSlot = this->m_eventSlots[slotIndex];

    if ( viewContextPtr ) {
      // Re-create the unique pointer
      auto viewContext = std::unique_ptr<EventContext>( viewContextPtr );
      topSlot.addSubSlot( std::move( viewContext ), nodeName );
      return StatusCode::SUCCESS;
    } else {
      // Disable the view node if there are no views
      topSlot.disableSubSlots( nodeName );
      return StatusCode::SUCCESS;
    }
  };

  m_actionsQueue.push( std::move( action ) );

  return StatusCode::SUCCESS;
}
