#include "pid.h"

void pid_init(pid_ctrl_t *p, float kp, float ki, float kd,
              float dt, float out_min, float out_max)
{
    p->kp = kp;
    p->ki = ki;
    p->kd = kd;
    p->dt = dt;
    p->out_min = out_min;
    p->out_max = out_max;
    pid_reset(p);
}

void pid_reset(pid_ctrl_t *p)
{
    p->integ = 0.0f;
    p->prev_meas = 0.0f;
}

float pid_update(pid_ctrl_t *p, float setpoint, float measurement)
{
    float error = setpoint - measurement;

    /* derivative on measurement: avoids a spike when the setpoint steps */
    float deriv = -(measurement - p->prev_meas) / p->dt;
    p->prev_meas = measurement;

    float out = p->kp * error + p->ki * p->integ + p->kd * deriv;

    /* conditional integration: only accumulate when not pushing further
     * into saturation */
    if ((out < p->out_max || error < 0.0f) &&
        (out > p->out_min || error > 0.0f)) {
        p->integ += error * p->dt;
    }

    out = p->kp * error + p->ki * p->integ + p->kd * deriv;

    if (out > p->out_max) out = p->out_max;
    if (out < p->out_min) out = p->out_min;
    return out;
}
