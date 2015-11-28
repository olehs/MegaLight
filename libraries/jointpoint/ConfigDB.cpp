#include "ConfigDB.h"
#include "JointPoint.h"

#include <EEPROM.h>

#define TABLE_SIZE(x,n) ((unsigned int)sizeof(x) * n + (unsigned int)sizeof(EDB_Header))

#define DB_ADDRESS (unsigned int)12
#define DB_MAGIC 0xBEEF

#ifdef DEBUG
#define DEBUGp(x, ...) Serialprint(x, ##__VA_ARGS__)
#else
#define DEBUGp(x, ...)
#endif

// The read and write handlers for using the EEPROM Library
void writer(uint32_t address, byte data)
{
//    DEBUGp("W: ");
//    DEBUGp(address);
//    DEBUGp("=");
//    DEBUGp("%d\n", data);
    EEPROM.update(address, data);
}

byte reader(uint32_t address)
{
    byte data = EEPROM.read(address);
//    DEBUGp("R: ");
//    DEBUGp(address);
//    DEBUGp("=");
//    DEBUGp("%d\n", data);
    return data;
}

ConfigDB::ConfigDB(JointPointList *list)
    : db(&writer, &reader)
    , jpList(list)
{
}

void ConfigDB::loadConfig()
{
    if(!jpList)
        return;

    if(!readHeader(&dbHeader))
        return;

    uint16_t size = updateAddresses(&dbHeader);

    jpList->clearAll();

    readButtons();
    readRelays();
    readJointPoints();
    readJB();
    readJR();

    DEBUGp("Config loaded (%d bytes).\n", size);
}

void ConfigDB::saveConfig()
{
    if(!jpList)
        return;

    dbHeader.magic = DB_MAGIC;
    dbHeader.vesion = DB_VERSION;

    dbHeader.buttons = jpList->buttonCount();
    dbHeader.relays = jpList->relayCount();

    dbHeader.joints = 0;
    dbHeader.szStrings = 0;
    for(JointPointList::iterator itr=jpList->begin(); itr != jpList->end(); itr++)
        for(int i = (int)ButtonEvent::StateChanged; i < (int)ButtonEvent::EventsCount; i++)
        {
            dbJoint.action = (*itr)->eventActions[i].action;
            if(dbJoint.action != RelayAction::Unassigned)
            {
                dbHeader.joints++;
                dbHeader.szStrings += (*itr)->eventActions[i].condition.length();
            }
        }

    dbHeader.jointButtons = 0;
    dbHeader.jointRelays = 0;

    for(JointPointList::iterator itr = jpList->begin(); itr != jpList->end(); ++itr)
    {
        dbHeader.jointButtons += (*itr)->buttonCount();
        dbHeader.jointRelays += (*itr)->relayCount();
    }

    uint16_t size = updateAddresses(&dbHeader);

    writeHeader(&dbHeader);

    writeButtons();
    writeRelays();
    writeJointPoints();
    writeJB();
    writeJR();

    DEBUGp("Config saved (%d bytes).\n", size);
}

void ConfigDB::writeHeader(DBHeader *header)
{
    EEPROM.put(DB_ADDRESS, *header);

    db.create(buttonsAddress, TABLE_SIZE(DBButton, header->buttons), (unsigned int)sizeof(DBButton));
    db.create(relaysAddress, TABLE_SIZE(DBRelay, header->relays), (unsigned int)sizeof(DBRelay));
    db.create(jointsAddress, TABLE_SIZE(DBJoint, header->joints), (unsigned int)sizeof(DBJoint));
    db.create(jointButtonsAddress, TABLE_SIZE(DBJointButtons, header->jointButtons), (unsigned int)sizeof(DBJointButtons));
    db.create(jointRelaysAddress, TABLE_SIZE(DBJointRelays, header->jointRelays), (unsigned int)sizeof(DBJointRelays));
}

uint16_t ConfigDB::updateAddresses(ConfigDB::DBHeader *header)
{
    buttonsAddress = DB_ADDRESS + (unsigned int)sizeof(DBHeader);
    relaysAddress = buttonsAddress + TABLE_SIZE(DBButton, header->buttons);
    jointsAddress = relaysAddress  + TABLE_SIZE(DBRelay, header->relays);
    jointButtonsAddress = jointsAddress + TABLE_SIZE(DBJoint, header->joints);
    jointRelaysAddress = jointButtonsAddress + TABLE_SIZE(DBJointButtons, header->jointButtons);
    stringsAddress = jointRelaysAddress + TABLE_SIZE(DBJointRelays, header->jointRelays);
    stringsSize = header->szStrings;
    return stringsAddress + stringsSize;
}

void ConfigDB::removeDB()
{
    DBHeader header;
    header.magic = 0;
    header.vesion = 0;

    writeHeader(&header);
}

bool ConfigDB::checkDB()
{
    DBHeader header;
    return readHeader(&header);
}

bool ConfigDB::saveRelayStateAndValue(int id, bool on, byte value, bool saveState, bool saveValue)
{
    db.open(relaysAddress);
    EDB_Status result = EDB_OK;

    for (int recno = 1; recno <= db.count(); recno++)
    {
        EDB_Status result = db.readRec(recno, EDB_REC dbRelay);
        if (result != EDB_OK)
        {
            DEBUGp("readrelay %d\n", result);
            return false;
        }

        if(dbRelay.ID != id)
            continue;

        if(saveState)
            dbRelay.on = on;
        if(saveValue)
            dbRelay.value = value;

        result = db.updateRec(recno, EDB_REC dbRelay);
        if (result != EDB_OK)
        {
            DEBUGp("updaterelay %d\n", result);
            return false;
        }
    }
    return true;
}

bool ConfigDB::readButtons()
{
    db.open(buttonsAddress);

    BounceButton *button;
    for (int recno = 1; recno <= db.count(); recno++)
    {
        EDB_Status result = db.readRec(recno, EDB_REC dbButton);
        if (result != EDB_OK)
        {
            DEBUGp("readbut %d\n", result);
            return false;
        }

        button = new BounceButton(dbButton.pin, dbButton.pullup);
        button->ID = dbButton.ID;
        button->setBounceInterval(dbButton.bounceInterval);
        button->setRepeat(dbButton.repeat);
        button->setRepeatInterval(dbButton.repeatInterval);
        button->setHoldInterval(dbButton.holdInreval);
        button->setDoubleClick(dbButton.dClickInterval, dbButton.preventClick);
        jpList->addButton(button);
    }
    return true;
}

bool ConfigDB::readRelays()
{
    db.open(relaysAddress);

    RelayModule *relay;
    for (int recno = 1; recno <= db.count(); recno++)
    {
        EDB_Status result = db.readRec(recno, EDB_REC dbRelay);
        if (result != EDB_OK)
        {
            DEBUGp("readrelay %d\n", result);
            return false;
        }

        relay = new RelayModule(dbRelay.pin);
        relay->ID = dbRelay.ID;
        relay->setPWM(dbRelay.pwm);
        relay->setInvert(dbRelay.invert);
        relay->setSaveState(dbRelay.saveState);
        relay->setValue(dbRelay.value);
        dbRelay.on ? relay->setOn() : relay->setOff();

        jpList->addRelay(relay);
    }
    return true;
}

bool ConfigDB::readJointPoints()
{
    db.open(jointsAddress);

    for (int recno = 1; recno <= db.count(); recno++)
    {
        EDB_Status result = db.readRec(recno, EDB_REC dbJoint);
        if (result != EDB_OK)
        {
            DEBUGp("readjoint %d\n", result);
            return false;
        }

        JointPoint *jp = jpList->find(dbJoint.ID);
        if(!jp)
        {
            jp = jpList->add(dbJoint.ID);
            jp->order = dbJoint.order;
            jp->final = dbJoint.final;
        }
        jp->setAction(dbJoint.event, dbJoint.action, dbJoint.param, dbJoint.timeout);

        char cond[dbJoint.szCondition + 1];
        readString(dbJoint.adrCondition, cond, dbJoint.szCondition);
        jp->setCondition(dbJoint.event, cond);
    }
    return true;
}

bool ConfigDB::readJB()
{
    db.open(jointButtonsAddress);

    for (int recno = 1; recno <= db.count(); recno++)
    {
        EDB_Status result = db.readRec(recno, EDB_REC dbJointButtons);
        if (result != EDB_OK)
        {
            DEBUGp("readJB %d\n", result);
            return false;
        }

        JointPoint *jp = jpList->find(dbJointButtons.jointID);
        if(jp)
            jp->addButton(dbJointButtons.buttonID);
    }
    return true;
}

bool ConfigDB::readJR()
{
    db.open(jointRelaysAddress);

    for (int recno = 1; recno <= db.count(); recno++)
    {
        EDB_Status result = db.readRec(recno, EDB_REC dbJointRelays);
        if (result != EDB_OK)
        {
            DEBUGp("readJR %d\n", result);
            return false;
        }

        JointPoint *jp = jpList->find(dbJointRelays.jointID);
        if(jp)
            jp->addRelay(dbJointRelays.relayID);
    }
    return true;
}

bool ConfigDB::writeButtons()
{
    db.open(buttonsAddress);
    db.clear();

    EDB_Status result = EDB_OK;

    BounceButton *button;
    ButtonList *buttonList = jpList->allButtons();
    for(ButtonList::iterator itr = buttonList->begin(); itr != buttonList->end(); ++itr)
    {
        button = (*itr);
        dbButton.ID = button->ID;
        dbButton.pin = button->pin();
        dbButton.pullup = button->pullup();
        dbButton.bounceInterval = button->bounceInterval();
        dbButton.repeat = button->repeat();
        dbButton.repeatInterval = button->repeatInterval();
        dbButton.holdInreval = button->holdInterval();
        dbButton.dClickInterval = button->doubleClickInterval();
        dbButton.preventClick = button->preventClick();

        result = db.appendRec(EDB_REC dbButton);
        if (result != EDB_OK)
        {
            DEBUGp("writebut %d\n", result);
            break;
        }
    }
    return result = EDB_OK;
}

bool ConfigDB::writeRelays()
{
    db.open(relaysAddress);
    db.clear();

    EDB_Status result = EDB_OK;

    RelayModule *relay;
    RelayList *relayList = jpList->allRelays();
    for(RelayList::iterator itr = relayList->begin(); itr != relayList->end(); ++itr)
    {
        relay = (*itr);
        dbRelay.ID = relay->ID;
        dbRelay.pin = relay->pin();
        dbRelay.pwm = relay->pwm();
        dbRelay.saveState = relay->saveState();
        dbRelay.on = relay->on();
        dbRelay.invert = relay->invert();
        dbRelay.value = relay->value();

        result = db.appendRec(EDB_REC dbRelay);
        if (result != EDB_OK)
        {
            DEBUGp("writerelay %d\n", result);
            break;
        }
    }
    return result = EDB_OK;
}

bool ConfigDB::writeJointPoints()
{
    db.open(jointsAddress);
    db.clear();

    unsigned int strSize = 0;

    JointPoint *jp;
    for(JointPointList::iterator itr = jpList->begin(); itr != jpList->end(); ++itr)
    {
        jp = (*itr);
        dbJoint.ID = jp->ID;
        dbJoint.order = jp->order;
        dbJoint.final = jp->final;

        for(int i = (int)ButtonEvent::StateChanged; i<(int)ButtonEvent::EventsCount; i++)
        {
            dbJoint.action = jp->eventActions[i].action;
            if(dbJoint.action != RelayAction::Unassigned)
            {
                dbJoint.event = (ButtonEvent::Type)i;
                dbJoint.param = jp->eventActions[i].param;
                dbJoint.timeout = jp->eventActions[i].timeout;

                dbJoint.szCondition = jp->eventActions[i].condition.length();
                dbJoint.adrCondition = stringsAddress + strSize;
                strSize += dbJoint.szCondition;

                EDB_Status result = db.appendRec(EDB_REC dbJoint);
                if (result != EDB_OK)
                {
                    DEBUGp("writejoint %d\n", result);
                    return false;
                }

                writeString(dbJoint.adrCondition, jp->eventActions[i].condition.c_str());
            }
        }
    }
    return true;
}

bool ConfigDB::writeJB()
{
    db.open(jointButtonsAddress);
    db.clear();

    JointPoint *jp;
    for(JointPointList::iterator itr = jpList->begin(); itr != jpList->end(); ++itr)
    {
        jp = (*itr);
        dbJointButtons.jointID = jp->ID;

        for(ButtonList::iterator itr = jp->buttonList.begin(); itr != jp->buttonList.end(); ++itr)
        {
            dbJointButtons.buttonID = (*itr)->ID;

            EDB_Status result = db.appendRec(EDB_REC dbJointButtons);
            if (result != EDB_OK)
            {
                DEBUGp("writeJB %d\n", result);
                return false;
            }
        }
    }
    return true;
}

bool ConfigDB::writeJR()
{
    db.open(jointRelaysAddress);
    db.clear();

    JointPoint *jp;
    for(JointPointList::iterator itr = jpList->begin(); itr != jpList->end(); ++itr)
    {
        jp = (*itr);
        dbJointRelays.jointID = jp->ID;

        for(RelayList::iterator itr = jp->relayList.begin(); itr != jp->relayList.end(); ++itr)
        {
            dbJointRelays.relayID = (*itr)->ID;

            EDB_Status result = db.appendRec(EDB_REC dbJointRelays);
            if (result != EDB_OK)
            {
                DEBUGp("writeJR %d\n", result);
                return false;
            }
        }
    }
    return true;
}

void ConfigDB::readString(unsigned int address, char *str, int len)
{
    char *p = str;
    uint32_t ee = stringsAddress + address;

    for (unsigned int i = 0; i < len; i++)
        *p++ = reader(ee++);

    *p = '\0';
}

void ConfigDB::writeString(unsigned int address, const char *str)
{
    char *p = (char *)str;
    uint32_t ee = stringsAddress + address;

    for (unsigned int i = 0; i < strlen(str); i++)
        writer(ee++, *p++);
}

bool ConfigDB::readHeader(DBHeader *header)
{
    EEPROM.get(DB_ADDRESS, *header);

    if (header->magic != DB_MAGIC || header->vesion != DB_VERSION)
        return false;

    return true;
}
