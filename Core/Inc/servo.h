#ifndef __SERVO_H
#define __SERVO_H

#include "stm32g0xx_hal.h"
#include <stdint.h>

typedef enum
{
    SERVO_OK = 0,
    SERVO_NO_REPLY = 1,
    SERVO_CRC_CMP = 2,
    SERVO_SLAVE_ID = 3,
    SERVO_BUFF_LEN = 4,
    SERVO_TX_ERROR = 5,
    SERVO_RX_ERROR = 6,
} servo_status_t;

servo_status_t servo_init(uint8_t id);
servo_status_t servo_set_position(uint16_t position, uint16_t time, uint16_t speed);
servo_status_t servo_read_position(uint16_t *position);

#endif // __SERVO_H