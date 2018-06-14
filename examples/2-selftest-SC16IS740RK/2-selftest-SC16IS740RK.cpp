#include "SC16IS740RK.h"

// Pick a debug level from one of these two:
// SerialLogHandler logHandler;
SerialLogHandler logHandler; //(LOG_LEVEL_TRACE);

SYSTEM_THREAD(ENABLED);

SC16IS740 extSerial(Wire, 0, D2);

// Connect the Photon TX pin to the SC16IS740 RX pin
// Connect the Photon RX pin to the SC16IS740 TX pin

typedef struct {
	int serial;
	int extSerial;
} OptionsPair;

OptionsPair options[10] = {
	{ SERIAL_8N1, SC16IS740::OPTIONS_8N1 }, // 0
	{ SERIAL_8E1, SC16IS740::OPTIONS_8E1 }, // 1
	{ SERIAL_8O1, SC16IS740::OPTIONS_8O1 }, // 2
	{ SERIAL_8N2, SC16IS740::OPTIONS_8N2 }, // 3
	{ SERIAL_8E2, SC16IS740::OPTIONS_8E2 }, // 4
	{ SERIAL_8O2, SC16IS740::OPTIONS_8O2 }, // 5
	{ SERIAL_7E1, SC16IS740::OPTIONS_7E1 }, // 6
	{ SERIAL_7O1, SC16IS740::OPTIONS_7O1 }, // 7
	{ SERIAL_7E2, SC16IS740::OPTIONS_7E2 }, // 8
	{ SERIAL_7O2, SC16IS740::OPTIONS_7O2 }  // 9
};

int bauds[] = { 1200, 2400, 4800, 9600, 19200, 57600, 115200 };

uint8_t tempBuf[16384];

void runSelfTest();

void setup() {
	Serial.begin(9600);

	delay(5000);

	Serial1.begin(9600);

	extSerial.begin(9600);
}

void loop() {
	runSelfTest();
	delay(15000);
}

bool clearAvailable() {
	while(Serial1.available()) {
		Serial1.read();
	}
	while(extSerial.available()) {
		extSerial.read();
	}
	return true;
}

bool waitForStream(Stream &stream) {
	unsigned long startTime = millis();

	while(!stream.available() && millis() - startTime < 1000) {
		delay(1);
	}
	return stream.available();
}

bool testSimpleReadWrite() {
	bool bResult;

	clearAvailable();

	for(int ch = 0; ch < 256; ch++) {
		extSerial.write(ch);
		bResult = waitForStream(Serial1);
		if (!bResult) {
			Log.error("failed line=%d ch=%d", __LINE__, ch);
			return false;
		}
		int value = Serial1.read();
		if (value != ch) {
			Log.error("failed line=%d ch=%d value=%d", __LINE__, ch, value);
			return false;
		}

		int ch2 = ~ch & 0xff;

		Serial1.write(ch2);
		bResult = waitForStream(extSerial);
		if (!bResult) {
			Log.error("failed line=%d ch=%d", __LINE__, ch);
			return false;
		}
		value = extSerial.read();
		if (value != ch2) {
			Log.error("failed line=%d ch=%d value=%d", __LINE__, ch, value);
			return false;
		}

	}

	Log.info("testSimpleReadWrite passed");

	return true;
}

bool testFifo1() {
	bool bResult;

	clearAvailable();

	int numToTest = 62;

	for(int ch = 0; ch < numToTest; ch++) {
		extSerial.write(ch);
	}

	for(int ch = 0; ch < numToTest; ch++) {
		bResult = waitForStream(Serial1);
		if (!bResult) {
			Log.error("failed line=%d ch=%d", __LINE__, ch);
			return false;
		}
		int value = Serial1.read();
		if (value != ch) {
			Log.error("failed line=%d ch=%d value=%d", __LINE__, ch, value);
			return false;
		}
	}

	for(int ch = 0; ch < numToTest; ch++) {
		Serial1.write(ch);
	}

	for(int ch = 0; ch < numToTest; ch++) {
		bResult = waitForStream(extSerial);
		if (!bResult) {
			Log.error("failed line=%d ch=%d", __LINE__, ch);
			return false;
		}
		int value = extSerial.read();
		if (value != ch) {
			Log.error("failed line=%d ch=%d value=%d", __LINE__, ch, value);
			return false;
		}
	}

	// Success is logged by the caller

	return true;

}


bool testLarge1() {
	srand(0);

	// Random test data
	for(size_t ii = 0; ii < sizeof(tempBuf); ii++) {
		tempBuf[ii] = rand();
	}

	clearAvailable();

	int readIndex = 0;

	for(size_t ii = 0; ii < sizeof(tempBuf); ) {
		if (Serial1.availableForWrite()) {
			Serial1.write(tempBuf[ii]);
			ii++;
		}

		while(extSerial.available()) {
			int c = extSerial.read();
			if (c != tempBuf[readIndex]) {
				Log.error("testLargefailed line=%d ii=%u got=%02x expected=%02x", __LINE__, ii, c, tempBuf[readIndex]);
				return false;
			}
			readIndex++;
		}
	}

	Log.info("testLarge passed line=%d", __LINE__);

	return true;
}



void runSelfTest() {

	testSimpleReadWrite();

	bool bResult = testFifo1();
	if (bResult) {
		Log.info("testFifo1 passed line=%d", __LINE__);
	}

	testLarge1();

	for(int ii = 0; ii < sizeof(bauds)/sizeof(bauds[0]); ii++) {

		for(int jj = 0; jj < sizeof(options)/sizeof(options[0]); jj++) {

			Serial1.begin(bauds[ii], options[jj].serial);
			extSerial.begin(bauds[ii], options[jj].extSerial);

			delay(10);

			bool bResult = testFifo1();
			if (bResult) {
				Log.info("testFifo passed for baud=%d options index=%d", bauds[ii], jj);
			}
			else {
				Log.error("testFifo failed line=%d for baud=%d options index=%d", __LINE__, bauds[ii], jj);
			}
		}
	}

	Serial1.begin(9600);
	extSerial.begin(9600);
	Log.info("runSelfTest completed");

}