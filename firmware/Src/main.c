/**
 * main.c — motor control + telemetry main loop.
 *
 * CONTROL_MODE selects behavior:
 *   MODE_OPEN_LOOP  (default) — steps through fixed duty levels while
 *     streaming telemetry. This is the bring-up mode: it validates encoder
 *     counting and UART framing on hardware before any PID output is trusted.
 *     pwm_duty and rpm in the telemetry are real; setpoint_rpm echoes 0.
 *   MODE_CLOSED_LOOP — PID on encoder RPM. Requires ENCODER_CPR (encoder.h)
 *     to have been measured on hardware first, and gains tuned.
 *   MODE_CALIBRATE_CPR — motor off; prints the raw encoder count over UART
 *     as text every 250ms ("count=2464\r\n", view with screen/minicom at
 *     115200). To measure ENCODER_CPR: reset the board, mark the output
 *     shaft, rotate exactly one turn by hand, read the printed count.
 *
 * Timing: 1ms SysTick. Control update every 10ms (100Hz), telemetry frame
 * every 50ms (20Hz, ~2.4% of the 115200-baud wire at 14 bytes/frame).
 *
 * TB6612FNG STBY must be tied HIGH externally.
 */
#include "stm32f401xe.h"
#include "systick.h"
#include "motor.h"
#include "encoder.h"
#include "uart.h"
#include "telemetry.h"
#include "pid.h"

#define MODE_OPEN_LOOP     0
#define MODE_CLOSED_LOOP   1
#define MODE_CALIBRATE_CPR 2

#ifndef CONTROL_MODE
#define CONTROL_MODE MODE_OPEN_LOOP
#endif

#define CONTROL_PERIOD_MS   10u
#define TELEM_PERIOD_MS     50u

#if CONTROL_MODE == MODE_CLOSED_LOOP
/* Starting-point gains only — tune on hardware. Output is duty percent. */
#define PID_KP  0.30f
#define PID_KI  1.50f
#define PID_KD  0.00f
static pid_ctrl_t pid;
static int16_t setpoint_rpm = 150;
#elif CONTROL_MODE == MODE_OPEN_LOOP
/* Open-loop bring-up profile: duty steps, 5s each, then repeat. */
static const int16_t duty_profile[] = { 30, 50, 70, 50, -50, 0 };
#define DUTY_STEP_MS 5000u
#endif

/* Called by Reset_Handler before .data/.bss are initialized — must not
 * touch static storage. The startup declares SystemInit weak with no
 * definition, so without this the call links to a NOP and the FPU stays
 * disabled: under -mfloat-abi=hard the first float instruction (PID) then
 * takes a UsageFault (NOCP). */
void SystemInit(void)
{
    SCB->CPACR |= (0xFu << 20);        /* CP10/CP11 full access (FPU) */
    __DSB();
    __ISB();
}

int main(void)
{
    systick_init();
    motor_init();
    encoder_init();
    uart_init();

#if CONTROL_MODE == MODE_CALIBRATE_CPR
    /* Motor stays at duty 0 (motor_init leaves CCR1=0, direction coast).
     * TIM2->CNT starts at 0, so the printed count IS the accumulated
     * counts since reset — one hand-turn of the output shaft = CPR. */
    for (;;) {
        uart_write_str("count=");
        uart_write_i32((int32_t)encoder_count());
        uart_write_str("\r\n");
        delay_ms(250);
    }
#else

#if CONTROL_MODE == MODE_CLOSED_LOOP
    pid_init(&pid, PID_KP, PID_KI, PID_KD,
             (float)CONTROL_PERIOD_MS / 1000.0f, -100.0f, 100.0f);
#endif

    uint32_t next_control = millis();
    uint32_t next_telem = millis();
    int16_t rpm = 0;
    int16_t duty = 0;

    for (;;) {
        uint32_t now = millis();

        if ((int32_t)(now - next_control) >= 0) {
            next_control += CONTROL_PERIOD_MS;
            rpm = encoder_rpm(CONTROL_PERIOD_MS);

#if CONTROL_MODE == MODE_CLOSED_LOOP
            duty = (int16_t)pid_update(&pid, (float)setpoint_rpm, (float)rpm);
#else
            duty = duty_profile[(now / DUTY_STEP_MS) %
                                (sizeof(duty_profile) / sizeof(duty_profile[0]))];
#endif
            motor_set(duty);
        }

        if ((int32_t)(now - next_telem) >= 0) {
            next_telem += TELEM_PERIOD_MS;
#if CONTROL_MODE == MODE_CLOSED_LOOP
            telemetry_send(now, rpm, duty, setpoint_rpm);
#else
            telemetry_send(now, rpm, duty, 0);
#endif
        }
    }
#endif /* CONTROL_MODE != MODE_CALIBRATE_CPR */
}
