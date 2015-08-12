#include "bldc_task.h"
#include "bldc_driver.h"
#include "debug_dac.h"
#include "util.h"
#include "FreeRTOS.h"
#include "task.h"
#include <math.h>

// Block commutation scheme
// ========================
//
//       B         .            1   2   3   4   5   6
//        \   2   .             aaaabbbbbbbbccccccccaaaa
//         \     .                 ba      cb      ac
//     3    \   .    1            b  a    c  b    a  c
//           \ .                 b    a  c    b  a    c
//   - - - - -7-------- A       b      ac      ba      c
//           / .                ccccccccaaaaaaaabbbbbbbb
//     4    /   .    6
//         /     .              0: Free-Wheel, 7: Brake
//        /   5   .
//       C         .
//
// TODO
// ====
// [x] LED-Blinken mit bldc_param->polepairs
// [x] Spannungsmessung  hypot(alpha, beta)
// [x] Auf 95% PWM-Verhältnis begrenzen
// [x] Tuning-Parameter für Back-EMF-Erkennung:
//     [x] t_emf_hold_off [50us]
//     [x] u_emf_hyst [V]
//     Damit lassen sich auch schwierige Motoren
//     (ebmpapst, conrad, etc..) in den Griff kriegen
// [x] check_emf() immer mitlaufen lassen
// [x] Drehzahlmessung in RPM
//     sp 410 1040 411 5000
//     sp 420 1001 421 10
//
// [ ] Zeitfenster für Kommutierung (Stall-Erkennung)
//
//     sp 410 1010 420 1011 411 10 421 10    1000 2 1007 1
//
// [ ] Anlauf-Sequenz
// [ ] State-Machine
//     Fehlerbits mit/ohne latch
//
// [x] Steps auch im Leerlauf weiterzählen
// [ ] Start aus free-wheeling mit
//       m->u_d = K_v * RPM
//
// [ ] Phasenstrom mit Current-Probe messen
// [ ] Current-Decay Spannungs/Zeitfläche messen, um Strom zu schätzen!
//     Strom klingt mit Zeitkonstante L/R ab. Parameter dafür wieder einführen.
//
// [ ] Direction unabhängig von Voltage?
//     Zum Bremsen kann negative Spannung sinnvoll sein
//
// [ ] check_mosfets() - Monitoring (oder integriert in check_limits?)
//     Nach t_holdoff gucken, ob gewünschte Spannung erreicht wird.
//
// [ ] 100kHz PWM mit Dithering
//
// [x] Overlap-Commutation wie TB6575 --> Ausprobiert. Bringt nichts.
//     Beim erreichen der EMF schonmal nächste Phase einschalten
//     (SiLabs AN7894  nennt das "Hyperdrive" ;)
//
union error_flags   errors;
union warning_flags warnings;

struct bldc_state   bldc_state;
struct bldc_params  bldc_params;


void adc_filter(void *s, void *d, int n)
{
    uint32_t *src = s;
    uint32_t *dst = d;

    for (int i=0; i < 12/2; i++)
        dst[i] = 0;

    // Just add long words. There is no overflow.
    //
    for (int j=0; j < n; j++)
        for (int i=0; i < 12/2; i++)
            dst[i] += *src++;
}


static void check_limits(void)
{
    // Check battery voltage
    //
    if (bldc_state.u_bat < bldc_params.u_bat_min      )  errors.u_bat_min = 1;
    if (bldc_state.u_bat > bldc_params.u_bat_min + 0.3)  errors.u_bat_min = 0;
    if (bldc_state.u_bat > bldc_params.u_bat_max      )  errors.u_bat_max = 1;
    if (bldc_state.u_bat < bldc_params.u_bat_max - 0.3)  errors.u_bat_max = 0;

    if (bldc_state.thdn)
        errors.fet_temp = 1;

    bldc_state.errors = errors.w;
}


static bool check_emf(const struct motor_state *m)
{
    float u_high = m->u_null + bldc_params.u_emf_hyst;
    float u_low  = m->u_null - bldc_params.u_emf_hyst;

    switch (m->step) {
    case 1:   return m->u_b > u_high;
    case 2:   return m->u_a < u_low;
    case 3:   return m->u_c > u_high;
    case 4:   return m->u_b < u_low;
    case 5:   return m->u_a > u_high;
    case 6:   return m->u_c < u_low;
    default:  return false;
    }
}


static void step_motor(struct motor_state *m)
{
    if (m->u_pwm >= 0) {
        m->pos++;
        if (++m->step > 6)
            m->step = 1;
    }
    else {
        m->pos--;
        if (--m->step < 1)
            m->step = 6;
    }

    m->t_step_last = bldc_irq_count;
}


static void update_sensorless(struct motor_state *m)
{
    uint32_t t = bldc_irq_count;
    int emf = check_emf(m);

    // If there's nothing scheduled yet and the hold-off time
    // has elapsed look for rising edges
    //
    if ((t > m->t_step_next)
            && (t > m->t_step_last + bldc_params.t_emf_hold_off)
            && (!m->emf && emf)
       )
    {
        // schedule next step
        //
        int dt = (t - m->t_step_next) / 2 - bldc_params.t_deadtime;
        m->t_step_next    = t + clamp(dt, 0, 20);
        m->t_step_timeout = t + (m->t_step_next - m->t_step_last) * 2;
        m->emf_ok = 1;
    }

    m->emf = emf;

    if (t >= m->t_step_timeout) {
        m->emf_ok = 0;
    }

    if (t == m->t_step_next) {
        // time reached, step motor
        //
        step_motor(m);
    }
}


static void update_start(struct motor_state *m)
{
    if (m->t_state == 0) {
        // brake
        m->u_pwm = 0;
        step_motor(m);
    }

    if (m->t_state == 2000)  {
        // align
        if (m->reverse)
            m->u_pwm = -2;
        else
            m->u_pwm = 2;
        step_motor(m);
    }

    if (m->t_state == 5000)  step_motor(m);
    if (m->t_state == 5100)  step_motor(m);
    if (m->t_state == 5200)  step_motor(m);

    if (m->t_state == 5300)  {
        m->state = STATE_RUNNING;
        m->t_state = 0;
        return;
    }

    m->t_state++;
}


static void update_motor(struct motor_state *m)
{
    switch (m->state) {
    case STATE_STOP:
        m->u_pwm = 0;
        m->step  = 0;
        m->t_state = 0;
        break;

    case STATE_START:
        update_start(m);
        break;

    case STATE_RUNNING: {
        const float dt = 1.0 / BLDC_IRQ_FREQ;
        const float du_max = bldc_params.dudt_max * dt;

        if (m->reverse)
            m->u_pwm = clamp(-m->u_d, m->u_pwm - du_max, m->u_pwm + du_max);
        else
            m->u_pwm = clamp( m->u_d, m->u_pwm - du_max, m->u_pwm + du_max);

        update_sensorless(m);
        break;
        }

    case STATE_ERROR:
        break;
    }

}


void bldc_irq_handler(void)
{
    check_limits();

    for (int id=0; id<4; id++) {
        struct motor_state *m = &bldc_state.motors[id];

        m->u_alpha = (2 * m->u_a - m->u_b - m->u_c) * (1.0/3);
        m->u_beta  = (m->u_b - m->u_c) * (1/M_SQRT3);
        m->u_null  = (m->u_a + m->u_b + m->u_c) * (1.0/3);

        update_motor(m);

        if (m->pos % (6 * bldc_params.polepairs) == 0)
            m->led = 255;
        else
            m->led = 0;
    }

    debug_dac_update();
}



static void rpm_update(void)
{
    for (int id=0; id<4; id++) {
        struct motor_state *m = &bldc_state.motors[id];

        float f_el = (m->pos - m->rpm_old_pos) * (configTICK_RATE_HZ / 6.0);
        float rpm  = f_el * 60 / bldc_params.polepairs;

        lp2_filter(&m->rpm_filter, rpm);

        m->rpm_old_pos = m->pos;
    }
}


static void rpm_init(void)
{
    float fc = 10.0 / configTICK_RATE_HZ;

    for (int id=0; id<4; id++) {
        struct motor_state *m = &bldc_state.motors[id];
        lp2_set_fc(&m->rpm_filter, FILTER_CRITICALDAMPED, fc);
    }
}


void bldc_task(void *pvParameters)
{
    bldc_driver_init();
    rpm_init();

    for(;;) {
        rpm_update();
        vTaskDelay(1);
    }
}
