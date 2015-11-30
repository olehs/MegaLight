#ifndef JOINTPOINT_H
#define JOINTPOINT_H

#include <SimpleList.h>
#include <BounceButton.h>
#include <RelayModule.h>
#include <ConfigDB.h>
#include <EthernetClient.h>
#include <ExpressionEvaluator.h>
#include <enums.h>

#define JPDEBUG

class JointPoint;

class RelayEventParam : public EventParam
{
public:
    RelayEventParam(void *sender, int param, uint32_t timeout = 0) : EventParam(sender, param), timeout(timeout) {}
    uint32_t timeout;
};

class JointPointList : public SimpleList<JointPoint *>
{
public:
    JointPointList();
    ~JointPointList();

    String host;
    String auth;
    uint16_t port;

    bool externalEventsEnabled;

    void loadConfig();
    void saveConfig();
    inline void removeDB() {config.removeDB();}
    inline bool checkDB() {return config.checkDB();}

    JointPoint *add(int id = DEFAULT_ID);
    void remove(JointPoint *jp);
    JointPoint *find(int id);

    void clearAll();

    inline ButtonList *allButtons() {return &buttonList;}
    inline RelayList *allRelays() {return &relayList;}

    bool addButton(BounceButton *button);
    bool removeButton(BounceButton *button);
    BounceButton *findButton(int id);
    int buttonCount();

    bool addRelay(RelayModule *relay);
    bool removeRelay(RelayModule *relay);
    RelayModule *findRelay(int id);
    int relayCount();

    bool eval(const char *expr);

private:
    void buttonEventListener( int event, EventParam *param );
    MemberFunctionCallable<JointPointList> callableButtonListener;

    void externalEventListener( int event, EventParam *param );
    MemberFunctionCallable<JointPointList> callableExtrenalListener;

    void relayEventListener(int event, EventParam *param);
    MemberFunctionCallable<JointPointList> callableRelayListener;

    int eval_token(char *expr);
    MemberFunctionTokenEvaluator<JointPointList> tokenEvaluator;

    EthernetClient client;

    ButtonList buttonList;
    RelayList relayList;

    ConfigDB config;

    ExpressionEvaluator evaluator;

    bool sorted;
};

class JointPoint
{
    friend class ConfigDB;
public:
    struct EventAction{
        RelayAction::Action action;
        int param;
        uint32_t timeout;
        String condition;
    };

    JointPoint(JointPointList *owner);
    ~JointPoint();

    int ID;
    char order;
    bool final;

    bool addButton(int id);
    bool removeButton(int id);
    bool hasButton(int id);
    bool hasButton(BounceButton *button);
    int buttonCount();
    void clearButtons();

    bool addRelay(int id);
    bool removeRelay(int id);
    bool hasRelay(int id);
    bool hasRelay(RelayModule *relay);
    int relayCount();
    void clearRelays();

    inline ButtonList *buttons() {return &buttonList;}
    inline RelayList *relays() {return &relayList;}

    EventAction &eventAction(ButtonEvent::Type event) {return eventActions[event];}

    void setAction(ButtonEvent::Type event, RelayAction::Action action, int param = 0, uint32_t timeout = 0);
    void unsetAction(ButtonEvent::Type event);
    void setCondition(ButtonEvent::Type event, const char *condition);

    bool processButtonEvent(int event, BounceButton *button);

private:
    JointPointList *jpList;
    ButtonList buttonList;
    RelayList relayList;


    EventAction eventActions[ButtonEvent::EventsCount];
};

#endif // JOINTPOINT_H
