#ifndef __ALARM_H__
#define __ALARM_H__

void alarmSettingsInit(void);
void alarmSettingTick(void);
void alarmDisplayAlarms(void);
void alarmSettingChange(void);
void alarmOff(void);
void alarmChangeValue(char add);
#if 0
void Alarm_TickSetting(); //executes every second
#endif

/* AB TODO try to reduce */
extern volatile struct alarm_bcd alarms[NUM_ALARMS];
extern volatile char alarmsEnabled;
extern volatile char alarmDuration; 
extern char alarmSnooze;
extern volatile uint8_t alarmIndex;

#endif /* HEADER GUARD */


