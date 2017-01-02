#include "BounceButton.h"

BounceButton::BounceButton(int pin, ButtonPullup::PullupType pullup)
    : Bounce()
    , ID(DEFAULT_ID)
    , _referenceCount(0)
    , m_pullup(pullup)
    , m_bounceInterval(BOUNCE_INTERVAL)
    , m_holdInterval(HOLD_INTERVAL)
    , m_repeat(false)
    , m_repeatInterval(REPEAT_INTERVAL)
    , m_doubleClickInterval(0)
    , m_preventClick(false)
    , buttonPressTimeStamp(0)
    , lastRepeatTimeStamp(0)
    , clickCount(0)
    , isHold(false)
    , eventManager(0)
{
    this->setPin(pin);
    this->setPullup(m_pullup);
}

void BounceButton::setPin(int pin)
{
    this->m_pin = pin;
    reset();
}

void BounceButton::setRepeat(bool repeat)
{
    this->m_repeat = repeat;
}

void BounceButton::setHoldInterval(int holdInterval)
{
    this->m_holdInterval = holdInterval;
}

void BounceButton::setDoubleClickInterval(int dClickInterval)
{
    this->m_doubleClickInterval = dClickInterval;
}

void BounceButton::setPreventClick(bool preventClick)
{
    this->m_preventClick = preventClick;
}

void BounceButton::setRepeatInterval(int repeatInterval)
{
    this->m_repeatInterval = repeatInterval;
}

void BounceButton::setDoubleClick(int dClickInterval, bool preventClick)
{
    this->m_doubleClickInterval = dClickInterval;
    this->m_preventClick = preventClick;
}

void BounceButton::setPullup(ButtonPullup::PullupType pullup)
{
    this->m_pullup = pullup;
    pinMode(this->m_pin, m_pullup == ButtonPullup::IntPullup ? INPUT_PULLUP : INPUT);
    reset();
}

void BounceButton::setBounceInterval(int bounceInterval)
{
    this->m_bounceInterval = bounceInterval;
    this->interval(bounceInterval);
}

bool BounceButton::pressed()
{
    return (isPullup() && fell()) || (!isPullup() && rose());
}

bool BounceButton::released()
{
    return (!isPullup() && fell()) || (isPullup() && rose());
}

bool BounceButton::down()
{
    return (!isPullup() && read()) || (isPullup() && !read());
}

inline bool BounceButton::up()
{
    return !down();
}

inline void BounceButton::queueEvent(int event)
{
    if(eventManager)
        eventManager->queueEvent(event, new EventParam(this));
}

void BounceButton::reset()
{
    this->attach(m_pin);
    this->bState = up() ? ButtonState::Up : ButtonState::Down;
}

void BounceButton::check(uint32_t millisec)
{
    if (!millisec)
        millisec = millis();

    if(this->update())
        queueEvent(ButtonEvent::StateChanged);

    if(up() && (clickCount > 0) && (millisec - buttonPressTimeStamp) >= m_doubleClickInterval)
    {
        if(clickCount == 1 && m_preventClick)
        {
            queueEvent((millisec - buttonPressTimeStamp) >= m_holdInterval ? ButtonEvent::LongClick : ButtonEvent::Click);
        }

        clickCount = 0;
    }

    if(this->pressed())
    {
        buttonPressTimeStamp = millisec;
        bState = ButtonState::Down;
        queueEvent(ButtonEvent::Pressed);
    }

    if(this->released())
    {
        queueEvent(ButtonEvent::Released);

        if(m_holdInterval && ((millisec - buttonPressTimeStamp) >= m_holdInterval))
        {
            queueEvent(ButtonEvent::LongClick);
        }
        else
        {
            if(this->m_doubleClickInterval > 0)
            {
                if(++clickCount == 1 && !m_preventClick)
                    queueEvent(ButtonEvent::Click);

                if(clickCount == 2)
                    queueEvent(ButtonEvent::DoubleClick);
            }
            else
            {
                queueEvent(isHold ? ButtonEvent::LongClick : ButtonEvent::Click);
            }
        }

        isHold = false;
        bState = ButtonState::Up;
    }

    if(down())
    {
        if(!buttonPressTimeStamp)
            buttonPressTimeStamp = millisec;

        if(m_repeat && isHold && (millisec - lastRepeatTimeStamp) >= m_repeatInterval)
        {
            lastRepeatTimeStamp = millisec;
            queueEvent(ButtonEvent::Repeat);
        }

        if(!isHold && m_holdInterval && (millisec - buttonPressTimeStamp) >= m_holdInterval)
        {
            queueEvent(ButtonEvent::Hold);
            isHold = true;
            bState |= ButtonState::HoldState;

            if(m_repeat && (millisec - buttonPressTimeStamp) >= m_holdInterval)
            {
                lastRepeatTimeStamp = millisec;
                queueEvent(ButtonEvent::Repeat);
            }

        }
    }
}

ButtonList::ButtonList() : SimpleList<BounceButton *>()
{

}

bool ButtonList::addButton(BounceButton *button)
{
    if(!button || hasButton(button) || find(button->ID))
        return false;

    this->push_back(button);
    return true;
}

bool ButtonList::removeButton(BounceButton *button)
{
    if(button)
    {
        for(ButtonList::iterator itr = begin(); itr != end(); ++itr)
        {
            if((*itr) == button)
            {
                erase(itr);
                delete (*itr);
                return true;
            }
        }
    }
    return false;
}

bool ButtonList::hasButton(BounceButton *button)
{
    if(button)
    {
        for(ButtonList::iterator itr = begin(); itr != end(); ++itr)
        {
            if((*itr) == button)
                return true;
        }

    }
    return false;
}

BounceButton *ButtonList::find(int id)
{
    if(id != DEFAULT_ID)
    {
        for(ButtonList::iterator itr = begin(); itr != end(); ++itr)
        {
            if((*itr)->ID == id)
                return (*itr);
        }
    }
    return 0;
}

void ButtonList::check()
{
    for(ButtonList::iterator itr = begin(); itr != end(); ++itr)
        (*itr)->check();
}
