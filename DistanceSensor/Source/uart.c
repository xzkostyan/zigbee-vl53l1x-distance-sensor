#include "hal_uart.h"
#include "osal.h"
#include "uart.h"


const uint8 SELECTED_PORT = HAL_UART_PORT_0;


static void uart0RxCb(uint8 port, uint8 event)
{
  uint8  ch;
  while (Hal_UART_RxBufLen(port))
  {
    // Read one byte from UART to ch
    HalUARTRead(port, &ch, 1);
  }
}

// HAL_UART_PORT_0
// 1. P0_2 as RX
// 2. P0_3 as TX
void initUART0(void)
{
  halUARTCfg_t uartConfig;
  uartConfig.configured           = TRUE;
  uartConfig.baudRate             = HAL_UART_BR_115200;
  uartConfig.flowControl          = FALSE;
  uartConfig.flowControlThreshold = 48;
  uartConfig.rx.maxBufSize        = 128;
  uartConfig.tx.maxBufSize        = 128;
  uartConfig.idleTimeout          = 6; 
  uartConfig.intEnable            = TRUE;            
  uartConfig.callBackFunc         = uart0RxCb;

  HalUARTOpen(SELECTED_PORT, &uartConfig);
}

uint16 writeUART0(char *pBuffer)
{
  return HalUARTWrite(SELECTED_PORT, (uint8 *)pBuffer, osal_strlen(pBuffer));
}