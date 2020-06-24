#include "manchester.h"

SYSTEM_THREAD(ENABLED);

namespace Manchester
{
    // PARAMETERS
    const int output_pin = D6;
    const int input_pin = D5;
    const size_t MAX_BUFFER_SIZE = 80;

    namespace Sender
    {
        // PARAMETERS
        system_tick_t lastTick = 0;
        const unsigned long SENDING_DELAY = 100;
        const int HALF_PERIOD_TICKS = 100;
        volatile int periodTicksCounter = 0;
        unsigned long delayBetweenSends = 0;

        volatile bool isSending, isMidBit, isReady = false;

        volatile size_t byteIndex, bufferSize;
        volatile uint8_t * dataBuffer;

        void senderThread();
        Thread sendThread("senderThread", senderThread);

        void reset()
        {
            byteIndex = 0;
            isSending = false;
            bufferSize = 0;
            for(size_t i = 0; i < MAX_BUFFER_SIZE; i++)
            {
                dataBuffer[i] = 0;
            }
        }

        void init()
        {
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Sender init");
            }
            dataBuffer = new uint8_t[MAX_BUFFER_SIZE];
            reset();
            isReady = true;
        }

        void onSystemTick()
        {
            if(periodTicksCounter++ % HALF_PERIOD_TICKS != 0)
                return;

            if(bufferSize > 0)
            { 
                uint8_t currentData = dataBuffer[0] >> byteIndex;
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
                    //on shift tout en avant
                    for (size_t i = 0; i < MAX_BUFFER_SIZE - 1; i++)
                    {
                        dataBuffer[i] = dataBuffer[i+1];
                    }
                    bufferSize--;
                    byteIndex = 0;
                }
            }
            /*else
            {
                os_thread_yield();
            }*/
            
            isMidBit = !isMidBit;
        }

        void send(uint8_t data)
        {
            dataBuffer[bufferSize++] = data;
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
                /*WITH_LOCK(Serial)
                {
                    Serial.printlnf("Awaiting send data buffer init...");
                }*/
			    waitUntil([]() { return bufferSize > 0; });
                WITH_LOCK(Serial)
                {
                    Serial.printlnf("Starting to send data buffer starting with %02X", dataBuffer[0]);
                }

                periodTicksCounter = 0;
                unsigned long d = micros() - delayBetweenSends;
                if(delayBetweenSends > 0)
                    periodTicksCounter = (d + 500) / 1000;
                WITH_LOCK(Serial)
                {
                    Serial.printlnf("Starting period ticks = %d, delay = %d", periodTicksCounter, d);
                }
                attachInterruptDirect(SysTick_IRQn, onSystemTick);
			    waitUntil([]() { return bufferSize == 0; });
                detachInterruptDirect(SysTick_IRQn);
                delayBetweenSends = micros();
                os_thread_delay_until(&lastTick, SENDING_DELAY);
            } 
            
        }
    }

    namespace Receiver
    {
        // PARAMETERS
	    void (*onByteReceived)(const uint8_t&);
        volatile bool isFirstEdge;
        volatile bool receivingByte;
        volatile unsigned long signalStart, periodWidth;
        
        volatile size_t byteIndex;
        uint8_t byteBuffer, byteReceived;

        volatile unsigned long endTime;

        void onEdgeChange();
        void reset();

        void receiverThread();
        Thread receiveThread("receiverThread", receiverThread);

        void init(void (*onByteReceivedCallback)(const uint8_t&))
        {
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Receiver init");
            }
            onByteReceived = onByteReceivedCallback;

            reset();
            periodWidth = 0;
            isFirstEdge = true;
            receivingByte = false;
            pinResetFast(input_pin);
            attachInterrupt(input_pin, onEdgeChange, CHANGE);
        }


        inline unsigned long signalTime()
        {
            return micros() - signalStart;
        }

        void reset()
        {
            byteBuffer = 0;
            byteIndex = 0;

            //isReceiving = false;
            //isFirstEdge = true;

            endTime = micros();
        }

        void receiverThread()
        {
            while(true)
            {
                waitUntil([]() {  return receivingByte; });
                receivingByte = false;
                os_thread_yield();
                /*WITH_LOCK(Serial)
                {
                    Serial.printlnf("Byte received. Calling callback...");
                }
                onByteReceived(byteReceived);
                byteReceived = 0;
                WITH_LOCK(Serial)
                {
                    Serial.printlnf("Awaiting reception");
                }
                waitUntil([]() {  return isReceiving; });
                WITH_LOCK(Serial)
                {
                    Serial.printlnf("Started receiving");
                }
                waitUntil([]() { return micros() - endTime > periodWidth * 15;  });
                isReceiving = false;
                WITH_LOCK(Serial)
                {
                    Serial.printlnf("Finished receiving");
                }
                reset();*/
            }
        }

        void addBitToBuffer(bool bit)
        {
            WITH_LOCK(Serial)
            {
                Serial.printlnf(bit ? "1" : "0");
            }
            ATOMIC_BLOCK()
            {	
                byteBuffer |= (bit ? 1 : 0) << byteIndex++;

                if(byteIndex >= 8)
                {
                    receivingByte = true;
                    onByteReceived(byteBuffer);
                    byteBuffer = 0;
                    byteIndex = 0;
                }
            }
        }

        inline bool isTransitionOnClock()
        {
            return (signalTime() - periodWidth / 3) % periodWidth > 2 * periodWidth / 3;
        }

        void onEdgeChange()
        {
            bool rising = pinReadFast(input_pin) == HIGH;

            endTime = micros(); // dès qu'on a un changement

            // si on a un délai entre deux changements, on entre en mode calibrate
            // en mode calibrate: on record 3 transitions
            // pour chaque on ajoute un 1 ou un zéro à un buffer_calibration
            // si trois ne sont pas synchros sur la clock d'ici le prochain delay
            // on a une erreur donc 
            // - on reset signal start au temps du first edge de la calibration
            // - on ajoute au buffer les transitions du buffer_calibration:
            // ex: si on recoit 01 10 01 10 01
            
            WITH_LOCK(Serial)
            {
                Serial.print(".");
            }
            
            if(isFirstEdge)
            {
                isFirstEdge = false;
                addBitToBuffer(rising);
                signalStart = micros();
            }
            else {

                if (periodWidth == 0)
                {
                    periodWidth = signalTime();
                    /*WITH_LOCK(Serial)
                    {
                        Serial.printlnf("Period width: %d", periodWidth);
                    }*/
                    addBitToBuffer(rising);
                    // faire une moyenne sur toutes les données du préambule?
                }
                else
                {
                    // si on est à la bonne place dans la période
                    if (isTransitionOnClock())
                    {
                        addBitToBuffer(rising);
                    }
                }
            }
        }
    }


    // FUNCTIONS
    void init(void (*onBufferFilledCallback)(const uint8_t&))
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

    void send(const uint8_t& data)
    {
        Sender::send(data);
    }

}