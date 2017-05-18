#!/usr/bin/env python3
#-----------------------------------------------------------------------------
# Title      : Eval board instance
#-----------------------------------------------------------------------------
# File       : evalBoard.py
# Author     : Ryan Herbst, rherbst@slac.stanford.edu
# Created    : 2016-09-29
# Last update: 2016-09-29
#-----------------------------------------------------------------------------
# Description:
# Rogue interface to eval board
#-----------------------------------------------------------------------------
# This file is part of the rogue_example software. It is subject to 
# the license terms in the LICENSE.txt file found in the top-level directory 
# of this distribution and at: 
#    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
# No part of the rogue_example software, including this file, may be 
# copied, modified, propagated, or distributed except according to the terms 
# contained in the LICENSE.txt file.
#-----------------------------------------------------------------------------
import pyrogue.utilities.prbs
import pyrogue.utilities.fileio
import rogue.interfaces.stream
import pyrogue.mesh
import pyrogue.epics
import surf
import surf.AxiVersion
import surf.SsiPrbsTx
import threading
import signal
import atexit
import yaml
import time
import sys
import testBridge

# Microblaze console printout
class MbDebug(rogue.interfaces.stream.Slave):

   def __init__(self):
      rogue.interfaces.stream.Slave.__init__(self)
      self.enable = False

   def _acceptFrame(self,frame):
      if self.enable:
         p = bytearray(frame.getPayload())
         frame.read(p,0)
         print('-------- Microblaze Console --------')
         print(p.decode('utf-8'))

# Custom run control
class MyRunControl(pyrogue.RunControl):
   def __init__(self,name):
      pyrogue.RunControl.__init__(self,name,'Run Controller')
      self._thread = None

      self.runRate.updateEnum({1:'1 Hz', 10:'10 Hz', 30:'30 Hz'})

   def _setRunState(self,dev,var,value,changed):
      if changed:
         if self.runState.get(read=False) == 'Running':
            self._thread = threading.Thread(target=self._run)
            self._thread.start()
         else:
            self._thread.join()
            self._thread = None

   def _run(self):
      self.runCount.set(0)
      self._last = int(time.time())

      while (self.runState.get(read=False) == 'Running'):



         delay = 1.0 / ({value: key for key,value in self.runRate.enum.items()}[self._runRate])
         time.sleep(delay)
         self._root.ssiPrbsTx.oneShot()

         self._runCount += 1
         if self._last != int(time.time()):
             self._last = int(time.time())
             self.runCount._updated()

# Set base
evalBoard = pyrogue.Root('evalBoard','Evaluation Board')

# Run control
evalBoard.add(MyRunControl('runControl'))

# File writer
dataWriter = pyrogue.utilities.fileio.StreamWriter('dataWriter')
evalBoard.add(dataWriter)

# Create the PGP interfaces
pgpVc0 = rogue.hardware.pgp.PgpCard('/dev/pgpcard_0',0,0) # Registers
pgpVc1 = rogue.hardware.pgp.PgpCard('/dev/pgpcard_0',0,1) # Data
pgpVc3 = rogue.hardware.pgp.PgpCard('/dev/pgpcard_0',0,3) # Microblaze

print("")
print("PGP Card Version: %x" % (pgpVc0.getInfo().version))

# Create and Connect SRP to VC0
srp = rogue.protocols.srp.SrpV0()
pyrogue.streamConnectBiDir(pgpVc0,srp)

# Add configuration stream to file as channel 0
pyrogue.streamConnect(evalBoard,dataWriter.getChannel(0x0))

# Add data stream to file as channel 1
pyrogue.streamConnect(pgpVc1,dataWriter.getChannel(0x1))

## Add microblaze console stream to file as channel 2
pyrogue.streamConnect(pgpVc3,dataWriter.getChannel(0x2))

# PRBS Receiver as secdonary receiver for VC1
prbsRx = pyrogue.utilities.prbs.PrbsRx('prbsRx')
pyrogue.streamTap(pgpVc1,prbsRx)
evalBoard.add(prbsRx)

# Microblaze console monitor add secondary tap
mbcon = MbDebug()
pyrogue.streamTap(pgpVc3,mbcon)

#br = testBridge.Bridge()
#br._setSlave(srp)

# Add Devices
#evalBoard.add(surf.AxiVersion.create(memBase=br,offset=0x0))
evalBoard.add(surf.AxiVersion.create(memBase=srp,offset=0x0))
evalBoard.add(surf.SsiPrbsTx.create(memBase=srp,offset=0x30000))

# Create mesh node
mNode = pyrogue.mesh.MeshNode('rogueTest',iface='eth3',root=evalBoard)
mNode.start()

# Create epics node
epics = pyrogue.epics.EpicsCaServer('rogueTest',evalBoard)
epics.start()

# Close window and stop polling
def stop():
    mNode.stop()
    epics.stop()
    evalBoard.stop()
    exit()

# Start with ipython -i scripts/evalBoard.py
print("Started rogue mesh and epics V3 server. To exit type stop()")

