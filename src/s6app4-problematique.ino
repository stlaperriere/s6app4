/*
Description : Code de la problematique de l'APP4 S6 info (E20)
Auteur : Samuel Laperriere
Date : 2020/06/19
*/
#include <queue>
#include "s6app4-problematique.h"
#include "frame.h"

SYSTEM_THREAD(ENABLED);

// Pins
int rcvPin = D5;
int sendPin = D6;

// Buffers
std::queue<bool> rcvBuf;
std::queue<bool> sendBuf;

// Threads
Thread thread("FrameParserThread", frameParserFunction);

// Working variables
FrameParser frameParser;


void setup() {
  attachInterrupt(rcvPin, rcvBit0, RISING); // TODO set priority?
  attachInterrupt(rcvPin, rcvBit1, FALLING); // TODO set priority?
  
}

void loop() {

}

void rcvBit0() {
  rcvBuf.push(0);
}

void rcvBit1() {
  rcvBuf.push(1);
}

void frameParserFunction() {
  if (!rcvBuf.empty()) {
    frameParser.push(rcvBuf.pop());
  }
}