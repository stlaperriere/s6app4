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
        const int HALF_PERIOD_TICKS = 5;
        const unsigned long SENDING_DELAY = 30 * HALF_PERIOD_TICKS;
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


        volatile bool readyToStop = false;
        void onSystemTick()
        {
            if(periodTicksCounter++ % HALF_PERIOD_TICKS != 0)
                return;

            if (bufferSize == 0 && !isMidBit)
            {
                pinResetFast(output_pin);
                readyToStop = true;
            }

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
                /*unsigned long d = micros() - delayBetweenSends;
                if(delayBetweenSends > 0)
                    periodTicksCounter = (d + 500) / 1000;
                WITH_LOCK(Serial)
                {
                    Serial.printlnf("Starting period ticks = %d, delay = %d", periodTicksCounter, d);
                }*/
                readyToStop = false;
                attachInterruptDirect(SysTick_IRQn, onSystemTick);
			    waitUntil([]() { return readyToStop; });
                detachInterruptDirect(SysTick_IRQn);
                //delayBetweenSends = micros();
                os_thread_delay_until(&lastTick, SENDING_DELAY);
            } 
            
        }
    }

    namespace Receiver
    {
        // PARAMETERS
	    void (*onByteReceived)(const uint8_t&);
        volatile bool isFirstEdge;
        volatile unsigned long signalStart, periodWidth;
        
        volatile size_t byteIndex;
        uint8_t byteBuffer, byteReceived;

        volatile unsigned long endTime;

        void onEdgeChange();
        void reset();

        struct Transition
        {
            unsigned long delay;
            bool rising;
        };

        volatile Transition lastTransition;
        volatile unsigned long timeSinceLastTransition = 0;
        bool firstTransition = true;
        volatile bool lastTransitionWasValid = false;
        volatile bool isReceiving = false;

        void receiverThread();
        Thread receiveThread("receiverThread", receiverThread);

        int pinState, lastPinState;
        bool pinChanged = false;
        unsigned long lastPinChange = 0;


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

            firstTransition = true;
            lastTransition.delay = 0;
            isReceiving = false;
            lastTransitionWasValid = false;
        
            endTime = micros();
            timeSinceLastTransition = 0;
            lastPinState = pinState == HIGH ? LOW : HIGH;
        }

        void receiverThread()
        {
            while(true)
            {
                waitUntil([]() { return isReceiving;});
                pinState = pinReadFast(input_pin);
                if(lastPinState != pinState)
                {
                    lastPinChange = micros();
                }

                ATOMIC_BLOCK() 
                {
                    if(micros() - lastPinChange > periodWidth * 5 && periodWidth > 0)
                    {
                        reset();
                        /*
                        WITH_LOCK(Serial)
                        {
                            Serial.printlnf("RESET");
                        }*/
                    }
                }
                lastPinState = pinState;
                os_thread_yield();
            }
        }

        void addBitToByteBuffer(bool bit)
        {
            /*WITH_LOCK(Serial)
            {
                Serial.printlnf(bit ? "+=1" : "+=0");
            }*/
            ATOMIC_BLOCK()
            {	
                byteBuffer |= (bit ? 1 : 0) << byteIndex++;

                if(byteIndex >= 8)
                {
                    onByteReceived(byteBuffer);
                    byteBuffer = 0;
                    byteIndex = 0;
                }
            }
        }

        void addTransitionToBuffer(bool rising, unsigned long delay)
        {
            /*WITH_LOCK(Serial)
            {
                Serial.printlnf(rising ? "01" : "10");
                //Serial.printlnf("delay of %d", delay);
            }*/
            ATOMIC_BLOCK()
            {	
                // ON POSE QUE LE SIGNAL REDEVIENT ZÉRO APRÈS ENVOI!!!!!!!!!
                // delay de 5 p pour set à zéro? (delay de chaque bord
                // ou on remet à zéro sur un fullbit?) <=========
                if (firstTransition && lastTransition.delay != 0)
                {
                    // on est sur un midbit SI la première transition a un delai de p
                    if (delay > 2 * periodWidth / 3)
                    {
                        addBitToByteBuffer(lastTransition.rising);
                    }
                    addBitToByteBuffer(rising);
                    lastTransitionWasValid = true;
                    firstTransition = false;
                }
                else 
                {
                    if(lastTransitionWasValid)
                    {
                        if(delay > 2 * periodWidth / 3)
                        {
                            addBitToByteBuffer(rising);
                        }
                        else
                        {
                            lastTransitionWasValid = false;
                        }
                    }
                    else
                    {
                        if(delay < 2 * periodWidth / 3)
                        {
                            addBitToByteBuffer(rising);
                            lastTransitionWasValid = true;
                        }
                    }
                }

                lastTransition.rising = rising;
                lastTransition.delay = delay;
            }
        }

        /*inline bool isTransitionOnClock()
        {
            return (signalTime() - periodWidth / 3) % periodWidth > 2 * periodWidth / 3;
        }*/

        void onEdgeChange()
        {
            bool rising = pinReadFast(input_pin) == HIGH;

            endTime = micros(); // dès qu'on a un changement
            isReceiving = true;
            
            /*WITH_LOCK(Serial)
            {
                Serial.print(".");
            }*/
            
            if(isFirstEdge)
            {
                isFirstEdge = false;
                signalStart = micros();
            }
            else {

                if (periodWidth == 0)
                {
                    periodWidth = signalTime();
                    // faire une moyenne sur toutes les données du préambule?

                    /*WITH_LOCK(Serial)
                    {
                        Serial.printlnf("Period width: %d", periodWidth);
                    }*/
                    //addBitToByteBuffer(rising);
                }

                
                /*else
                {
                    // si on est à la bonne place dans la période
                    if (isTransitionOnClock())
                    {
                        addBitToByteBuffer(rising);
                    }
                }*/
            }

            addTransitionToBuffer(rising, micros() - timeSinceLastTransition);
            timeSinceLastTransition = micros();

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