#ifndef ENUMS_H
#define ENUMS_H

#include <EventManager.h>

#define DEFAULT_ID -1

extern EventManager buttonEM;
extern EventManager relayEM;
extern EventManager externalEM;

namespace ButtonEvent {
enum Type {
    StateChanged,
    Pressed,
    Released,
    Repeat,
    Hold,
    LongClick,
    Click,
    DoubleClick,
    EventsCount // must be last
};
}

namespace ButtonPullup {
enum PullupType {
    PullDown,
    IntPullup,
    ExtPullup
};
}

namespace ButtonState {
enum State {
    Any = 0,
    Up = 1,
    Down = 2,
    HoldState = 4
};
}

namespace ActionCondition {
enum Condition {
    ANY,
    ALL,
    ONE,
    NONE
};
}

namespace RelayAction {
enum Action {
    Unassigned,
    NoAction,
    On,
    Off,
    Toggle,
    Value,
    IncValue
};
}

namespace RelayStateSave {
enum Save {
    None,
    State,
    Value,
    StateAndValue
};
}

#endif // ENUMS_H
