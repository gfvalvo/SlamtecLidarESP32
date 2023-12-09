/*
 * net_seial.h
 *
 *  Created on: Nov 12, 2023
 *      Author: GFV
 */

#ifndef SDK_SRC_ARCH_ESP32_NET_SEIAL_H_
#define SDK_SRC_ARCH_ESP32_NET_SEIAL_H_

#include <Arduino.h>
#include "../../hal/abs_rxtx.h"

template<typename T, size_t LOG_SIZE>
class Fifo {
public:
	Fifo() {
		fifoArray = new (std::nothrow) T[fifoSize];
		assert(fifoArray != nullptr);
	}

	~Fifo() {
		delete[] fifoArray;
	}

	bool isFull() const {
		return full;
	}

	bool isEmpty() const {
		return empty;
	}

	void emptyFifo() {
		empty = true;
		full = false;
		writePointer = 0;
		readPointer = 0;
	}

	void writeValue(const T &value) {
		assert(!full);
		fifoArray[writePointer++] = value;
		writePointer &= sizeMask;
		empty = false;
		if (writePointer == readPointer) {
			full = true;
		}
	}

	const T& readValue() {
		assert(!empty);
		size_t ptr = readPointer++;
		readPointer &= sizeMask;
		full = false;
		if (readPointer == writePointer) {
			empty = true;
		}
		return fifoArray[ptr];
	}

	size_t getNumValues() const {
		size_t count;
		if (full) {
			count = fifoSize;
		} else {
			count = (writePointer - readPointer) & sizeMask;
		}
		return count;
	}

private:
	const size_t fifoSize = 1UL << LOG_SIZE;
	const size_t sizeMask = fifoSize - 1;
	bool empty = true;
	bool full = false;
	size_t writePointer = 0;
	size_t readPointer = 0;
	T *fifoArray;
};

namespace rp {
namespace arch {
namespace net {

class raw_serial: public rp::hal::serial_rxtx {
public:
	raw_serial(Stream &str);

	virtual bool bind(const char *portname, uint32_t baudrate, uint32_t flags = 0) {
		return true;
	}

	virtual bool open() {
		return true;
	}

	virtual void close() {
	}

	virtual void flush(_u32 flags) {
		ioStream.flush();
		byteFifo.emptyFifo();
		while(ioStream.read() >= 0) {
		}
	}

	virtual int waitfordata(size_t data_count, _u32 timeout = -1, size_t *returned_size = NULL);

	virtual int senddata(const unsigned char *data, size_t size) {
		return ioStream.write(data, size);
	}

	virtual int recvdata(unsigned char *data, size_t size);

	virtual int waitforsent(_u32 timeout = -1, size_t *returned_size = NULL) {
		return 0;
	}

	virtual int waitforrecv(_u32 timeout = -1, size_t *returned_size = NULL) {
		return 0;
	}

	virtual size_t rxqueue_count() {
		return 0;
	}

	virtual void setDTR() {
	}

	virtual void clearDTR() {
	}

private:
	Stream &ioStream;
	Fifo<uint8_t, 9> byteFifo;
};

}
}
}

#endif /* SDK_SRC_ARCH_ESP32_NET_SEIAL_H_ */
