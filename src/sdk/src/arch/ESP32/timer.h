/*
 * timer.h
 *
 *  Created on: Nov 9, 2023
 *      Author: TR001221
 */

#ifndef SDK_SRC_ARCH_ESP32_TIMER_H_
#define SDK_SRC_ARCH_ESP32_TIMER_H_

#include "../../hal/types.h"

namespace rp {
namespace arch {

_u64 rp_getus();
_u32 rp_getms();

}
}

#define getms() rp::arch::rp_getms()

#endif /* SDK_SRC_ARCH_ESP32_TIMER_H_ */
