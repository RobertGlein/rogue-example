
# Python 3 support
source /afs/slac.stanford.edu/g/reseng/python/3.5.2/settings.csh
source /afs/slac.stanford.edu/g/reseng/boost/1.62.0_p3/settings.csh

# Python 2 support
#source /afs/slac.stanford.edu/g/reseng/python/2.7.13/settings.csh
#source /afs/slac.stanford.edu/g/reseng/boost/1.62.0_p2/settings.csh

source /afs/slac.stanford.edu/g/reseng/zeromq/4.2.0/settings.csh
source /afs/slac.stanford.edu/g/reseng/epics/base-R3-16-0/settings.csh

# Package directories
setenv SURF_DIR   ${PWD}/../surf
setenv ROGUE_DIR  ${PWD}/../rogue

# Setup python path
setenv PYTHONPATH ${PWD}/python:${SURF_DIR}/python:${ROGUE_DIR}/python:${PYTHONPATH}

# Setup library path
setenv LD_LIBRARY_PATH ${ROGUE_DIR}/python::${LD_LIBRARY_PATH}

