#ifndef REMOTE_PPM_H
#define REMOTE_PPM_H

#define RC_NUM_CHANNELS   8

struct rc_ppm_frame {
    int       channels[RC_NUM_CHANNELS];
    unsigned  timestamp;
    int       glitches;
};

extern void rc_init(void);
extern void rc_get_frame(struct rc_ppm_frame *rc);


#endif
