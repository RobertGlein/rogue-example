#include <rogue/protocols/udp/Core.h>
#include <rogue/protocols/udp/Client.h>
#include <rogue/protocols/rssi/Client.h>
#include <rogue/protocols/rssi/Transport.h>
#include <rogue/protocols/rssi/Application.h>
#include <rogue/protocols/packetizer/CoreV2.h>
#include <rogue/protocols/packetizer/Transport.h>
#include <rogue/protocols/packetizer/Application.h>
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>
#include <rogue/interfaces/stream/Buffer.h>
#include <rogue/Helpers.h>

//! Receive slave data, count frames and total bytes for example purposes.
class TestSink : public rogue::interfaces::stream::Slave {

   public:

      uint32_t rxCount;
      uint64_t rxBytes;
      uint32_t rxLast;

      TestSink() {
         rxCount = 0; // Total frames
         rxBytes = 0; // Total bytes
         rxLast  = 0; // Last frame size
      }

      void acceptFrame ( boost::shared_ptr<rogue::interfaces::stream::Frame> frame ) {
         uint32_t nbytes = frame->getPayload();
         rxLast   = nbytes;
         rxBytes += nbytes;
         rxCount++;

         // Iterators to start and end of frame
         rogue::interfaces::stream::Frame::iterator iter = frame->beginRead();
         rogue::interfaces::stream::Frame::iterator  end = frame->endRead();

         // Example destination for data copy
         uint8_t *buff = (uint8_t *)malloc (nbytes);
         uint8_t  *dst = buff;

         //Iterate through contigous buffers
         while ( iter != end ) {

            //  Get contigous size
            auto size = iter.remBuffer ();

            // Get the data pointer from current position
            auto *src = iter.ptr ();

            // Copy some data
            memcpy(dst, src, size);

            // Update destination pointer and source iterator
            dst  += size;
            iter += size;
         }
      }
};


int main (int argc, char **argv) {
   struct timeval last;
   struct timeval curr;
   struct timeval diff;
   double   timeDiff;
   uint64_t lastBytes;
   uint64_t diffBytes;
   double bw;
   // rglein
   // struct timeval last1;
   // struct timeval curr1;
   // struct timeval diff1;
   // double   timeDiff1;
   // uint64_t lastBytes1;
   // uint64_t diffBytes1;
   // double bw1;

   //rogue::Logging::setLevel(rogue::Logging::Debug);

   // Create the UDP client, jumbo = true
   rogue::protocols::udp::ClientPtr udp  = rogue::protocols::udp::Client::create("172.30.0.128",8192,true);
   udp->setRxBufferCount(64); // Make enough room for 64 outstanding buffers
   // rglein: Create the UDP client, jumbo = true
   // rogue::protocols::udp::ClientPtr udp1  = rogue::protocols::udp::Client::create("172.30.0.128",8192,true);
   // udp1->setRxBufferCount(64); // Make enough room for 64 outstanding buffers

   // RSSI
   rogue::protocols::rssi::ClientPtr rssi = rogue::protocols::rssi::Client::create(udp->maxPayload());
   // rglein: RSSI
   // rogue::protocols::rssi::ClientPtr rssi1 = rogue::protocols::rssi::Client::create(udp1->maxPayload());

   // Packetizer, ibCrc = false, obCrc = true
   rogue::protocols::packetizer::CoreV2Ptr pack = rogue::protocols::packetizer::CoreV2::create(false,true,true);
   // rglein: Packetizer, ibCrc = false, obCrc = true
   // rogue::protocols::packetizer::CoreV2Ptr pack1 = rogue::protocols::packetizer::CoreV2::create(false,true,true);

   // Connect the RSSI engine to the UDP client
   streamConnectBiDir(udp,rssi->transport());
   // rglein: Connect the RSSI engine to the UDP client
   // streamConnectBiDir(udp1,rssi1->transport());

   // Connect the RSSI engine to the packetizer
   streamConnectBiDir(rssi->application(), pack->transport());
   // rglein: Connect the RSSI engine to the packetizer
   // streamConnectBiDir(rssi1->application(), pack1->transport());

   // Create a test sink and connect to channel 1 of the packetizer
   boost::shared_ptr<TestSink> sink = boost::make_shared<TestSink>();
   streamConnect(pack->application(1),sink);
   // rglein: Create a test sink and connect to channel 1 of the packetizer
   // boost::shared_ptr<TestSink> sink1 = boost::make_shared<TestSink>();
   // streamConnect(pack1->application(1),sink1);

   // Start the rssi link
   rssi->start();
   // Start the rssi link
   // rssi1->start();

   // Loop forever showing counts
   lastBytes = 0;
   gettimeofday(&last,NULL);

   while(1) {
      sleep(1);
      gettimeofday(&curr,NULL);

      timersub(&curr,&last,&diff);

      diffBytes = sink->rxBytes - lastBytes;
      lastBytes = sink->rxBytes;

      timeDiff = (double)diff.tv_sec + ((double)diff.tv_usec / 1e6);
      bw = (((float)diffBytes * 8.0) / timeDiff) / 1e9;

      gettimeofday(&last,NULL);

      printf("RSSI = %i. RxLast=%i, RxCount=%i, RxTotal=%li, Bw=%f, DropRssi=%i, DropPack=%i\n",rssi->getOpen(),sink->rxLast,sink->rxCount,sink->rxBytes,bw,rssi->getDropCount(),pack->getDropCount());

      // rglein
      //rogue::interfaces::stream::FramePtr frame = rogue::interfaces::stream::Frame::create();
      //printf("acceptFrame=%l\n",frame.use_count()); //send->myGenFrame(data,100);
      // gettimeofday(&curr1,NULL);
      // timersub(&curr1,&last1,&diff1);
      // diffBytes1 = sink1->rxBytes - lastBytes1;
      // lastBytes1 = sink1->rxBytes;
      // timeDiff1 = (double)diff1.tv_sec + ((double)diff1.tv_usec / 1e6);
      // bw1 = (((float)diffBytes1 * 8.0) / timeDiff1) / 1e9;
      // gettimeofday(&last1,NULL);
      // printf("RSSI = %i. RxLast=%i, RxCount=%i, RxTotal=%li, Bw=%f, DropRssi=%i, DropPack=%i\n",rssi1->getOpen(),sink1->rxLast,sink1->rxCount,sink1->rxBytes,bw1,rssi1->getDropCount(),pack1->getDropCount());
   }
}
