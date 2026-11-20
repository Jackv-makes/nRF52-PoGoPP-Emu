#pragma once

#include <stdint.h>

#include "pp_ble_dispatch.h"

enum ConnectionCoordinatorAuthResult {
  CONNECTION_COORDINATOR_AUTH_NOT_HANDLED,
  CONNECTION_COORDINATOR_AUTH_HANDLED_NO_STORE,
  CONNECTION_COORDINATOR_AUTH_HANDLED_AND_STORE
};

ConnectionCoordinatorAuthResult ppConnectionCoordinatorHandleAuthWrite(const BleDispatchRefs& refs,
                                                                       const uint8_t* data,
                                                                       uint16_t len);
void ppConnectionCoordinatorHandleServiceDNotifyEnabled(const BleDispatchRefs& refs);
// True while the app is running a full sign-in (0x18) on a board that was
// already paired; see ppConnectionCoordinatorHandleAuthWrite.
bool ppReauthActive();
void ppReauthClear();
bool ppConnectionCoordinatorHandleServiceDWrite(const BleDispatchRefs& refs,
                                                const uint8_t* data,
                                                uint16_t len);
