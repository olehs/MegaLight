#include <EEPROM.h>
#include "PWM.h"
#include <JointPoint.h>

// MajorDoMo server settings
static char mdHost[] = "192.168.21.100";
static char mdAuth[] = "";
static uint16_t mdPort = 80;

JointPointList jpList;

void loadJPConfig()
{
  if (!jpList.checkDB())
    jpList.saveConfig();
  else
    jpList.loadConfig();
}

void setupJoints()
{
  jpList.host = mdHost;
  jpList.port = mdPort;
  jpList.auth = mdAuth;

//  jpList.saveConfig();
  loadJPConfig();
}

