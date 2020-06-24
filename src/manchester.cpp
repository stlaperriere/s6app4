#include "manchester.h"

SYSTEM_THREAD(ENABLED);

namespace Manchester
{
    // PARAMETERS
    const int output_pin = D6;
    const int input_pin = D5;

    namespace Sender
    {
        // PARAMETERS
        system_tick_t lastTick = 0;
        const unsigned long sendingDelay = 500;

        volatile bool isSending, isMidBit, isReady = false;

        volatile size_t bufferIndex, byteIndex;
        volatile uint8_t * dataBuffer;

        void senderThread();
        Thread sendThread("senderThread", senderThread);

        void reset()
        {
            bufferIndex = 0;
            byteIndex = 0;
            isSending = false;
            dataBuffer = nullptr;
        }

        void init()
        {
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Sender init");
            }
            reset();
            isReady = true;
        }

        void onSystemTick()
        {
            if(!isSending)
                return;

            uint8_t currentData = dataBuffer[bufferIndex] >> byteIndex;
            if((currentData & 1) == 0)
            {
                if(!isMidBit)
                {
                    pinSetFast(output_pin);
                }
                else 
                {
                    pinResetFast(output_pin);
                }
            }
            else  
            {
                if(!isMidBit)
                {
                    pinResetFast(output_pin);
                }
                else 
                {
                    pinSetFast(output_pin);
                }
            }

            if(isMidBit)
                byteIndex++;

            if (byteIndex >= 8)
            {
                bufferIndex++;
                byteIndex = 0;
            }

            isMidBit = !isMidBit;

            if (bufferIndex >= sizeof(dataBuffer))
            {
                reset();
            }
        }

        void send(uint8_t* data)
        {
            dataBuffer = data; // ajouter une data queue?
            // send 1 byte à la fois?
        }

        void senderThread()
        {
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Awaiting sender init...");
            }
            waitUntil([]() { return isReady;});

            while(true)
            {
                bufferIndex = 0;
                byteIndex = 0;
                isMidBit = false;
                WITH_LOCK(Serial)
                {
                    Serial.printlnf("Awaiting send data buffer init...");
                }
			    waitUntil([]() { return dataBuffer != nullptr; });
                WITH_LOCK(Serial)
                {
                    Serial.printlnf("Starting to send %x", *dataBuffer);
                }
		        attachInterruptDirect(SysTick_IRQn, onSystemTick);
                isSending = true;
			    waitUntil([]() { return !isSending; });
                detachInterruptDirect(SysTick_IRQn);
                os_thread_delay_until(&lastTick, sendingDelay);
            }
        }
    }

    namespace Receiver
    {
        // PARAMETERS
        const size_t MAX_BUFFER_SIZE = 80;
	    void (*onBufferFilled)(uint8_t*);
        volatile bool isFirstEdge;
        volatile bool isReceiving;
        volatile unsigned long signalStart, periodWidth;
        
        volatile size_t bufferIndex, byteIndex;
        uint8_t* dataBuffer;

        volatile unsigned long endTime;

        void onEdgeChange();
        void reset();

        void receiverThread();
        Thread receiveThread("receiverThread", receiverThread);

        void init(void (*onBufferFilledCallback)(uint8_t*))
        {
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Receiver init");
            }
            onBufferFilled = onBufferFilledCallback;

            reset();
            pinResetFast(input_pin);
            attachInterrupt(input_pin, onEdgeChange, CHANGE);
        }

        void receiverThread()
        {
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Awaiting reception");
            }
            waitUntil([]() {  return isReceiving; });
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Started receiving");
            }
            waitUntil([]() {  return micros() - endTime > periodWidth * 5;  });
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Finished receiving");
            }
            isReceiving = false;
            onBufferFilled(dataBuffer);
        }

        void reset()
        {
            if(dataBuffer != nullptr)
                delete[] dataBuffer;
            dataBuffer = new uint8_t[MAX_BUFFER_SIZE];

            isReceiving = false;
            endTime = 0;
            periodWidth = 0;
            bufferIndex = 0;
            byteIndex = 0;
            signalStart = micros();
            isFirstEdge = true;
        }

        inline unsigned long signalTime()
        {
            return micros() - signalStart;
        }

        void addBitToBuffer(bool bit)
        {
            ATOMIC_BLOCK()
            {		
                dataBuffer[bufferIndex] |= (bit ? 1 : 0) << byteIndex++;

                if(byteIndex >= 8)
                {
                    /*
                    CANNOT ALLOCATE MEMORY FROM AN ISR
                    uint8_t* temp = new uint8_t[++bufferIndex];
                    for(size_t i = 0; i < sizeof(dataBuffer); i++)
                    {
                        temp[i] = dataBuffer[i];
                    }
                    
                    dataBuffer = new uint8_t[bufferIndex + 1];
                    for(size_t i = 0; i < sizeof(dataBuffer); i++)
                    {
                        dataBuffer[i] = temp[i];
                    }

                    dataBuffer[bufferIndex] = 0;*/
                    bufferIndex++;
                    byteIndex = 0;
                }
            }
        }

        void onEdgeChange()
        {
            bool rising = pinReadFast(input_pin) == HIGH;

            endTime = micros(); // dès qu'on a un changement
            if(!isReceiving)
                isReceiving = true;
            
            if(isFirstEdge)
            {
                isFirstEdge = false;
                addBitToBuffer(rising);
                signalStart = micros();
            }
            else {

                if(periodWidth == 0)
                {
                    periodWidth = signalTime();
                    addBitToBuffer(rising);
                }
                else
                {
                    // si on est à la bonne place dans la période
                    if ((signalTime() - periodWidth / 3) % periodWidth > 2 * periodWidth / 3)
                    {
                        addBitToBuffer(rising);
                    }
                }
            }
        }
    }

    // FUNCTIONS
    void init(void (*onBufferFilledCallback)(uint8_t*))
	{
        delayMicroseconds(2000000);
        WITH_LOCK(Serial)
		{
			Serial.printlnf("Manchester init");
		}
		pinMode(output_pin, OUTPUT);
		pinMode(input_pin, INPUT);
        Receiver::init(onBufferFilledCallback);
        Sender::init();
	}

    void send(uint8_t* data)
    {
        Sender::send(data);
    }

}