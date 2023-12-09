/*
 * timer.cpp
 *
 *  Created on: Nov 9, 2023
 *      Author: TR001221
 */

#include "arch_esp32.h"

namespace rp {
namespace arch {

_u64 rp_getus() {
	return esp_timer_get_time();
}

_u32 rp_getms() {
	return esp_timer_get_time() / 1000;
}

}
}
