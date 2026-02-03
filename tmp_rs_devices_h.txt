
#ifndef RS_DEVICES_H_
#define RS_DEVICES_H_

#include <stdbool.h>

#define ALLBUTTON_MIN          0x08  
#define ALLBUTTON_MAX          0x0B

#define RS_SERIAL_ADAPTER_MIN  0x48
#define RS_SERIAL_ADAPTER_MAX  0x49

#define ONETOUCH_MIN           0x40
#define ONETOUCH_MAX           0x43

#define AQUALINKTOUCH_MIN      0x30
#define AQUALINKTOUCH_MAX      0x33

#define IAQUALINK_MIN          0xA0
#define IAQUALINK_MAX          0xA3

#define SPA_REMOTE_MIN         0x20
#define SPA_REMOTE_MAX         0x23

#define REMOTE_PWR_CENT_MIN    0x28
#define REMOTE_PWR_CENT_MAX    0x2B

#define PC_DOCK_MIN            0x58
#define PC_DOCK_MAX            0x5B

#define PDA_MIN                0x60
#define PDA_MAX                0x63

#define JANDY_DEV_SWG_MIN      0x50
#define JANDY_DEV_SWG_MAX      0x53

#define JANDY_DEV_PUMP_MIN     0x78
#define JANDY_DEV_PUMP_MAX     0x7B

#define JANDY_DEV_PUMP2_MIN    0xE0
#define JANDY_DEV_PUMP2_MAX    0xE3

#define JANDY_DEV_JXI_MIN      0x68
#define JANDY_DEV_JXI_MAX      0x6B

#define JANDY_DEV_LX_MIN       0x38
#define JANDY_DEV_LX_MAX       0x3B

#define JANDY_DEV_CHEM_MIN     0x80
#define JANDY_DEV_CHEM_MAX     0x83

#define JANDY_DEV_HPUMP_MIN    0x70
#define JANDY_DEV_HPUMP_MAX    0x73

#define JANDY_DEV_JLIGHT_MIN   0xF0
#define JANDY_DEV_JLIGHT_MAX   0xF4

#define JANDY_DEV_CHEM_ANLZ_MIN 0x84  
#define JANDY_DEV_CHEM_ANLZ_MAX 0x87

#define PENTAIR_DEV_PUMP_MIN   0x60
#define PENTAIR_DEV_PUMP_MAX   0x6F


// Helper macro for range checks
#define BETWEEN_UCHAR(val, min, max) ((unsigned char)(val) >= (unsigned char)(min) && (unsigned char)(val) <= (unsigned char)(max))


// --- Inline ID checkers ---
static inline bool is_allbutton_id(unsigned char val)         { return BETWEEN_UCHAR(val, ALLBUTTON_MIN, ALLBUTTON_MAX); }
static inline bool is_rsserialadapter_id(unsigned char val)   { return BETWEEN_UCHAR(val, RS_SERIAL_ADAPTER_MIN, RS_SERIAL_ADAPTER_MAX); }
static inline bool is_onetouch_id(unsigned char val)          { return BETWEEN_UCHAR(val, ONETOUCH_MIN, ONETOUCH_MAX); }
static inline bool is_aqualink_touch_id(unsigned char val)    { return BETWEEN_UCHAR(val, AQUALINKTOUCH_MIN, AQUALINKTOUCH_MAX); }
static inline bool is_iaqualink_id(unsigned char val)         { return BETWEEN_UCHAR(val, IAQUALINK_MIN, IAQUALINK_MAX); }
static inline bool is_spa_remote_id(unsigned char val)        { return BETWEEN_UCHAR(val, SPA_REMOTE_MIN, SPA_REMOTE_MAX); }
static inline bool is_remote_powercenter_id(unsigned char val){ return BETWEEN_UCHAR(val, REMOTE_PWR_CENT_MIN, REMOTE_PWR_CENT_MAX); }
static inline bool is_pc_dock_id(unsigned char val)           { return BETWEEN_UCHAR(val, PC_DOCK_MIN, PC_DOCK_MAX); }
static inline bool is_pda_id(unsigned char val)               { return BETWEEN_UCHAR(val, PDA_MIN, PDA_MAX); }

static inline bool is_swg_id(unsigned char val)               { return BETWEEN_UCHAR(val, JANDY_DEV_SWG_MIN, JANDY_DEV_SWG_MAX); }
static inline bool is_jxi_heater_id(unsigned char val)        { return BETWEEN_UCHAR(val, JANDY_DEV_JXI_MIN, JANDY_DEV_JXI_MAX); }
static inline bool is_lx_heater_id(unsigned char val)         { return BETWEEN_UCHAR(val, JANDY_DEV_LX_MIN, JANDY_DEV_LX_MAX); }
static inline bool is_chem_feeder_id(unsigned char val)       { return BETWEEN_UCHAR(val, JANDY_DEV_CHEM_MIN, JANDY_DEV_CHEM_MAX); }
static inline bool is_chem_anlzer_id(unsigned char val)       { return BETWEEN_UCHAR(val, JANDY_DEV_CHEM_ANLZ_MIN, JANDY_DEV_CHEM_ANLZ_MAX); }
static inline bool is_heat_pump_id(unsigned char val)         { return BETWEEN_UCHAR(val, JANDY_DEV_HPUMP_MIN, JANDY_DEV_HPUMP_MAX); }
static inline bool is_jandy_light_id(unsigned char val)       { return BETWEEN_UCHAR(val, JANDY_DEV_JLIGHT_MIN, JANDY_DEV_JLIGHT_MAX); }

static inline bool is_jandy_pump_std_id(unsigned char val)    { return BETWEEN_UCHAR(val, JANDY_DEV_PUMP_MIN, JANDY_DEV_PUMP_MAX); }
static inline bool is_jandy_pump_new_id(unsigned char val)    { return BETWEEN_UCHAR(val, JANDY_DEV_PUMP2_MIN, JANDY_DEV_PUMP2_MAX); }
static inline bool is_jandy_pump_id(unsigned char val)        {return is_jandy_pump_std_id(val) || is_jandy_pump_new_id(val);}

static inline bool is_pentair_pump_id(unsigned char val)      { return BETWEEN_UCHAR(val, PENTAIR_DEV_PUMP_MIN, PENTAIR_DEV_PUMP_MAX); }


#endif // RS_DEVICES_H_