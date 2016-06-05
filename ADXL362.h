#ifndef __ADXL362_H
#define __ADXL362_H

// Connect the ADXL362 breakout (when using with SPI instead of SPI1)
// VIN: 3V3
// GND: GND
// SCL: A3 (SCK)
// SDA: A5 (MOSI)
// SDO: A4 (MISO)
// CS: A2 (SS)
// INT2: no connection
// INT1: no connection

class ADXL362Data; // forward declaration

class ADXL362 {
public:
	ADXL362(SPIClass &spi, int ss = A2);
	virtual ~ADXL362();

	void softReset();

	void setMeasureMode(bool enabled = true);

	void readXYZT(int &x, int &y, int &z, int &t);

	uint8_t readStatus();
	uint16_t readNumFifoEntries();

	void readFifoAsync(ADXL362Data *data);
	static void readFifoCallbackInternal(void);

	// samples 0-511, fifoMode = FIFO_DISABLED, etc.
	void writeFifoControl(uint16_t samples, bool storeTemp, uint8_t fifoMode);

	void writeFilterControl(uint8_t range, bool halfBW, bool extSample, uint8_t odr);

	uint8_t readRegister8(uint8_t addr);
	uint16_t readRegister16(uint8_t addr);

	void writeRegister8(uint8_t addr, uint8_t value);

	// number of bytes for XYZ or XYZT
	size_t getEntrySetSize() { return storeTemp ? 8 : 6; };

	bool getIsBusy() { return busy; };
	void beginTransaction();
	void endTransaction();

	void syncTransaction(void *req, void *resp, size_t len);

	// Command bytes
	const uint8_t CMD_WRITE_REGISTER = 0x0a;
	const uint8_t CMD_READ_REGISTER = 0x0b;
	const uint8_t CMD_READ_FIFO = 0x0d;

	// Registers
	const uint8_t REG_DEVID_AD = 0x00;
	const uint8_t REG_DEVID_MST = 0x01;
	const uint8_t REG_STATUS = 0x0b;
	const uint8_t REG_FIFO_ENTRIES_L = 0x0c;
	const uint8_t REG_FIFO_ENTRIES_H = 0x0d;
	const uint8_t REG_XDATA_L = 0x0e;
	const uint8_t REG_XDATA_H = 0x0f;
	const uint8_t REG_YDATA_L = 0x10;
	const uint8_t REG_YDATA_H = 0x11;
	const uint8_t REG_ZDATA_L = 0x12;
	const uint8_t REG_ZDATA_H = 0x13;
	const uint8_t REG_TDATA_L = 0x14;
	const uint8_t REG_TDATA_H = 0x15;
	const uint8_t REG_SOFT_RESET = 0x1f;
	const uint8_t REG_FIFO_CONTROL = 0x28;
	const uint8_t REG_FIFO_SAMPLES = 0x29;
	const uint8_t REG_FIFO_INTMAP1 = 0x2a;
	const uint8_t REG_FIFO_INTMAP2 = 0x2b;
	const uint8_t REG_FILTER_CTL = 0x2c;
	const uint8_t REG_POWER_CTL = 0x2d;

	// Range in Filter Control Register
	const uint8_t RANGE_2G 	= 0x0;// default
	const uint8_t RANGE_4G 	= 0x1;
	const uint8_t RANGE_8G 	= 0x2;


	// Output Data Rate in Filter Control Register
	const uint8_t ODR_12_5 	= 0x0;
	const uint8_t ODR_25 	= 0x1;
	const uint8_t ODR_50 	= 0x2;
	const uint8_t ODR_100 	= 0x3; // default
	const uint8_t ODR_200 	= 0x4;
	const uint8_t ODR_400 	= 0x5;

	// FIFO mode
	const uint8_t FIFO_DISABLED 	= 0x0;
	const uint8_t FIFO_OLDEST_SAVED = 0x1;
	const uint8_t FIFO_STREAM 		= 0x2;
	const uint8_t FIFO_TRIGGERED 	= 0x3;

private:
	static void syncCallback(void);

	SPIClass &spi; // Typically SPI or SPI1
	int ss;		// SS or /CS chip select pin. Default: A2
	bool storeTemp = false;
	bool busy = false;
};

class ADXL362Data {
public:
	static const size_t BUF_SIZE = 128;
	static const int STATE_FREE = 0;
	static const int STATE_READING_FIFO = 1;
	static const int STATE_READ_COMPLETE = 2;

	ADXL362Data();
	virtual ~ADXL362Data();

	uint8_t buf[BUF_SIZE];
	bool storeTemp;	// false=XYZ, true=XYZT (includes temperature)
	int state = STATE_FREE;
	size_t bytesRead = 0;

};

#endif /* __ADXL362_H */

