/* industrial-counter-bricklet
 * Copyright (C) 2017 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication.c: TFP protocol message handling
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "communication.h"

#include "bricklib2/utility/communication_callback.h"
#include "bricklib2/protocols/tfp/tfp.h"

#include "counter.h"

extern Counter counter;

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case FID_GET_COUNTER: return get_counter(message, response);
		case FID_GET_ALL_COUNTER: return get_all_counter(message, response);
		case FID_SET_COUNTER: return set_counter(message);
		case FID_SET_ALL_COUNTER: return set_all_counter(message);
		case FID_GET_SIGNAL_DATA: return get_signal_data(message, response);
		case FID_GET_ALL_SIGNAL_DATA: return get_all_signal_data(message, response);
		case FID_SET_COUNTER_ACTIVE: return set_counter_active(message);
		case FID_SET_ALL_COUNTER_ACTIVE: return set_all_counter_active(message);
		case FID_GET_COUNTER_ACTIVE: return get_counter_active(message, response);
		case FID_GET_ALL_COUNTER_ACTIVE: return get_all_counter_active(message, response);
		case FID_SET_COUNTER_CONFIGURATION: return set_counter_configuration(message);
		case FID_GET_COUNTER_CONFIGURATION: return get_counter_configuration(message, response);
		case FID_SET_ALL_COUNTER_CALLBACK_CONFIGURATION: return set_all_counter_callback_configuration(message);
		case FID_GET_ALL_COUNTER_CALLBACK_CONFIGURATION: return get_all_counter_callback_configuration(message, response);
		case FID_SET_ALL_SIGNAL_DATA_CALLBACK_CONFIGURATION: return set_all_signal_data_callback_configuration(message);
		case FID_GET_ALL_SIGNAL_DATA_CALLBACK_CONFIGURATION: return get_all_signal_data_callback_configuration(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}


BootloaderHandleMessageResponse get_counter(const GetCounter *data, GetCounter_Response *response) {
	if(data->pin > COUNTER_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(GetCounter_Response);
	response->counter = counter_get_count(data->pin);


	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_counter(const GetAllCounter *data, GetAllCounter_Response *response) {
	response->header.length = sizeof(GetAllCounter_Response);
	for(uint8_t pin = 0; pin < COUNTER_NUM; pin++) {
		response->counter[pin] = counter_get_count(pin);
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_counter(const SetCounter *data) {
	if((data->counter > COUNTER_MAX_VALUE) || (data->counter < COUNTER_MIN_VALUE)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if(data->pin > COUNTER_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	uint8_t active = counter_get_active();
	if(active & (1 << data->pin)) {
		counter_set_active((active & (~(1 << data->pin))));
	}
	counter_set_count(data->pin, data->counter);

	if(active & (1 << data->pin)) {
		counter_set_active(active);
	}


	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_all_counter(const SetAllCounter *data) {
	if((data->counter[0] > COUNTER_MAX_VALUE) || (data->counter[0] < COUNTER_MIN_VALUE) ||
	   (data->counter[1] > COUNTER_MAX_VALUE) || (data->counter[1] < COUNTER_MIN_VALUE) ||
	   (data->counter[2] > COUNTER_MAX_VALUE) || (data->counter[2] < COUNTER_MIN_VALUE) ||
	   (data->counter[3] > COUNTER_MAX_VALUE) || (data->counter[3] < COUNTER_MIN_VALUE)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	uint8_t active = counter_get_active();
	counter_set_active(0);

	for(uint8_t pin = 0; pin < COUNTER_NUM; pin++) {
		counter_set_count(pin, data->counter[pin]);
	}

	counter_set_active(active);

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_signal_data(const GetSignalData *data, GetSignalData_Response *response) {
	if(data->pin > COUNTER_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(GetSignalData_Response);

	uint16_t duty_cycle;
	uint64_t period;
	counter_get_duty_cycle_and_period(data->pin, &duty_cycle, &period);
	response->duty_cycle = duty_cycle;
	response->period = period;

	//counter_get_duty_cycle_and_period(data->pin, &(response->duty_cycle), &(response->period));
	response->pin_value = counter_get_pin_value(data->pin);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_signal_data(const GetAllSignalData *data, GetAllSignalData_Response *response) {
	response->header.length = sizeof(GetAllSignalData_Response);
	for(uint8_t pin = 0; pin < COUNTER_NUM; pin++) {
		uint16_t duty_cycle;
		uint64_t period;
		counter_get_duty_cycle_and_period(pin, &duty_cycle, &period);
		response->duty_cycle[pin] = duty_cycle;
		response->period[pin] = period;
		response->pin_value[pin] = counter_get_pin_value(pin);
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_counter_active(const SetCounterActive *data) {
	if(data->pin > COUNTER_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	counter.config_active[data->pin] = data->active;

	uint8_t mask = counter_get_active();
	if(data->active) {
		mask &= ~(1 << data->pin);
	} else {
		mask |= ~(1 << data->pin);
	}

	counter_set_active(mask);

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_all_counter_active(const SetAllCounterActive *data) {
	for(uint8_t pin = 0; pin < COUNTER_NUM; pin++) {
		counter.config_active[pin] = data->active[pin];
	}

	counter_set_active((data->active[0] << 0) | (data->active[1] << 1) | (data->active[2] << 2) | (data->active[3] << 3));

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_counter_active(const GetCounterActive *data, GetCounterActive_Response *response) {
	if(data->pin > COUNTER_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(GetCounterActive_Response);
	response->active = counter_get_active() & (1 << data->pin);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_counter_active(const GetAllCounterActive *data, GetAllCounterActive_Response *response) {
	response->header.length = sizeof(GetAllCounterActive_Response);
	uint8_t mask = counter_get_active();
	for(uint8_t pin = 0; pin < COUNTER_NUM; pin++) {
		response->active[pin] = mask & (1 << pin);
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_counter_configuration(const SetCounterConfiguration *data) {
	if(data->pin > COUNTER_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	counter.config_count_edge[data->pin] = data->count_edge;
	counter.config_count_direction[data->pin] = data->count_direction;
	counter.config_duty_cylce_prescaler[data->pin] = data->duty_cylce_prescaler;
	counter.config_frequency_integration_time[data->pin] = data->frequency_integration_time;
	counter.config_update[data->pin] = true;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_counter_configuration(const GetCounterConfiguration *data, GetCounterConfiguration_Response *response) {
	if(data->pin > COUNTER_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(GetCounterConfiguration_Response);
	response->count_edge = counter.config_count_edge[data->pin];
	response->count_direction = counter.config_count_direction[data->pin];
	response->duty_cylce_prescaler = counter.config_duty_cylce_prescaler[data->pin];
	response->frequency_integration_time = counter.config_frequency_integration_time[data->pin];

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_all_counter_callback_configuration(const SetAllCounterCallbackConfiguration *data) {

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_all_counter_callback_configuration(const GetAllCounterCallbackConfiguration *data, GetAllCounterCallbackConfiguration_Response *response) {
	response->header.length = sizeof(GetAllCounterCallbackConfiguration_Response);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_all_signal_data_callback_configuration(const SetAllSignalDataCallbackConfiguration *data) {

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_all_signal_data_callback_configuration(const GetAllSignalDataCallbackConfiguration *data, GetAllSignalDataCallbackConfiguration_Response *response) {
	response->header.length = sizeof(GetAllSignalDataCallbackConfiguration_Response);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}




bool handle_all_counter_callback(void) {
	static bool is_buffered = false;
	static AllCounter_Callback cb;

	if(!is_buffered) {
		tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(AllCounter_Callback), FID_CALLBACK_ALL_COUNTER);
		// TODO: Implement AllCounter callback handling

		return false;
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(AllCounter_Callback));
		is_buffered = false;
		return true;
	} else {
		is_buffered = true;
	}

	return false;
}

bool handle_all_signal_data_callback(void) {
	static bool is_buffered = false;
	static AllSignalData_Callback cb;

	if(!is_buffered) {
		tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(AllSignalData_Callback), FID_CALLBACK_ALL_SIGNAL_DATA);
		// TODO: Implement AllSignalData callback handling

		return false;
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(AllSignalData_Callback));
		is_buffered = false;
		return true;
	} else {
		is_buffered = true;
	}

	return false;
}

void communication_tick(void) {
	communication_callback_tick();
}

void communication_init(void) {
	communication_callback_init();
}