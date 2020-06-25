/*
 * Project problematique
 * Description: 
 * Author: Charles Murphy (murc3002)
 * Date: 23 juin 2020
 */

#include "problematique.h"
#include "manchester.h"
#include "testFrame.h"
#include "frameWriter.h"
#include "frameParser.h"

SYSTEM_THREAD(ENABLED);

Thread byteSendThread("byteSenderThread", byteSenderThread);
Thread applicationReadThread("applicationReadThread", applicationReaderThread); // Simulation des couches superieures
Timer timer(APPLICATION_DATA_CREATION_PERIOD_MS, applicationSenderCallback); // Simulation de la creation et l'envoi de donnees depuis les couches superieures
FrameWriter frameWriter;
FrameParser frameParser;
int testFramePtr = 0;

namespace FrameLayer
{
	// assemblage et désassemblage de trames (frames)

	uint8_t input_data_buffer = 0;
	size_t frame_writer_index = 0;

	void onDataBufferFilled(const uint8_t& input_data_buffer)
	{	
		TRY_LOCK(Serial)
		{
			Serial.printlnf("Data received: %x", input_data_buffer);
		}

		frameParser.acquireData(input_data_buffer);
	}
}

void setup() 
{
	Serial.begin(9600);
	Manchester::init(FrameLayer::onDataBufferFilled);

	// Create a frame to be sent
	frameWriter.setFrame(0x11, TestFrame::testPayload, TestFrame::testPayloadLength);
	timer.start(); // The next frames will be created here
}

// loop() runs over and over again, as quickly as it can execute.
void loop() 
{
	// unitTestFrameParser(); 
}

void byteSenderThread() {
    while(true) {
        waitUntil([]() { return frameWriter.frameReady(); });

		uint8_t byte = 0;
		if (frameWriter.nextByte(&byte)) {
			/*
			WITH_LOCK(Serial) {
				Serial.printlnf("Sending byte no.%d : %x", frameWriter.getBytePointer(), byte);
			}*/

			Manchester::send(byte);
		}

		os_thread_yield();
    }
}

void applicationReaderThread() {
	while(true) {
		waitUntil([]() { return frameParser.dataAvailable(); });

		uint8_t* data;
		uint8_t length = frameParser.getData(data);

		WITH_LOCK(Serial) {
			Serial.print("The application has recieved: ");
			for (int i = 0; i < length; i++) {
				Serial.printf(" %x ", data[i]);
			}
		}

		frameParser.reset();
	}
}

void applicationSenderCallback() {
	frameWriter.reset();
	frameWriter.setFrame(0x11, TestFrame::testPayload, TestFrame::testPayloadLength);

	/*
	if (testFrameShouldHaveErrors) { // Aux fins de test, on cree volontairement des trames erronees.
		frameWriter.setFaultyBit(17);
		testFrameShouldHaveErrors = false;
	} else {
		testFrameShouldHaveErrors = true;
	}*/
}

void unitTestFrameParser() {
	if (testFramePtr < 11) {
		FrameLayer::onDataBufferFilled(TestFrame::testData[testFramePtr++]);
	} else {
		uint8_t* data;
		uint8_t length = frameParser.getData(data);

		WITH_LOCK(Serial) {
			Serial.printf("Payload recieved:");
		}
		for (int i = 0; i < length; i++) {
			WITH_LOCK(Serial) {
				Serial.printf(" %x ", data[i]);
			}
		}
	}

	delay(500);
}