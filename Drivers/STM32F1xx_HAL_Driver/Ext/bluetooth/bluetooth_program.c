#include "bluetooth.h"

/* UART handle */
static UART_HandleTypeDef *bt_uart;

/* Receive byte and buffer */
static uint8_t rx_byte;
static uint8_t rx_buffer[BT_BUFFER_SIZE];

/* Buffer indexes and error flag */
static volatile uint16_t head = 0;
static volatile uint16_t tail = 0;
static volatile uint8_t uart_error = 0;
static volatile uint8_t recovery_needed = 0;

/* Get the next buffer position */
static uint16_t NextPosition(uint16_t position)
{
return (position + 1U) % BT_BUFFER_SIZE;
}

/* Initialize Bluetooth reception */
HAL_StatusTypeDef Bluetooth_Init(UART_HandleTypeDef *huart)
{
if (huart == NULL)
return HAL_ERROR;


bt_uart = huart;

head = 0;
tail = 0;
uart_error = 0;
recovery_needed = 0;

return HAL_UART_Receive_IT(bt_uart, &rx_byte, 1);


}

/* Check if the buffer contains data */
uint8_t Bluetooth_Available(void)
{
return (head != tail);
}

/* Read one byte from the buffer */
uint8_t Bluetooth_Read(uint8_t *data)
{
if (data == NULL || head == tail)
return 0;


*data = rx_buffer[tail];
tail = NextPosition(tail);

return 1;


}

/* Read several bytes from the buffer */
uint16_t Bluetooth_ReadBuffer(uint8_t *buffer, uint16_t size)
{
uint16_t count = 0;


if (buffer == NULL)
    return 0;

while (count < size && Bluetooth_Read(&buffer[count]))
    count++;

return count;


}

/* Check if a UART error occurred */
uint8_t Bluetooth_HasError(void)
{
return uart_error;
}

/* Clear the error flag */
void Bluetooth_ClearError(void)
{
uart_error = 0;
}

/* Handle received data */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
if (huart == bt_uart)
{
uint16_t next = NextPosition(head);


    /* Store data if the buffer is not full */
    if (next != tail)
    {
        rx_buffer[head] = rx_byte;
        head = next;
    }
    else
    {
        uart_error = 1;
    }

    /* Continue receiving data */
    if (HAL_UART_Receive_IT(bt_uart, &rx_byte, 1) != HAL_OK)
    {
        uart_error = 1;
        recovery_needed = 1;
    }
}


}

/* Handle UART errors */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
if (huart == bt_uart)
{
uart_error = 1;
recovery_needed = 1;
}
}

/* Recover UART reception outside the interrupt */
void Bluetooth_Process(void)
{
if (bt_uart == NULL || !recovery_needed)
return;


recovery_needed = 0;

/* Clear UART error flags */
__HAL_UART_CLEAR_PEFLAG(bt_uart);
__HAL_UART_CLEAR_FEFLAG(bt_uart);
__HAL_UART_CLEAR_NEFLAG(bt_uart);
__HAL_UART_CLEAR_OREFLAG(bt_uart);

/* Restart reception if UART is ready */
if (HAL_UART_GetState(bt_uart) == HAL_UART_STATE_READY)
{
    if (HAL_UART_Receive_IT(bt_uart, &rx_byte, 1) != HAL_OK)
        recovery_needed = 1;
}
else
{
    recovery_needed = 1;
}


}
