/*
 * Project problematique
 * Description: 
 * Author: Charles Murphy (murc3002)
 * Date: 23 juin 2020
 */

#include "manchester.h"

namespace FrameLayer
{
	// assemblage et désassemblage de trames (frames)

	uint8_t input_data_buffer = 0;
	size_t frame_writer_index = 0;

	struct Frame
	{
	 	uint8_t preambule;
		uint8_t start;
		uint8_t header;
		uint32_t payload;
		uint8_t control;
		uint8_t end;
	};

	Frame* currentFrame;

	void onDataBufferFilled(uint8_t* input_data_buffer)
	{
		TRY_LOCK(Serial)
		{
			Serial.printlnf("Data received: %d", *input_data_buffer);
		}
		/*if (++frame_writer_index > sizeof(Frame)) 
		{
			((uint8_t*)(currentFrame))[frame_writer_index] = input_data_buffer;

			frame_writer_index = 0;
			currentFrame = new Frame{};
		}*/
	}

}

void setup() 
{
	Serial.begin(9600);
	Manchester::init(FrameLayer::onDataBufferFilled);
	uint8_t* data = new uint8_t[80];
	*data = 0x55;
	Manchester::send(data);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() 
{
}