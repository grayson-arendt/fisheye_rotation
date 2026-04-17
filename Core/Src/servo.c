#include "servo.h"
#include <string.h>

#define SERVO_TIMEOUT_MS 50
#define MAX_PACKET_SIZE 64
#define BROADCAST_ID 0xFE

// Instructions
#define INST_READ 0x02
#define INST_WRITE 0x03

// SCS0009 Registers
#define SCS0009_TORQUE_ENABLE 40
#define SCS0009_GOAL_POSITION_L 42
#define SCS0009_GOAL_POSITION_H 43
#define SCS0009_GOAL_TIME_L 44
#define SCS0009_GOAL_TIME_H 45
#define SCS0009_GOAL_SPEED_L 46
#define SCS0009_GOAL_SPEED_H 47
#define SCS0009_PRESENT_POSITION_L 56
#define SCS0009_PRESENT_POSITION_H 57

// External UART handle
extern UART_HandleTypeDef huart1;

// Store servo ID after initialization
static uint8_t servo_id = -1;

static servo_status_t servo_write_packet(uint8_t id, uint8_t addr,
                                         uint8_t *data, uint8_t len,
                                         uint8_t cmd)
{
  // Check length
  if (len > MAX_PACKET_SIZE)
  {
    return SERVO_TX_ERROR;
  }

  uint8_t msg_len = 3 + (data ? len : 0);
  uint8_t checksum = id + msg_len + cmd + addr;

  uint8_t buf[6 + MAX_PACKET_SIZE] = {0};

  buf[0] = 0xFF; // Header
  buf[1] = 0xFF; // Header
  buf[2] = id;
  buf[3] = msg_len;
  buf[4] = cmd;
  buf[5] = addr;

  // Copy data and calculate checksum
  if (data)
  {
    memcpy(&buf[6], data, len);
    for (uint8_t i = 0; i < len; ++i)
    {
      checksum += data[i];
    }
  }

  buf[6 + len] = ~checksum; // Invert bits for checksum

  // Clear any pending error flags and flush RX buffer
  __HAL_UART_CLEAR_FLAG(&huart1, UART_CLEAR_PEF | UART_CLEAR_FEF |
                                     UART_CLEAR_NEF | UART_CLEAR_OREF);
  __HAL_UART_SEND_REQ(&huart1, UART_RXDATA_FLUSH_REQUEST);

  // Enable transmitter
  HAL_HalfDuplex_EnableTransmitter(&huart1);

  // Transmit TX buffer via TXE polling
  uint8_t tx_len = 6 + len + 1;
  uint32_t tx_timeout = HAL_GetTick() + SERVO_TIMEOUT_MS;
  for (uint8_t i = 0; i < tx_len; i++)
  {
    while (!__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE))
    {
      if (HAL_GetTick() >= tx_timeout)
      {
        return SERVO_TX_ERROR;
      }
    }
    huart1.Instance->TDR = buf[i];
  }
  // Wait for last byte to finish transmitting
  while (!__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC))
  {
    if (HAL_GetTick() >= tx_timeout)
    {
      return SERVO_TX_ERROR;
    }
  }

  // Enable receiver
  HAL_HalfDuplex_EnableReceiver(&huart1);

  return SERVO_OK;
}

static servo_status_t servo_read_bytes(uint8_t *buf, uint8_t len)
{
  // Clear any pending error flags
  __HAL_UART_CLEAR_FLAG(&huart1, UART_CLEAR_PEF | UART_CLEAR_FEF |
                                     UART_CLEAR_NEF | UART_CLEAR_OREF);

  uint32_t rx_timeout = HAL_GetTick() + SERVO_TIMEOUT_MS;
  for (uint8_t i = 0; i < len; i++)
  {
    // Poll RXNE until a byte arrives or timeout
    while (!__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE))
    {
      if (HAL_GetTick() >= rx_timeout)
      {
        return SERVO_RX_ERROR;
      }
    }
    buf[i] = (uint8_t)(huart1.Instance->RDR & 0xFF);
  }
  return SERVO_OK;
}

static servo_status_t servo_read_word(uint8_t id, uint8_t addr,
                                      uint16_t *data)
{
  // Send out INST_READ
  uint8_t read_len = 2; // We want to read 2 bytes (position is 16 bits)
  if (servo_write_packet(id, addr, &read_len, 1, INST_READ) != SERVO_OK)
  {
    return SERVO_TX_ERROR;
  }

  // Read full response: 0xFF 0xFF ID LEN STATUS DATA_L DATA_H CHECKSUM
  uint8_t buf[8];
  if (servo_read_bytes(buf, 8) != SERVO_OK)
  {
    return SERVO_RX_ERROR;
  }

  // Validate header
  if (buf[0] != 0xFF || buf[1] != 0xFF)
  {
    return SERVO_NO_REPLY;
  }

  // Validate ID
  if (buf[2] != id && id != BROADCAST_ID)
  {
    return SERVO_SLAVE_ID;
  }

  // Validate length (STATUS + DATA_L + DATA_H + CHECKSUM = 4 bytes)
  if (buf[3] != 4)
  {
    return SERVO_BUFF_LEN;
  }

  // Verify checksum
  uint8_t calc_checksum = ~(buf[2] + buf[3] + buf[4] + buf[5] + buf[6]);
  if (buf[7] != calc_checksum)
  {
    return SERVO_CRC_CMP;
  }

  // Combine bytes to form the 16-bit data value
  *data = (buf[5] << 8) | buf[6];
  return SERVO_OK;
}

servo_status_t servo_init(uint8_t id)
{
  servo_id = id;
  return SERVO_OK;
}

servo_status_t servo_set_position(uint16_t position, uint16_t time,
                                  uint16_t speed)
{
  // Write all 6 bytes: position (2), time (2), speed (2)
  uint8_t buf[6];

  // Position
  buf[0] = (position >> 8) & 0xFF;
  buf[1] = position & 0xFF;

  // Time
  buf[2] = (time >> 8) & 0xFF;
  buf[3] = time & 0xFF;

  // Speed
  buf[4] = (speed >> 8) & 0xFF;
  buf[5] = speed & 0xFF;

  return servo_write_packet(servo_id, SCS0009_GOAL_POSITION_L, buf, 6,
                            INST_WRITE);
}

servo_status_t servo_read_position(uint16_t *position)
{
  return servo_read_word(servo_id, SCS0009_PRESENT_POSITION_L, position);
}
