#ifndef APP_DEVICE_CONFIG_H
#define APP_DEVICE_CONFIG_H

#define DEVICE_ROLE_MASTER  0
#define DEVICE_ROLE_REMOTE  1

#ifndef DEVICE_ROLE
#define DEVICE_ROLE         DEVICE_ROLE_REMOTE
#endif

/* 调试用：手写主设备 MAC 跳过配网。设为 1 启用，并按实际填写 6 字节。 */
#define REMOTE_DEBUG_FIXED_MAC          1
#define REMOTE_DEBUG_FIXED_MAC_B0       0xFF
#define REMOTE_DEBUG_FIXED_MAC_B1       0xFF
#define REMOTE_DEBUG_FIXED_MAC_B2       0xFF
#define REMOTE_DEBUG_FIXED_MAC_B3       0xFF
#define REMOTE_DEBUG_FIXED_MAC_B4       0xFF
#define REMOTE_DEBUG_FIXED_MAC_B5       0xFF

#endif
