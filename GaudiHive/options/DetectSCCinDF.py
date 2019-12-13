#!/usr/bin/env gaudirun.py
"""
A test modeling a cycle in the DF realm.
"""

from Gaudi.Configuration import *
from Configurables import HiveWhiteBoard, HiveSlimEventLoopMgr, AvalancheSchedulerSvc, CPUCruncher, CPUCrunchSvc, PrecedenceSvc

# metaconfig
evtMax = 1
evtslots = 1
algosInFlight = 1

CPUCrunchSvc(shortCalib=True)

PrecedenceSvc(OutputLevel=DEBUG)

whiteboard = HiveWhiteBoard(
    "EventDataSvc", EventSlots=evtslots, OutputLevel=INFO)

slimeventloopmgr = HiveSlimEventLoopMgr(
    SchedulerName="AvalancheSchedulerSvc", OutputLevel=INFO)

AvalancheSchedulerSvc(ThreadPoolSize=algosInFlight)

# Set up a data flow cycle ############
Alg1 = CPUCruncher(name="CycledAlg1")
Alg1.inpKeys = ["/Event/B", "/Event/F"]
Alg1.outKeys = ["/Event/C", "/Event/A"]

Alg2 = CPUCruncher(name="CycledAlg2")
Alg2.inpKeys = ["/Event/C"]
Alg2.outKeys = ["/Event/D", "/Event/B"]

Alg3 = CPUCruncher(name="CycledAlg3")
Alg3.inpKeys = ["/Event/D"]
Alg3.outKeys = ["/Event/F"]
#######################################

Alg4 = CPUCruncher(name="Alg4")
Alg4.inpKeys = ["/Event/A"]

Alg5 = CPUCruncher(name="Alg5")
Alg5.outKeys = ["/Event/E"]

ApplicationMgr(
    EvtMax=evtMax,
    EvtSel='NONE',
    ExtSvc=[whiteboard],
    EventLoop=slimeventloopmgr,
    TopAlg=[Alg1, Alg2, Alg3, Alg4, Alg5],
    MessageSvcType="InertMessageSvc",
    OutputLevel=INFO)
