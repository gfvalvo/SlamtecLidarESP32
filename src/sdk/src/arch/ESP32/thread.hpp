/*
 * thread.hpp
 *
 *  Created on: Nov 9, 2023
 *      Author: TR001221
 */

#ifndef SDK_SRC_ARCH_ESP32_THREAD_HPP_
#define SDK_SRC_ARCH_ESP32_THREAD_HPP_

#include "arch_esp32.h"

namespace rp {
namespace hal {

SemaphoreHandle_t Thread::threadControlMutex = xSemaphoreCreateMutex();
std::vector<Thread::ThreadControlBlock> Thread::taskControlBlockVector;
uint32_t Thread::nextThreadID = 1;

Thread Thread::create(const threadFunction &proc, void *data) {
	Thread newborn;

	xSemaphoreTake(threadControlMutex, portMAX_DELAY);
	QueueHandle_t returnValueQueue = xQueueCreate(1, sizeof(_word_size_t));
	assert(returnValueQueue != nullptr);

	//uint32_t *newTaskId = new uint32_t {nextThreadID++};
	//newborn.threadId = *newTaskId;
	newborn.threadId = nextThreadID++;
	if (nextThreadID == 0) {
		nextThreadID = 1;
	}

	threadFunction *functPtr = new threadFunction(proc);
	taskControlBlockVector.emplace_back(newborn.threadId, data, functPtr, returnValueQueue, nullptr);
	xSemaphoreGive(threadControlMutex);

	//BaseType_t returnCode = xTaskCreatePinnedToCore(launchThread, "Thread", 5000, newTaskId, 6, NULL, CONFIG_ARDUINO_RUNNING_CORE);
	BaseType_t returnCode = xTaskCreatePinnedToCore(launchThread, "Thread", 5000, reinterpret_cast<void*>(newborn.threadId), 6, NULL, CONFIG_ARDUINO_RUNNING_CORE);

	assert(returnCode == pdPASS);
	return newborn;
}

u_result Thread::join(unsigned long timeout) {
	xSemaphoreTake(threadControlMutex, portMAX_DELAY);
	if (threadId == 0) {
		xSemaphoreGive(threadControlMutex);
		return 0;
	}

	uint32_t id = threadId;
	auto predicate = [id](ThreadControlBlock &block) {
		return (id == block.threadId);
	};
	auto itr = std::find_if(taskControlBlockVector.begin(), taskControlBlockVector.end(), predicate);
	if (itr == taskControlBlockVector.end()) {
		xSemaphoreGive(threadControlMutex);
		return 0;
	}

	QueueHandle_t returnValueQueue = itr->returnValueQueue;
	xSemaphoreGive(threadControlMutex);
	_word_size_t returnValue;
	xQueueReceive(returnValueQueue, &returnValue, timeout);
	kill();
	return 0;
}

u_result Thread::terminate() {
	if (threadId == 0) {
		return 0;
	}
	kill();
	threadId = 0;
	return 0;
}

void Thread::launchThread(void *pvParameters) {
	uint32_t id = reinterpret_cast<uint32_t>(pvParameters);

	auto predicate = [id](ThreadControlBlock &block) {
		return (id == block.threadId);
	};

	xSemaphoreTake(threadControlMutex, portMAX_DELAY);

	auto itr = std::find_if(taskControlBlockVector.begin(), taskControlBlockVector.end(), predicate);
	if (itr != taskControlBlockVector.end()) {
		threadFunction *threadFunction = itr->functPtr;
		void *param = itr->param;
		itr->taskHandle = xTaskGetCurrentTaskHandle();
		xSemaphoreGive(threadControlMutex);

		_word_size_t returnValue = (*threadFunction)();
		delete threadFunction;

		xSemaphoreTake(threadControlMutex, portMAX_DELAY);
		itr = std::find_if(taskControlBlockVector.begin(), taskControlBlockVector.end(), predicate);
		if (itr != taskControlBlockVector.end()) {
			itr->functPtr = nullptr;
			itr->taskHandle = nullptr;
			xQueueOverwrite(itr->returnValueQueue, &returnValue);
		} else {
			log_e("Didn't find control block");
		}
		xSemaphoreGive(threadControlMutex);
	} else {
		log_e("Didn't find control block");
		xSemaphoreGive(threadControlMutex);
	}
	vTaskDelete(NULL);
}

Thread::~Thread() {
	kill();
}

void Thread::kill() {
	xSemaphoreTake(threadControlMutex, portMAX_DELAY);
	if (threadId == 0) {
		xSemaphoreGive(threadControlMutex);
		return;
	}

	uint32_t id = threadId;
	auto predicate = [id](ThreadControlBlock &block) {
		return (id == block.threadId);
	};

	auto itr = std::find_if(taskControlBlockVector.begin(), taskControlBlockVector.end(), predicate);
	if (itr == taskControlBlockVector.end()) {
		xSemaphoreGive(threadControlMutex);
		return;
	}
	if (itr->taskHandle != nullptr) {
		vTaskDelete(itr->taskHandle);
	}
	if (itr->functPtr != nullptr) {
		delete itr->functPtr;
	}
	if (itr->returnValueQueue != nullptr) {
		vQueueDelete(itr->returnValueQueue);
	}

	taskControlBlockVector.erase(itr);
	xSemaphoreGive(threadControlMutex);
	return;
}

}
}

#endif /* SDK_SRC_ARCH_ESP32_THREAD_HPP_ */
