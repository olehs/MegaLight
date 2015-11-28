#include "RelayModule.h"
#include "PWM.h"

RelayModule::RelayModule(int pin)
    : ID(DEFAULT_ID)
    , _referenceCount(0)
    , m_timeout(0)
    , m_dim_t0(0)
    , m_dim_t1(0)
    , m_dim_v0(0)
    , m_dim_v1(0)
    , eventManager(0)
    , m_pwm(0)
    , m_on(false)
    , m_invert(false)
    , m_saveState(RelayStateSave::None)
    , m_value(0)
{
    setPin(pin);
}

void RelayModule::setPin(int pin)
{
    m_pin = pin;
    setupPin();
}

void RelayModule::setPWM(uint32_t freq)
{
    m_pwm = freq;
    setupPin();
}

void RelayModule::setValue(byte value, uint32_t timeout)
{
    if(timeout)
    {
        m_dim_v0 = m_value;
        m_dim_v1 = value;
        m_dim_t0 = millis();
        m_dim_t1 = m_dim_t0 + timeout;
        check();
    }
    else
    {
        m_value = value;
        m_dim_t1 = 0;
        updatePin(this->timeout());
    }
}

void RelayModule::setOn(uint32_t timeout)
{
    m_on = true;
    updatePin(timeout);
}

void RelayModule::setOff(uint32_t timeout)
{
    m_on = false;
    updatePin(timeout);
}

bool RelayModule::toggle(uint32_t timeout)
{
    on() ? setOff(timeout) : setOn(timeout);
}

void RelayModule::incValue(int val, uint32_t timeout)
{
    val += (int)m_value;

    if(val < 0)
        val = 0;
    else if(val > PWM_HIGH)
        val = PWM_HIGH;

    setValue(val, timeout);
}

bool RelayModule::action(RelayAction::Action action, int param, uint32_t timeout)
{
    switch(action) {
    case RelayAction::NoAction:
        updatePin();
        break;
    case RelayAction::On:
        setOn(timeout);
        break;
    case RelayAction::Off:
        setOff(timeout);
        break;
    case RelayAction::Toggle:
        toggle(timeout);
        break;
    case RelayAction::Value:
        setValue(param, timeout);
        break;
    case RelayAction::IncValue:
        incValue(param, timeout);
        break;
    }

    return value();
}

void RelayModule::check(uint32_t millisec)
{
    if(!millisec)
        millisec = millis();

    byte nv;
    if(m_dim_t1)
    {
        if(millisec >= m_dim_t1)
        {
            nv = m_dim_v1;
            m_dim_t1 = 0;
            emitState();
        }
        else
        {
            int dV = m_dim_v1 - m_dim_v0;
            int32_t dT = (m_dim_t1 - m_dim_t0);
            int32_t t = millisec - m_dim_t0;

            double v = t * dV / dT + m_dim_v0;
            if(v < 0)
                nv = 0;
            else if(v > PWM_HIGH)
                nv = PWM_HIGH;
            else
                nv = (byte)v;
        }
        if(nv != m_value)
        {
            m_value = nv;
            updatePin(timeout(), false);
//            Serialprint("val=%d\n", m_value);
        }
    }

    if(m_timeout)
    {
        if(millisec > m_timeout)
            toggle();
    }
}

uint32_t RelayModule::timeout()
{
    if(!m_timeout)
        return 0;
    return max(0, m_timeout - millis());
}

RelayStateSave::Save RelayModule::saveState()
{
    return m_saveState;
}

void RelayModule::setSaveState(RelayStateSave::Save saveState)
{
    m_saveState = saveState;
}

bool RelayModule::invert()
{
    return m_invert;
}

void RelayModule::setInvert(bool inv)
{
    m_invert = inv;
}

void RelayModule::setupPin()
{
    if(m_pin)
    {
        if (m_pwm)
            SetPinFrequencySafe(m_pin, m_pwm);
        else
            pinMode(m_pin, OUTPUT);
    }
    updatePin();
}

void RelayModule::updatePin(uint32_t timeout, bool doEmit)
{
    if(timeout)
        m_timeout = millis() + timeout;
    else
        m_timeout = 0;

    if(m_pin)
    {
        if(m_pwm)
        {
            byte onVal  = m_invert ? PWM_HIGH - m_value : m_value;
            byte offVal = m_invert ? PWM_HIGH : 0;
            pwmWrite(m_pin, m_on ? onVal : offVal);
        }
        else
        {
            int onState  = m_invert ? LOW : HIGH;
            int offState = m_invert ? HIGH : LOW;
            digitalWrite(m_pin, m_on ? onState : offState);
        }
    }

    if(doEmit)
        emitState();
}

void RelayModule::emitState()
{
    if(eventManager)
    {
        eventManager->queueEvent(0, new EventParam(this));
    }
}

RelayList::RelayList() : SimpleList<RelayModule *>()
{

}

bool RelayList::addRelay(RelayModule *relay)
{
    if(!relay || hasRelay(relay) || find(relay->ID))
        return false;

    this->push_back(relay);
    return true;
}

bool RelayList::removeRelay(RelayModule *relay)
{
    if(relay)
    {
        for(RelayList::iterator itr = begin(); itr != end(); ++itr)
        {
            if((*itr) == relay)
            {
                erase(itr);
                delete (*itr);
                return true;
            }
        }
    }
    return false;
}

bool RelayList::hasRelay(RelayModule *relay)
{
    if(relay)
    {
        for(RelayList::iterator itr = begin(); itr != end(); ++itr)
        {
            if((*itr) == relay)
                return true;
        }

    }
    return false;
}

RelayModule *RelayList::find(int id)
{
    if(id != DEFAULT_ID)
    {
        for(RelayList::iterator itr = begin(); itr != end(); ++itr)
        {
            if((*itr)->ID == id)
                return (*itr);
        }
    }
    return 0;
}


void RelayList::check()
{
    for(RelayList::iterator itr = begin(); itr != end(); ++itr)
        (*itr)->check();
}
