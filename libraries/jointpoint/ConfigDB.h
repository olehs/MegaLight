#ifndef CONFIGDB_H
#define CONFIGDB_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <EDB.h>

#include "enums.h"

#define DB_VERSION 2

#define DEBUG

class JointPointList;

class ConfigDB
{
public:
    ConfigDB(JointPointList *list = 0);

    void setList(JointPointList *list);

    void loadConfig();
    void saveConfig();
    void removeDB();
    bool checkDB();

    bool saveRelayStateAndValue(int id, bool on, byte value, bool saveState, bool saveValue);

private:
    struct DBHeader {
        unsigned int magic;
        unsigned int vesion;
        unsigned int buttons;
        unsigned int relays;
        unsigned int joints;
        unsigned int jointButtons;
        unsigned int jointRelays;
        unsigned int szStrings;
    } dbHeader;

    struct DBButton {
        int ID;
        byte pin;
        ButtonPullup::PullupType pullup;
        bool repeat;
        int bounceInterval;
        int holdInreval;
        int repeatInterval;
        int dClickInterval;
        bool preventClick;
    } dbButton;

    struct DBRelay {
        int ID;
        byte pin;
        uint32_t pwm;
        bool on;
        int invert;
        byte value;
        RelayStateSave::Save saveState;
    } dbRelay;

    struct DBJoint {
        int ID;
        char order;
        bool final;
        ButtonEvent::Type event;
        RelayAction::Action action;
        int param;
        uint32_t timeout;
        unsigned int adrCondition;
        unsigned int szCondition;
    } dbJoint;

    struct DBJointButtons
    {
        int jointID;
        int buttonID;
    } dbJointButtons;

    struct DBJointRelays
    {
        int jointID;
        int relayID;
    } dbJointRelays;

    JointPointList *jpList;
    EDB db;

    bool readHeader(DBHeader *header);
    void writeHeader(DBHeader *header);
    uint16_t updateAddresses(DBHeader *header);

    bool readButtons();
    bool readRelays();
    bool readJointPoints();
    bool readJB();
    bool readJR();

    bool writeButtons();
    bool writeRelays();
    bool writeJointPoints();
    bool writeJB();
    bool writeJR();

    void readString(unsigned int address, char *str, int len);
    void writeString(unsigned int address, const char *str);

    unsigned int buttonsAddress;
    unsigned int relaysAddress;
    unsigned int jointsAddress;
    unsigned int jointButtonsAddress;
    unsigned int jointRelaysAddress;
    unsigned int stringsAddress;
    unsigned int stringsSize;
};

#endif // CONFIGDB_H
