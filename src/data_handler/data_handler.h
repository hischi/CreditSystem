#pragma once

#include <Arduino.h>
#include "../util/time_format.h"
#include "business_model.h"

void dh_init();

/**
 * Access to member data 
 **/
bool dh_is_authorised(uint32_t memberID, uint32_t cardID);
bool dh_is_available(uint32_t memberID, uint16_t itemID);
uint32_t dh_calculate_discount(uint32_t memberID, uint32_t cost);
sMember* dh_get_member_from_idx(uint32_t idx);

/**
 * Access to transaction data 
 **/
bool dh_create_transaction(uint32_t memberID, uint8_t itemID, uint32_t cost, uint16_t discount);
bool dh_approve_transaction();
bool dh_complete_transaction();
bool dh_cancle_transaction();
bool dh_timeout_transaction();

/**
 * Access to log
 **/
bool dh_store_log(char *log_buffer, uint32_t size);

bool dh_prepare_log();
