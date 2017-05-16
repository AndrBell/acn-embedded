/* Copyright (c) 2017 Arrow Electronics, Inc.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Apache License 2.0
 * which accompanies this distribution, and is available at
 * http://apache.org/licenses/LICENSE-2.0
 * Contributors: Arrow Electronics, Inc.
 */

#include "arrow/storage.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <arrow/utf8.h>

#include "StorageEEPROM.h"
#include <debug.h>

int check_mgc() {
  int *c = flash_read();
//  if ( *c != FLASH_MAGIC_NUMBER ) {
//    return 0;
//  }
  return 1;
}

int restore_gateway_info(arrow_gateway_t *gateway) {
  DBG("restore gateway info");
  if (check_mgc()) {
    flash_mem_t *mem = (flash_mem_t *)flash_read();
    if ( utf8check(mem->gateway_hid) && strlen(mem->gateway_hid) > 0 ) {
    	DBG("restore gateway hid %s", mem->gateway_hid);
      arrow_gateway_add_hid(gateway, mem->gateway_hid);
      return 0;
    }
  }
  return -1;
}

void save_gateway_info(const arrow_gateway_t *gateway) {
  DBG("new registration");
  flash_mem_t mem;
  memcpy(&mem, flash_read(), sizeof(flash_mem_t));
  strcpy(mem.gateway_hid, gateway->hid);
  int ret = flash_write(&mem, sizeof(flash_mem_t));
  DBG("flash read %d", ret);
}

int restore_device_info(arrow_device_t *device) {
	DBG("restore device info");
  if (check_mgc()) {
    flash_mem_t *mem = (flash_mem_t *)flash_read();
    if ( !mem ) {
    	DBG("eeprom mem is NULL");
    	return -1;
    }
    if ( !utf8check(mem->device_hid) || strlen(mem->device_hid) == 0 ) {
      return -1;
    }
    DBG("read device hid %s", mem->device_hid);
    arrow_device_set_hid(device, mem->device_hid);
  #if defined(__IBM__)
    if ( !utf8check(mem->device_eid) || strlen(mem->device_eid) == 0 ) {
      return -1;
    }
    arrow_device_set_eid(device, mem->device_eid);
  #endif
    return 0;
  }
  return -1;
}

void save_device_info(arrow_device_t *device) {
  flash_mem_t mem;
  memcpy(&mem, flash_read(), sizeof(flash_mem_t));
  strcpy(mem.device_hid, device->hid);
#if defined(__IBM__)
  strcpy(mem.device_eid, device->eid);
#endif
  flash_write(&mem, sizeof(flash_mem_t));
}

void save_wifi_setting(const char *ssid, const char *pass, int sec) {
  flash_mem_t mem;
  memcpy(&mem, flash_read(), sizeof(flash_mem_t));
  strcpy(mem.ssid, ssid);
  strcpy(mem.pass, pass);
  mem.sec = sec;
  flash_write(&mem, sizeof(flash_mem_t));
}

int restore_wifi_setting(char *ssid, char *pass, int *sec) {
#if defined(DEFAULT_WIFI_SSID)  \
  && defined(DEFAULT_WIFI_PASS) \
  && defined(DEFAULT_WIFI_SEC)
  strcpy(ssid, DEFAULT_WIFI_SSID);
  strcpy(pass, DEFAULT_WIFI_PASS);
  *sec = DEFAULT_WIFI_SEC;
  return 0;
#else
  if (check_mgc()) {
    flash_mem_t *mem = (flash_mem_t *)flash_read();
    strcpy(ssid, mem->ssid);
    strcpy(pass, mem->pass);
    *sec = mem->sec;
    return 0;
  }
#endif
  return -1;
}