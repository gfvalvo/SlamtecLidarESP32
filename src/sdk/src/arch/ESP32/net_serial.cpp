/*
 * net_serial.cpp
 *
 *  Created on: Nov 13, 2023
 *      Author: GFV
 */

#include "arch_esp32.h"
#include "../../hal/types.h"
#include "net_seial.h"

namespace rp {
namespace arch {
namespace net {

raw_serial::raw_serial(Stream &str) :
		ioStream(str) {
}

int raw_serial::waitfordata(size_t data_count, _u32 timeout, size_t *returned_size) {
	int nextByte;
	size_t dummyByteCount = 0;

	if (returned_size == nullptr) {
		returned_size = &dummyByteCount;
	}
	*returned_size = byteFifo.getNumValues();

	uint32_t startMillis = millis();
	while (millis() - startMillis < timeout) {
		while ((nextByte = ioStream.read()) > -1) {
			byteFifo.writeValue(static_cast<uint8_t>(nextByte));
			(*returned_size)++;
		}
		if (*returned_size >= data_count) {
			return ANS_OK;
		}
		//vTaskDelay(2);
	}
	return ANS_TIMEOUT;
}

int raw_serial::recvdata(unsigned char *data, size_t size) {
	size_t byteCount = 0;
	while (byteCount < size) {
		if (byteFifo.isEmpty()) {
			return byteCount;
		}
		*data++ = byteFifo.readValue();
		byteCount++;
	}
	return byteCount;
}

}
}
}

namespace rp {
namespace hal {

serial_rxtx* serial_rxtx::CreateRxTx(Stream &str) {
	return new rp::arch::net::raw_serial(str);
}

}
}

