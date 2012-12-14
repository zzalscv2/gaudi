gaudi_subdir(GaudiHive v1r0)

gaudi_depends_on_subdirs(GaudiKernel GaudiAlg GaudiCommonSvc GaudiCoreSvc)

find_package(AIDA)
find_package(Boost COMPONENTS system filesystem)
find_package(ROOT COMPONENTS Hist RIO)
find_package(TBB)
find_package(pyanalysis)

#---Libraries---------------------------------------------------------------
gaudi_add_module(GaudiHive
                        *.cpp 
                        LINK_LIBRARIES GaudiKernel Boost ROOT TBB GaudiAlgLib GaudiCoreSvcHive
                        INCLUDE_DIRS Boost ROOT AIDA TBB)