/**
 * pid.h — float PID (F401 has single-precision FPU, hard float ABI).
 * Derivative on measurement (no setpoint kick), conditional-integration
 * anti-windup, symmetric output clamp.
 *
 * Named pid_ctrl_t, not pid_t: newlib's <sys/types.h> already owns pid_t.
 */
#ifndef PID_H
#define PID_H

typedef struct {
    float kp;
    float ki;          /* per second */
    float kd;          /* seconds */
    float dt;          /* control period, seconds */
    float out_min;
    float out_max;
    float integ;       /* internal state */
    float prev_meas;   /* internal state */
} pid_ctrl_t;

void pid_init(pid_ctrl_t *p, float kp, float ki, float kd,
              float dt, float out_min, float out_max);
void pid_reset(pid_ctrl_t *p);
float pid_update(pid_ctrl_t *p, float setpoint, float measurement);

#endif
