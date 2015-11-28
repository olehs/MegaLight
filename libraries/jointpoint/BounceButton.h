#ifndef BOUNCEBUTTON_H
#define BOUNCEBUTTON_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <enums.h>

#define DOUBLE_CLICK_INTERVAL 400
#define HOLD_INTERVAL 500
#define REPEAT_INTERVAL 250
#define BOUNCE_INTERVAL 40

#include <Bounce2.h>

#include <SimpleList.h>

class BounceButton;

class ButtonList : public SimpleList<BounceButton *>
{
public:
    ButtonList();

    bool addButton(BounceButton *button);
    bool removeButton(BounceButton *button);
    bool hasButton(BounceButton *button);
    BounceButton *find(int id);

    void check();

private:
};

class BounceButton : public Bounce
{
    friend class JointPoint;
public:
    BounceButton(int pin = 0, ButtonPullup::PullupType pullup = ButtonPullup::PullDown);

    int ID;

    EventManager *eventManager;

    inline int pin() {return m_pin;}
    inline ButtonPullup::PullupType pullup() {return m_pullup;}
    inline int bounceInterval() {return m_bounceInterval;}
    inline int holdInterval() {return m_holdInterval;}
    inline bool repeat() {return m_repeat;}
    inline int repeatInterval() {return m_repeatInterval;}
    inline int doubleClickInterval() {return m_doubleClickInterval;}
    inline bool preventClick() {return m_preventClick;}

    void setPin(int pin);
    void setPullup(ButtonPullup::PullupType pullup = ButtonPullup::IntPullup);
    void setBounceInterval(int bounceInterval);

    void setRepeat(bool repeat = true);
    void setRepeatInterval(int repeatInterval);
    void setHoldInterval(int holdInterval);
    void setDoubleClickInterval(int dClickInterval);
    void setPreventClick(bool preventClick = true);

    void setDoubleClick(int dClickInterval = DOUBLE_CLICK_INTERVAL, bool preventClick = true);

    void check(uint32_t millisec = 0);
    bool pressed();
    bool released();

    inline bool isPullup() {return m_pullup != ButtonPullup::PullDown;}

    inline bool hold() {return isHold;}
    bool down();
    bool up();
    inline int state() {return bState;}
    inline int referenceCount() {return _referenceCount;}

protected:
    void queueEvent(int event);
    void reset();

protected:
    int m_pin;
    ButtonPullup::PullupType m_pullup;
    int m_bounceInterval;

    bool m_repeat;
    int m_holdInterval;
    int m_repeatInterval;

    int m_doubleClickInterval;
    bool m_preventClick;

    uint32_t buttonPressTimeStamp;
    uint32_t lastRepeatTimeStamp;
    bool isHold;
    int clickCount;
    int bState;

private:
    int _referenceCount;
};

#endif // BOUNCEBUTTON_H
