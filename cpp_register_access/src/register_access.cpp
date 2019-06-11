#include <rogue/Logging.h>
#include <rogue/protocols/udp/Core.h>
#include <rogue/protocols/udp/Client.h>
#include <rogue/protocols/rssi/Client.h>
#include <rogue/protocols/rssi/Transport.h>
#include <rogue/protocols/rssi/Application.h>
#include <rogue/protocols/packetizer/CoreV2.h>
#include <rogue/protocols/packetizer/Transport.h>
#include <rogue/protocols/packetizer/Application.h>
#include <rogue/protocols/srp/SrpV3.h>
#include <rogue/interfaces/memory/Master.h>
#include <rogue/interfaces/memory/Constants.h>
#include <rogue/Helpers.h>


int main (int argc, char **argv) {
   uint32_t ver;
   uint32_t spad;
   uint32_t upTimeCnt;

   uint32_t ksize=500; // 2**13 b=8169=0x2000; bc of 32 b -> max addr 499
   uint32_t memWr[ksize];
   uint32_t memWr1[ksize];
   uint32_t memRd[ksize];
   uint32_t memRd1[ksize];


   //rogue::Logging::setLevel(rogue::Logging::Debug);

   // Create the UDP client, jumbo = true
   rogue::protocols::udp::ClientPtr udp  = rogue::protocols::udp::Client::create("172.30.0.128",8192,true);
   udp->setRxBufferCount(64); // Make enough room for 64 outstanding buffers

   // RSSI
   rogue::protocols::rssi::ClientPtr rssi = rogue::protocols::rssi::Client::create(udp->maxPayload());
   udp->setSlave(rssi->transport());
   rssi->transport()->setSlave(udp);

   // Packetizer, ibCrc = false, obCrc = true
   rogue::protocols::packetizer::CoreV2Ptr pack = rogue::protocols::packetizer::CoreV2::create(false,true,true);
   rssi->application()->setSlave(pack->transport());
   pack->transport()->setSlave(rssi->application());

   // Create an SRP master and connect it to the packetizer
   rogue::protocols::srp::SrpV3Ptr srp = rogue::protocols::srp::SrpV3::create();
   pack->application(0)->setSlave(srp);
   srp->setSlave(pack->application(0));

   // Create a memory master and connect it to the srp
   rogue::interfaces::memory::MasterPtr mast = rogue::interfaces::memory::Master::create();
   mast->setSlave(srp);
   // Create a memory master and connect it to the srp
   rogue::interfaces::memory::MasterPtr mast1 = rogue::interfaces::memory::Master::create();
   mast1->setSlave(srp);

   // Start the RSSI connection
   rssi->start();

   while ( ! rssi->getOpen() ) {
      sleep(1);
      printf("Establishing link ...\n");
   }


   ver = 0xFFFFFFFF;
   spad = 0xFFFFFFFF;
   upTimeCnt = 0xFFFFFFFF;

   // Read from fpga version
   printf("\n -- Read from fpga version ---------------------------------- \n");
   mast->reqTransaction(0x00000000,4,&ver,rogue::interfaces::memory::Read);
   mast->reqTransaction(0x00000004,4,&spad,rogue::interfaces::memory::Read);
   mast->reqTransaction(0x00000008,4,&upTimeCnt,rogue::interfaces::memory::Read);
   mast->waitTransaction(0);
   printf("Register done. Value=0x%x, Spad=0x%x, UpTimeCnt=0x%x, Error=0x%x\n",ver,spad,upTimeCnt,mast->getError());


   // rglein test
   printf("\n -- rglein test ---------------------------------- \n");
   for (int i = 0; i <= ksize; i++)
    {
        memWr[i] = i;
        memRd[i] = 0;
        memWr1[i] = i+ksize;
        memRd1[i] = 0;
    }
    mast->reqTransaction(0x00030000,2000,&memWr[0],rogue::interfaces::memory::Write);
    mast->waitTransaction(0);
    printf("Register done. Value=0x%x, Spad=0x%x, UpTimeCnt=0x%x, Error=0x%x\n",ver,spad,upTimeCnt,mast->getError());
    printf("memWr[0]: 0x%x, memWr[1]: 0x%x, memWr[ksize]: 0x%x \n ", memWr[0], memWr[1], memWr[ksize]);
    mast->reqTransaction(0x00030000,2000,&memRd[0],rogue::interfaces::memory::Read);
    mast->waitTransaction(0);
    printf("Register done. Value=0x%x, Spad=0x%x, UpTimeCnt=0x%x, Error=0x%x\n",ver,spad,upTimeCnt,mast->getError());
    printf("memRd[256]: 0x%x, memRd[499]: 0x%x, memRd[500]: 0x%x \n ", memRd[256], memRd[499], memRd[500]); // 499 is the last valid addr bc 32 b access 2000/4-1

    printf("\n -- Loop test ------------------------------------ \n");
    for (int i = 0; i <= 10000; i++)
    {
         mast->reqTransaction(0x00030000,2000,&memWr[0],rogue::interfaces::memory::Write);
         mast1->reqTransaction(0x00030000,2000,&memWr1[0],rogue::interfaces::memory::Write);
         mast->waitTransaction(0);
         mast1->waitTransaction(0);
         mast->reqTransaction(0x00030000,2000,&memRd[0],rogue::interfaces::memory::Read);
         mast1->reqTransaction(0x00030000,2000,&memRd1[0],rogue::interfaces::memory::Read);
         mast->waitTransaction(0);
         mast1->waitTransaction(0);
     }
     printf("Register done. Value=0x%x, Spad=0x%x, UpTimeCnt=0x%x, Error=0x%x\n",ver,spad,upTimeCnt,mast->getError());
     printf("memRd[256]: 0x%x, memRd[499]: 0x%x, memRd[500]: 0x%x \n ", memRd[256], memRd[499], memRd[500]); // 499 is the last valid addr bc 32 b access 2000/4-1
     printf("Register done. Value=0x%x, Spad=0x%x, UpTimeCnt=0x%x, Error=0x%x\n",ver,spad,upTimeCnt,mast1->getError());
     printf("memRd[256]: 0x%x, memRd[499]: 0x%x, memRd[500]: 0x%x \n ", memRd1[256], memRd1[499], memRd1[500]); // 499 is the last valid addr bc 32 b access 2000/4-1

}
