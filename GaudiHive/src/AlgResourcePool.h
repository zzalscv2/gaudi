#ifndef GAUDIHIVE_ALGRESOURCEPOOL_H
#define GAUDIHIVE_ALGRESOURCEPOOL_H

// Include files
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/IAlgManager.h"
#include "GaudiKernel/IAlgResourcePool.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/Service.h"

// TODO: include here is only a workaround
#include <atomic>
#include <bitset>
#include <list>
#include <mutex>
#include <string>
#include <vector>

// External libs
#include "boost/dynamic_bitset.hpp"

#include "PrecedenceRulesGraph.h"
#include "tbb/concurrent_queue.h"

/** @class AlgResourcePool AlgResourcePool.h GaudiHive/AlgResourcePool.h

    The AlgResourcePool is a concrete implementation of the IAlgResourcePool interface.
    It either creates all instances upfront or lazily.
    Internal bookkeeping is done via hashes of the algo names.

    @author Benedikt Hegner
*/
class AlgResourcePool : public extends<Service, IAlgResourcePool>
{
public:
  // Standard constructor
  using extends::extends;
  // Standard destructor
  ~AlgResourcePool() override;

  StatusCode start() override;
  StatusCode initialize() override;
  /// Acquire a certain algorithm using its name
  StatusCode acquireAlgorithm( const std::string& name, IAlgorithm*& algo, bool blocking = false ) override;
  /// Release a certain algorithm
  StatusCode releaseAlgorithm( const std::string& name, IAlgorithm*& algo ) override;
  /// Acquire a certain resource
  StatusCode acquireResource( const std::string& name ) override;
  /// Release a certrain resource
  StatusCode releaseResource( const std::string& name ) override;

  std::list<IAlgorithm*> getFlatAlgList() override;
  std::list<IAlgorithm*> getTopAlgList() override;

  StatusCode beginRun() override;
  StatusCode endRun() override;

  StatusCode stop() override;

  virtual concurrency::PrecedenceRulesGraph* getPRGraph() const { return m_PRGraph; }

private:
  typedef tbb::concurrent_bounded_queue<IAlgorithm*> concurrentQueueIAlgPtr;
  typedef std::list<SmartIF<IAlgorithm>> ListAlg;
  typedef boost::dynamic_bitset<> state_type;

  std::mutex m_resource_mutex;

  state_type m_available_resources{0};
  std::map<size_t, concurrentQueueIAlgPtr*> m_algqueue_map;
  std::map<size_t, state_type> m_resource_requirements;
  std::map<size_t, size_t> m_n_of_allowed_instances;
  std::map<size_t, unsigned int> m_n_of_created_instances;
  std::map<std::string, unsigned int> m_resource_indices;

  /// Decode the top alg list
  StatusCode decodeTopAlgs();

  /// Recursively flatten an algList
  StatusCode flattenSequencer( Algorithm* sequencer, ListAlg& alglist, const std::string& parentName,
                               unsigned int recursionDepth = 0 );

  Gaudi::Property<bool> m_lazyCreation{this, "CreateLazily", false, ""};
  Gaudi::Property<std::vector<std::string>> m_topAlgNames{
      this, "TopAlg", {}, "names of the algorithms to be passed to the algorithm manager"};

  /// The list of all algorithms created withing the Pool which are not top
  ListAlg m_algList;

  /// The list of top algorithms
  ListAlg m_topAlgList;

  /// The flat list of algorithms w/o clones
  ListAlg m_flatUniqueAlgList;

  /// The flat list of algorithms w/o clones which is returned
  std::list<IAlgorithm*> m_flatUniqueAlgPtrList;

  /// The top list of algorithms
  std::list<IAlgorithm*> m_topAlgPtrList;

  /// OMG yet another hack
  concurrency::PrecedenceRulesGraph* m_PRGraph = nullptr;
};

#endif // GAUDIHIVE_ALGRESOURCEPOOL_H
