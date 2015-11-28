#ifndef RELAYMODULE_H
#define RELAYMODULE_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <SimpleList.h>
#include <enums.h>

#define PWM_FREQUENCY = 1000
#define PWM_HIGH 255

class RelayModule;

class RelayList : public SimpleList<RelayModule *>
{
public:
    RelayList();

    bool addRelay(RelayModule *relay);
    bool removeRelay(RelayModule *relay);
    bool hasRelay(RelayModule *relay);
    RelayModule *find(int id);

    void check();
private:
};

class RelayModule
{
    friend class JointPoint;
public:
    RelayModule(int pin = 0);

    int ID;

    EventManager *eventManager;

    inline int pin() {return m_pin;}
    inline uint32_t pwm() {return m_pwm;}

    void setPin(int pin);
    void setPWM(uint32_t freq);
    void setInvert(bool inv);

    inline byte value() {return m_value;}
    inline bool on() {return m_on;}
    inline bool off() {return !m_on;}

    void setOn(uint32_t timeout = 0);
    void setOff(uint32_t timeout = 0);
    bool toggle(uint32_t timeout = 0);
    void setValue(byte value, uint32_t timeout = 0);
    void incValue(int val, uint32_t timeout = 0);

    uint32_t timeout();
    bool invert();

    bool action(RelayAction::Action action, int param = 0, uint32_t timeout = 0);
    void check(uint32_t millisec = 0);

    RelayStateSave::Save saveState();

    void setSaveState(RelayStateSave::Save saveState);
    inline int referenceCount() {return _referenceCount;}

protected:
    int m_pin;
    uint32_t m_pwm;
    byte m_value;
    bool m_on;
    bool m_invert;

    RelayStateSave::Save m_saveState;
    uint32_t m_timeout;
    uint32_t m_dim_t0, m_dim_t1;
    byte m_dim_v0, m_dim_v1;

    void setupPin();
    void updatePin(uint32_t timeout = 0, bool doEmit = true);
    void emitState();

private:
    int _referenceCount;
};

#endif // RELAYMODULE_H
