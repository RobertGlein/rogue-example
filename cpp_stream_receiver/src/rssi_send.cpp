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

//! Receive slave data, count frames and total bytes for example purposes.
class TestSend : public rogue::interfaces::stream::Master {

   public:

      uint32_t txCount;
      uint64_t txBytes;
      uint32_t txLast;

      TestSend() {
         txCount = 0; // Total frames
         txBytes = 0; // Total bytes
         txLast  = 0; // Last frame size
      }

      void myGenFrame(uint8_t *data, uint32_t size) {
         rogue::interfaces::stream::FramePtr frame;
         rogue::interfaces::stream::FrameIterator it;

         // Request frame
         frame = reqFrame(size,true);

         // Get data write iterator
         it = frame->beginWrite();

         std::copy(data,data+size,it);

         // Set new frame size
         frame->setPayload(size);

         //Send frame
         sendFrame(frame);

         // Update counters
         txCount += 1;
         txBytes += size;
         txLast  = size;
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

   // Create the UDP client, jumbo = true
   rogue::protocols::udp::ClientPtr udp  = rogue::protocols::udp::Client::create("192.168.2.187",8194,true);
   udp->setRxBufferCount(64); // Make enough room for 64 outstanding buffers

   // RSSI
   rogue::protocols::rssi::ClientPtr rssi = rogue::protocols::rssi::Client::create(udp->maxPayload());

   // Packetizer, ibCrc = false, obCrc = true
   rogue::protocols::packetizer::CoreV2Ptr pack = rogue::protocols::packetizer::CoreV2::create(false,true,true);

   // Connect the RSSI engine to the UDP client
   udp->setSlave(rssi->transport());
   rssi->transport()->setSlave(udp);

   // Connect the RSSI engine to the packetizer
   rssi->application()->setSlave(pack->transport());
   pack->transport()->setSlave(rssi->application());

   // Create a test source and connect to channel 1 of the packetizer
   boost::shared_ptr<TestSend> send = boost::make_shared<TestSend>();
   send->setSlave(pack->application(1));

   // Start the rssi link
   rssi->start();

   // Loop forever showing counts
   lastBytes = 0;
   gettimeofday(&last,NULL);

   while(1) {

      //send->myGenFrame(data,size);
      sleep(1);
      gettimeofday(&curr,NULL);

      timersub(&curr,&last,&diff);

      diffBytes = send->txBytes - lastBytes;
      lastBytes = send->txBytes;

      timeDiff = (double)diff.tv_sec + ((double)diff.tv_usec / 1e6);
      bw = (((float)diffBytes * 8.0) / timeDiff) / 1e9;

      gettimeofday(&last,NULL);

      printf("RSSI = %i. TxLast=%i, TxCount=%i, TxTotal=%li, Bw=%f, DropRssi=%i, DropPack=%i\n",rssi->getOpen(),send->txLast,send->txCount,send->txBytes,bw,rssi->getDropCount(),pack->getDropCount());
   }
}

