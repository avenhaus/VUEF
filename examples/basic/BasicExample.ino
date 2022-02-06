#include <VUEF.h>

size_t counter = 0;
StateUInt32 stateTestCounter(FST("Counter"), counter, FST("Counter"), 0, nullptr, &counter);

RegGroup configGroupTest(FST("Test"));
char testText[64] = "Bar";
StateStr stateTestString(FST("Foo"), testText, FST("Test State String"), 0, &configGroupTest);
StateUInt32 stateTestInt1(FST("Millis"), 0, FST("Millis"), 0, &configGroupTest);



void setup() {
    vuefInit();
}


void loop() {
    uint32_t now = millis();
    vuefRun(now);

    static uint32_t nextUpdate = 0;
    if (now > nextUpdate) {
        nextUpdate = now + 5000;
        stateTestInt1.set(now);
        counter++;
        sprintf(testText, "Hello World %d", counter);
    }
}