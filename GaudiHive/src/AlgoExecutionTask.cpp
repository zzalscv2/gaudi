// Framework
#include "AlgoExecutionTask.h"
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/IMessageSvc.h"
#include "GaudiKernel/IProperty.h"
#include "GaudiKernel/ContextSpecificPtr.h"

// C++
#include <functional>

// local includes
#include "RetCodeGuard.h"

tbb::task* AlgoExecutionTask::execute() {

  Algorithm* this_algo = dynamic_cast<Algorithm*>(m_algorithm.get());  
   if (!this_algo){
     throw GaudiException ("Cast to Algorithm failed!","AlgoExecutionTask",StatusCode::FAILURE);
   }

  bool eventfailed=false;
  EventContext* eventContext = this_algo->getContext();
  eventContext->m_thread_id = pthread_self();
  Gaudi::Hive::setCurrentContextId(eventContext->m_evt_slot);

  // Get the IProperty interface of the ApplicationMgr to pass it to RetCodeGuard
  const SmartIF<IProperty> appmgr(m_serviceLocator);
  
  SmartIF<IMessageSvc> messageSvc (m_serviceLocator);
  MsgStream log(messageSvc, "AlgoExecutionTask");

  StatusCode sc(StatusCode::FAILURE);
  try {
    RetCodeGuard rcg(appmgr, Gaudi::ReturnCode::UnhandledException);
    sc = m_algorithm->sysExecute();
    if (UNLIKELY(!sc.isSuccess()))  {
      log << MSG::WARNING << "Execution of algorithm " << m_algorithm->name() << " failed" << endmsg;
    eventfailed = true;
  }    
    rcg.ignore(); // disarm the guard
  } catch ( const GaudiException& Exception ) {
    log << MSG::FATAL << ".executeEvent(): Exception with tag=" << Exception.tag()
            << " thrown by " << m_algorithm->name() << endmsg;
    log << MSG::ERROR << Exception << endmsg;
  } catch ( const std::exception& Exception ) {
    log << MSG::FATAL << ".executeEvent(): Standard std::exception thrown by "
            << m_algorithm->name() << endmsg;
    log << MSG::ERROR <<  Exception.what()  << endmsg;
  } catch(...) {
    log << MSG::FATAL << ".executeEvent(): UNKNOWN Exception thrown by "
            << m_algorithm->name() << endmsg;
  }  

  // DP it is important to propagate the failure of an event.
  // We need to stop execution when this happens so that execute run can 
  // then receive the FAILURE
  eventContext->m_evt_failed=eventfailed;  
  
  // Push in the scheduler queue an action to be performed 
  auto action_promote2Executed = std::bind(&ForwardSchedulerSvc::promoteToExecuted,
                                           m_schedSvc, 
                                           m_algoIndex, 
                                           eventContext->m_evt_slot,
                                           m_algorithm);

  m_schedSvc->m_actionsQueue.push(action_promote2Executed);    

  return nullptr;
}