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
#include <rogue/ApiWrapper.h>

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
   uint32_t ksize = 4000000;//8000000;
   uint8_t data[ksize];
   struct timeval last;
   struct timeval curr;
   struct timeval diff;
   double   timeDiff;
   uint64_t lastBytes;
   uint64_t diffBytes;
   double bw;

   uint8_t data1[ksize];
   struct timeval last1;
   struct timeval curr1;
   struct timeval diff1;
   double   timeDiff1;
   uint64_t lastBytes1;
   uint64_t diffBytes1;
   double bw1;

   //Create data
   for (int i = 0; i <= ksize; i++)
    {
        data[i] = i;
        data1[i] = ksize-i;
    }

   // Create the UDP client, jumbo = true
   rogue::protocols::udp::ClientPtr udp  = rogue::protocols::udp::Client::create("172.30.0.128",8192,true);
   udp->setRxBufferCount(64); // Make enough room for 64 outstanding buffers

   // RSSI
   rogue::protocols::rssi::ClientPtr rssi = rogue::protocols::rssi::Client::create(udp->maxPayload());

   // Packetizer, ibCrc = false, obCrc = true
   rogue::protocols::packetizer::CoreV2Ptr pack = rogue::protocols::packetizer::CoreV2::create(false,true,true);

   // Connect the RSSI engine to the UDP client
   streamConnectBiDir(udp, rssi->transport());

   // Connect the RSSI engine to the packetizer
   streamConnectBiDir(rssi->application(), pack->transport());

   // Create a test source and connect to channel 1 of the packetizer
   boost::shared_ptr<TestSend> send = boost::make_shared<TestSend>();
   streamConnect(send,pack->application(0));
   // Create a test source and connect to channel 1 of the packetizer
   boost::shared_ptr<TestSend> send1 = boost::make_shared<TestSend>();
   streamConnect(send1,pack->application(0));

   // Start the rssi link
   rssi->start();

   // Loop forever showing counts
   lastBytes = 0;
   gettimeofday(&last,NULL);

   while(1) {

      send->myGenFrame(data,ksize);
      //sleep(1);
      gettimeofday(&curr,NULL);
      timersub(&curr,&last,&diff);
      diffBytes = send->txBytes - lastBytes;
      lastBytes = send->txBytes;
      timeDiff = (double)diff.tv_sec + ((double)diff.tv_usec / 1e6);
      bw = (((float)diffBytes * 8.0) / timeDiff) / 1e9;
      gettimeofday(&last,NULL);
      printf("RSSI = %i. TxLast=%i, TxCount=%i, TxTotal=%li, Bw=%f, DropRssi=%i, DropPack=%i\n",rssi->getOpen(),send->txLast,send->txCount,send->txBytes,bw,rssi->getDropCount(),pack->getDropCount());

      send1->myGenFrame(data1,ksize);
      gettimeofday(&curr1,NULL);
      timersub(&curr1,&last1,&diff1);
      diffBytes1 = send1->txBytes - lastBytes1;
      lastBytes1 = send1->txBytes;
      timeDiff1 = (double)diff1.tv_sec + ((double)diff1.tv_usec / 1e6);
      bw1 = (((float)diffBytes1 * 8.0) / timeDiff1) / 1e9;
      gettimeofday(&last1,NULL);
      printf("RSSI = %i. TxLast=%i, TxCount=%i, TxTotal=%li, Bw=%f, DropRssi=%i, DropPack=%i\n\n",rssi->getOpen(),send1->txLast,send1->txCount,send1->txBytes,bw1,rssi->getDropCount(),pack->getDropCount());
   }
}
