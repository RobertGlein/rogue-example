#include <rogue/ApiWrapper.h>

int main (int argc, char **argv) {

   try {
      rogue::ApiWrapperPtr wrap = rogue::ApiWrapper::remote("rogueTest","evalBoard");

      printf("Return value int = %li\n",wrap->getUInt32("evalBoard.AxiVersion.UpTimeCnt"));
      printf("Return value str = %s\n",wrap->getString("evalBoard.AxiVersion.UpTimeCnt").c_str());

      wrap->setUInt32("evalBoard.AxiVersion.ScratchPad",0xCC);
      printf("Spad value int = 0x%x\n",wrap->getUInt32("evalBoard.AxiVersion.ScratchPad"));

   } catch (boost::python::error_already_set) {
      PyErr_Print();
   }
}

