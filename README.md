# Bare-Metal STM32 BLDC/DC Motor Controller

Register-level embedded systems project on STM32F401RE (Nucleo-64).
No ST HAL/LL drivers — direct register manipulation against RM0368,
UM1724, and PM0214. CMSIS core headers (NVIC/SysTick intrinsics,
device register definitions) are used; ST's abstraction driver layer
is not.

Status: in progress. Start date 2026-07-01.
