#include "Particle.h"

#include "ADXL362.h"

// ********* WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING *********
// This is a proof-of-concept only. I'm not entirely sure it's working exactly right,
// though it appears to be close. There are very likely bugs in this code. Use it
// with great caution because it's been barely tested.
// ********* WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING *********

// Data Sheet:
// http://www.analog.com/media/en/technical-documentation/data-sheets/ADXL362.pdf

static Mutex syncCallbackMutex;

static ADXL362Data *readFifoData;
static ADXL362 *readFifoObject;

ADXL362::ADXL362(SPIClass &spi, int ss) : spi(spi), ss(ss) {
	spi.begin(ss);

	// This initialization possibly should go in beginTransaction() for compatibility with SPI
	// bus sharing in different modes, but it seems to require an indeterminate delay to take
	// effect, otherwise the operations fail. Since SPI bus sharing across modes tends not to
	// work with other devices, anyway, I put the code here.
	spi.setBitOrder(MSBFIRST);
	spi.setClockSpeed(8, MHZ);
	spi.setDataMode(SPI_MODE0); // CPHA = 0, CPOL = 0 : MODE = 0
}

ADXL362::~ADXL362() {
}

void ADXL362::softReset() {

	Serial.println("softReset");
	writeRegister8(REG_SOFT_RESET, 'R');
}

void ADXL362::setMeasureMode(bool enabled) {

	uint8_t value = readRegister8(REG_POWER_CTL);

	value &= 0xfc; // remove low 2 bits
	if (enabled) {
		value |= 0x02;
	}

	writeRegister8(REG_POWER_CTL, value);

	//value = readRegister8(REG_POWER_CTL);
	//Serial.printlnf("setMeasureMode check=%02d", value);

}

void ADXL362::readXYZT(int &x, int &y, int &z, int &t) {
	uint8_t req[10], resp[10];

	req[0] = CMD_READ_REGISTER;
	req[1] = REG_XDATA_L;
	for(size_t ii = 2; ii < sizeof(req); ii++) {
		req[ii] = 0;
	}

	syncTransaction(req, resp, sizeof(req));

	x = resp[2] | (((int)resp[3]) << 8);
	y = resp[4] | (((int)resp[5]) << 8);
	z = resp[6] | (((int)resp[7]) << 8);
	t = resp[8] | (((int)resp[9]) << 8);
}

uint8_t ADXL362::readStatus() {
	return readRegister8(REG_STATUS);
}

uint16_t ADXL362::readNumFifoEntries() {
	return readRegister16(REG_FIFO_ENTRIES_L);
}

void ADXL362::readFifoAsync(ADXL362Data *data) {
	readFifoData = data;
	readFifoObject = this;

	size_t entrySetSize = getEntrySetSize();

	size_t maxFullEntries = ADXL362Data::BUF_SIZE / entrySetSize;

	size_t numEntries = ((size_t) readNumFifoEntries() * 2) / entrySetSize;
	if (numEntries > maxFullEntries) {
		numEntries = maxFullEntries;
	}
	data->bytesRead = numEntries * entrySetSize;
	data->state = ADXL362Data::STATE_READING_FIFO;
	data->storeTemp = storeTemp;


	beginTransaction();

	spi.transfer(CMD_READ_FIFO);

	spi.transfer(NULL, data->buf, data->bytesRead, readFifoCallbackInternal);
}

// [static]
void ADXL362::readFifoCallbackInternal(void) {
	readFifoObject->endTransaction();
	readFifoData->state = ADXL362Data::STATE_READ_COMPLETE;
}


void ADXL362::writeFifoControl(uint16_t samples, bool storeTemp, uint8_t fifoMode) {
	uint8_t value = 0;

	this->storeTemp = storeTemp;

	if (samples >= 0x100) {
		value |= 0x08; // AH bit
	}
	if (storeTemp) {
		value |= 0x04; // FIFO_TEMP bit
	}

	value |= (fifoMode & 0x3);

	writeRegister8(REG_FIFO_SAMPLES, samples & 0xff);
	writeRegister8(REG_FIFO_CONTROL, value);
}

void ADXL362::writeFilterControl(uint8_t range, bool halfBW, bool extSample, uint8_t odr) {
	uint8_t value = 0;

	value |= (range & 0x3) << 6;

	if (halfBW) {
		value |= 0x10;
	}
	if (extSample) {
		value |= 0x08;
	}
	value |= (odr & 0x7);

	writeRegister8(REG_FILTER_CTL, value);
}


uint8_t ADXL362::readRegister8(uint8_t addr) {
	uint8_t req[3], resp[3];

	req[0] = CMD_READ_REGISTER;
	req[1] = addr;
	req[2] = 0;

	syncTransaction(req, resp, sizeof(req));

	return resp[2];
}

uint16_t ADXL362::readRegister16(uint8_t addr) {
	uint8_t req[4], resp[4];

	req[0] = CMD_READ_REGISTER;
	req[1] = addr;
	req[2] = req[3] = 0;

	syncTransaction(req, resp, sizeof(req));

	return resp[2] | (((uint16_t)resp[3]) << 8);
}


void ADXL362::writeRegister8(uint8_t addr, uint8_t value) {
	Serial.printlnf("writeRegister addr=%02x value=%02x", addr, value);

	uint8_t req[3], resp[3];

	req[0] = CMD_WRITE_REGISTER;
	req[1] = addr;
	req[2] = value;

	syncTransaction(req, resp, sizeof(req));

}

void ADXL362::beginTransaction() {
	// See note in the constructor for ADXL362
	busy = true;
	digitalWrite(ss, LOW);
}

void ADXL362::endTransaction() {
	digitalWrite(ss, HIGH);
	busy = false;
}

void ADXL362::syncTransaction(void *req, void *resp, size_t len) {
	syncCallbackMutex.lock();

	beginTransaction();

	spi.transfer(req, resp, len, syncCallback);
	syncCallbackMutex.lock();

	endTransaction();

	syncCallbackMutex.unlock();
}

// [static]
void ADXL362::syncCallback(void) {
	syncCallbackMutex.unlock();
}

ADXL362Data::ADXL362Data() {
}

ADXL362Data::~ADXL362Data() {
}




