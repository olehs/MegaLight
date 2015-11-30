#include "JointPoint.h"

EventManager buttonEM(EventManager::kNotInterruptSafe);
EventManager relayEM(EventManager::kNotInterruptSafe);
EventManager externalEM(EventManager::kNotInterruptSafe);


inline bool byOrder(JointPoint *a, JointPoint *b) {
    return a->order > b->order;
}

JointPointList::JointPointList()
    : callableButtonListener(this,   &JointPointList::buttonEventListener)
    , callableRelayListener(this,    &JointPointList::relayEventListener)
    , callableExtrenalListener(this, &JointPointList::externalEventListener)
    , externalEventsEnabled(true)
    , tokenEvaluator(this, &JointPointList::eval_token)
    , evaluator(&tokenEvaluator)
    , config(this)
    , sorted(false)
{
    buttonEM.addListener(ButtonEvent::StateChanged, &callableButtonListener);
    buttonEM.addListener(ButtonEvent::Pressed     , &callableButtonListener);
    buttonEM.addListener(ButtonEvent::Released    , &callableButtonListener);
    buttonEM.addListener(ButtonEvent::Repeat      , &callableButtonListener);
    buttonEM.addListener(ButtonEvent::Hold        , &callableButtonListener);
    buttonEM.addListener(ButtonEvent::LongClick   , &callableButtonListener);
    buttonEM.addListener(ButtonEvent::Click       , &callableButtonListener);
    buttonEM.addListener(ButtonEvent::DoubleClick , &callableButtonListener);

    relayEM.addListener(RelayAction::NoAction, &callableRelayListener);
    relayEM.addListener(RelayAction::On,       &callableRelayListener);
    relayEM.addListener(RelayAction::Off,      &callableRelayListener);
    relayEM.addListener(RelayAction::Toggle,   &callableRelayListener);
    relayEM.addListener(RelayAction::Value,    &callableRelayListener);
    relayEM.addListener(RelayAction::IncValue, &callableRelayListener);


    externalEM.setDefaultListener(&callableExtrenalListener);
}

JointPointList::~JointPointList()
{
    clearAll();
}

void JointPointList::loadConfig()
{
    config.loadConfig();
    sort(byOrder);
    sorted = true;
}

void JointPointList::saveConfig()
{
    sort(byOrder);
    sorted = true;
    config.saveConfig();
}

JointPoint *JointPointList::add(int id)
{
    JointPoint *jp = new JointPoint(this);
    jp->ID = id;
    push_back(jp);
    sorted = false;
    return jp;
}

void JointPointList::remove(JointPoint *jp)
{
    if(!jp)
        return;

    for(JointPointList::iterator itr = begin(); itr != end(); ++itr)
    {
        if((*itr) == jp)
        {
            itr = erase(itr);
            delete jp;
            return;
        }
    }
}

JointPoint *JointPointList::find(int id)
{
    if(id != DEFAULT_ID)
    {
        for(JointPointList::iterator itr = begin(); itr != end(); ++itr)
        {
            if((*itr)->ID == id)
                return (*itr);
        }
    }
    return 0;
}

bool JointPointList::addButton(BounceButton *button)
{
    if(button)
    {
        if(buttonList.addButton(button))
        {
            button->eventManager = &buttonEM;
            return true;
        }
    }
    return false;
}

bool JointPointList::removeButton(BounceButton *button)
{
    if(button)
    {
        if(button->referenceCount())
            return false;

        return buttonList.removeButton(button);
    }
    return false;
}

RelayModule *JointPointList::findRelay(int id)
{
    return relayList.find(id);
}

int JointPointList::relayCount()
{
    return relayList.size();
}

bool JointPointList::eval(const char *expr)
{
    int r = evaluator.eval(expr);
    if(r == EVAL_FAILURE)
    {
#ifdef JPDEBUG
        Serialprint("Error evaluating expression %s\n", expr);
#endif
        return false;
    }
    return r != 0;
}

void JointPointList::clearAll()
{
    for(ButtonList::iterator itr = buttonList.begin(); itr != buttonList.end(); ++itr)
        delete (*itr);
    buttonList.clear();

    for(RelayList::iterator itr = relayList.begin(); itr != relayList.end(); ++itr)
        delete (*itr);
    relayList.clear();

    for(JointPointList::iterator itr = begin(); itr != end(); ++itr)
        delete (*itr);
    clear();
}

BounceButton *JointPointList::findButton(int id)
{
    return buttonList.find(id);
}

int JointPointList::buttonCount()
{
    return buttonList.size();
}

JointPoint::JointPoint(JointPointList *owner)
    : jpList(owner)
    , ID(DEFAULT_ID)
    , order(0)
    , final(false)
{
    for(int i=0; i<ButtonEvent::EventsCount; i++)
    {
        eventActions[i].action = RelayAction::Unassigned;
        eventActions[i].param = 0;
        eventActions[i].timeout = 0;
        eventActions[i].condition = "";
    }
}

JointPoint::~JointPoint()
{
    clearButtons();
    clearRelays();
}

bool JointPoint::hasButton(BounceButton *button)
{
    for(ButtonList::iterator itr = buttonList.begin(); itr != buttonList.end(); ++itr)
    {
        if((*itr) == button)
            return true;
    }
    return false;
}

bool JointPoint::addButton(int id)
{
    BounceButton *button = jpList->findButton(id);
    if(!button)
        return false;

    if(buttonList.addButton(button))
    {
        button->_referenceCount++;
        return true;
    }
    return false;
}

bool JointPoint::removeButton(int id)
{
    BounceButton *button = jpList->findButton(id);
    if(!button)
        return false;

    for(ButtonList::iterator itr = buttonList.begin(); itr != buttonList.end(); ++itr)
    {
        if((*itr) == button)
        {
            buttonList.erase(itr);
            button->_referenceCount--;
            return true;
        }
    }
    return false;
}

bool JointPoint::hasButton(int id)
{
    BounceButton *button = jpList->findButton(id);
    if(!button)
        return false;

    return hasButton(button);
}

int JointPoint::buttonCount()
{
    return buttonList.size();
}

void JointPoint::clearButtons()
{
    for(ButtonList::iterator itr = buttonList.begin(); itr != buttonList.end(); ++itr)
        (*itr)->_referenceCount--;
    buttonList.clear();
}

bool JointPoint::hasRelay(RelayModule *relay)
{
    for(RelayList::iterator itr = relayList.begin(); itr != relayList.end(); ++itr)
    {
        if((*itr) == relay)
            return true;
    }
    return false;
}

bool JointPoint::addRelay(int id)
{
    RelayModule *relay = jpList->findRelay(id);
    if(!relay)
        return false;

    if(relayList.addRelay(relay))
    {
        relay->_referenceCount++;
        return true;
    }
    return false;
}

bool JointPoint::removeRelay(int id)
{
    RelayModule *relay = jpList->findRelay(id);
    if(!relay)
        return false;

    for(RelayList::iterator itr = relayList.begin(); itr != relayList.end(); ++itr)
    {
        if((*itr) == relay)
        {
            relayList.erase(itr);
            relay->_referenceCount--;
            return true;
        }
    }
    return false;
}

bool JointPoint::hasRelay(int id)
{
    RelayModule *relay = jpList->findRelay(id);
    if(!relay)
        return false;

    return hasRelay(relay);
}

int JointPoint::relayCount()
{
    return relayList.size();
}

void JointPoint::clearRelays()
{
    for(RelayList::iterator itr = relayList.begin(); itr != relayList.end(); ++itr)
        (*itr)->_referenceCount--;
    relayList.clear();
}

void JointPoint::setAction(ButtonEvent::Type event, RelayAction::Action action, int param, uint32_t timeout)
{
    eventActions[event].action = action;
    eventActions[event].param = param;
    eventActions[event].timeout = timeout;
}

void JointPoint::unsetAction(ButtonEvent::Type event)
{
    eventActions[event].action = RelayAction::Unassigned;
}

void JointPoint::setCondition(ButtonEvent::Type event, const char *condition)
{
    eventActions[event].condition = condition;
}

bool JointPoint::processButtonEvent(int event, BounceButton *button)
{
    RelayAction::Action action = eventActions[event].action;
    if(action == RelayAction::Unassigned)
        return false;

    if(!hasButton(button))
        return false;

    if(eventActions[event].condition.length() && !jpList->eval(eventActions[event].condition.c_str()))
        return false;

    for(RelayList::iterator itr = relayList.begin(); itr != relayList.end(); ++itr)
        relayEM.queueEvent(action, new RelayEventParam((*itr), eventActions[event].param, eventActions[event].timeout));

    return true;
}

bool JointPointList::addRelay(RelayModule *relay)
{
    if(relay)
    {
        if(relayList.addRelay(relay))
        {
            relay->eventManager = &externalEM;
            return true;
        }
    }
    return false;
}

bool JointPointList::removeRelay(RelayModule *relay)
{
    if(relay)
    {
        if(relay->referenceCount())
            return false;

        return relayList.removeRelay(relay);
    }
    return false;
}

void JointPointList::buttonEventListener(int event, EventParam *param)
{
    BounceButton *button = reinterpret_cast<BounceButton *>(param->sender);

#ifdef JPDEBUG
    switch(event){
    case ButtonEvent::Pressed:
        Serialprint("Button %d Pressed\n", button->ID);
        break;
    case ButtonEvent::Released:
        Serialprint("Button %d Released\n", button->ID);
        break;
    case ButtonEvent::Repeat:
        Serialprint("Button %d Repeat\n", button->ID);
        break;
    case ButtonEvent::Hold:
        Serialprint("Button %d Hold\n", button->ID);
        break;
    case ButtonEvent::Click:
        Serialprint("Button %d Click\n", button->ID);
        break;
    case ButtonEvent::DoubleClick:
        Serialprint("Button %d DoubleClick\n", button->ID);
        break;
    case ButtonEvent::LongClick:
        Serialprint("Button %d LongClick\n", button->ID);
        break;
    }
#endif

    if(!sorted)
    {
        sort(byOrder);
        sorted = true;
    }

    for(JointPointList::iterator itr = begin(); itr != end(); ++itr)
    {
        if((*itr)->processButtonEvent(event, button))
            if((*itr)->final)
                break;
    }
}

void JointPointList::relayEventListener(int event, EventParam *param)
{
    RelayEventParam *p = reinterpret_cast<RelayEventParam *>(param);
    RelayModule *relay = static_cast<RelayModule *>(p->sender);

    if(!relay || !relayList.hasRelay(relay))
        return;

    relay->action((RelayAction::Action)event, p->param, p->timeout);

#ifdef JPDEBUG
    switch(event){
    case RelayAction::NoAction:
        Serialprint("Relay %d NoAction\n", relay->ID);
        break;
    case RelayAction::On:
        Serialprint("Relay %d On\n", relay->ID);
        break;
    case RelayAction::Off:
        Serialprint("Relay %d Off\n", relay->ID);
        break;
    case RelayAction::Toggle:
        Serialprint("Relay %d Toggle\n", relay->ID);
        break;
    case RelayAction::Value:
        Serialprint("Relay %d Value=%d\n", relay->ID, p->param);
        break;
    case RelayAction::IncValue:
        Serialprint("Relay %d IncValue=%d\n", relay->ID, p->param);
        break;
    }
#endif
}

int JointPointList::eval_token(char *expr)
{
    String s = expr;
    s.trim();
    if(!s.length())
        return 0;

    s.toLowerCase();

    char c = s[0];
    if(c == 'r')
    {
        s.remove(0, 1);
        c = s[0];
        bool needValue = false;
        if(c == 'v')
            needValue = true;

        while(c && !isdigit(c))
        {
            s.remove(0, 1);
            c = s[0];
        }

        int id = s.toInt();
        RelayModule *relay = findRelay(id);
        if(relay)
            return needValue ? relay->value() : (relay->on() ? 1 : 0);
    }
    else if(c == 'b')
    {
        s.remove(0, 1);
        ButtonState::State state;
        c = s[0];
        switch(c) {
        case 'u' :
            state = ButtonState::Up;
            break;
        case 'h' :
            state = ButtonState::HoldState;
            break;
        default:
            state = ButtonState::Down;
        }

        while(c && !isdigit(c))
        {
            s.remove(0, 1);
            c = s[0];
        }

        int id = s.toInt();
        BounceButton *button = findButton(id);
        if(button)
            return button->state() == state ? 1 : 0;
    }
    else
    {
        return s.toInt();
    }

    return 0;
}

void JointPointList::externalEventListener(int event, EventParam *param)
{
    if(!externalEventsEnabled)
        return;
    
    if(event == 0) // relay event
    {
        RelayModule *relay = reinterpret_cast<RelayModule *>(param->sender);

        bool saveState = (relay->saveState() == RelayStateSave::State) || (relay->saveState() == RelayStateSave::StateAndValue);
        bool saveValue = (relay->saveState() == RelayStateSave::Value) || (relay->saveState() == RelayStateSave::StateAndValue);

        config.saveRelayStateAndValue(relay->ID, relay->on(), relay->value(), saveState, saveValue);

        if(!client.connected())
            client.stop();

        if(client.connect(host.c_str(), port))
        {
            Streamprint(client, "GET /objects/?object=ThisComputer&op=m&m=setRelayState");
            Streamprint(client, "&id=%d", relay->ID);
            Streamprint(client, "&on=%d", relay->on());
            Streamprint(client, "&v=%d", relay->value());
            Streamprint(client, " HTTP/1.0\n");
            Streamprint(client, "Host: %s\n", host.c_str());
            if(auth.length())
                Streamprint(client, "Authorization: Basic %s\n", auth.c_str());
            Streamprint(client, "Connection: close\n\n");
            delay(10);
            while (client.available()) {
                client.read();
            }
            client.stop();
        }
    }
}
