#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
#include <utility/w5100.h>

#define TCP_RETRANSMISSION_TIME 150

static uint8_t mac[] = { 0x34, 0xAD, 0xBE, 0x43, 0xFE, 0x68 };
static uint8_t ip[] = { 192, 168, 21, 210 };

void saveEthernetConfig()
{
  EEPROM.update(0, 0xDE);
  EEPROM.update(1, 0xAD);
  for(int i=0; i<6; i++)
    EEPROM.update(i + 2, mac[i]);
  
  for(int i=0; i<4; i++)
    EEPROM.update(i + 8, ip[i]);
}

void loadEthernetConfig()
{
  if(EEPROM.read(0) != 0xDE || EEPROM.read(1) != 0xAD)
  {
    saveEthernetConfig();
    return;
  }
  
  for(int i=0; i<6; i++)
    mac[i] = EEPROM.read(i + 2);
  
  for(int i=0; i<4; i++)
    ip[i] = EEPROM.read(i + 8);
}

#define PREFIX ""

WebServer webserver(PREFIX, 80);

P(Header_Buttons) = "<a href=\"/buttons\">Buttons</a>&nbsp;|&nbsp;";
P(Header_Relays) = "<a href=\"/relays\">Relays</a>&nbsp;|&nbsp;";
P(Header_JointPoints) = "<a href=\"/joints\">JointPoints</a>&nbsp;|&nbsp;";
P(Header_Config) = "<a href=\"/config\">Config</a>\n";
P(Header_font_start) = "<font size=\"5\">";
P(Header_font_end) = "</font>";


P(Page_start) = "<html><head><title>Setup</title></head><body>\n";
P(Page_end) = "</body></html>";

P(Form_eth_start1) = "<FORM action=\"";
P(Form_eth_start2) = "\" method=\"get\">";
P(Form_end) = "</FORM>";
P(Form_input_send) = "<INPUT type=\"submit\" value=\"Save config\">";

P(Form_input_text_start) = "<input type=\"text\" style=\"text-align: center;width: 40px;\" name=\"";
P(Form_input_number_start) = "<input type=\"number\" style=\"text-align: center;width: 80px;\" name=\"";
P(Form_input_value)  = "\" value=\"";
P(Form_input_min) = "\" min=\"";
P(Form_input_max) = "\" max=\"";
P(Form_input_end) = "\">\n";


P(br) = "<br>\n";

P(table_start) = "<table border=\"1\" cellspacing=\"0\" cellpadding=\"2\">";
P(table_th_start) = "<th>";
P(table_th_end) = "</th>";
P(table_tr_start) = "<tr align=\"center\">";
P(table_tr_end) = "</tr>";
P(table_td_start) = "<td>";
P(table_td_start_left) = "<td style=\"text-align: left; font-family: monospace\">";
P(table_td_end) = "</td>";
P(table_end) = "</table>";

P(Form_cb_start) = "<input type=\"checkbox\" name=\"";
P(Form_cb_value) = "\" value=\"";
P(Form_cb_checked) = "\" checked=\"checked";
P(Form_cb_end) = "\">";

P(Form_sel_start) = "<select name=\"";
P(Form_sel_attr_end) = "\">";
P(Form_sel_option_start) = "<option value=\"";
P(Form_sel_option_selected) = "\" selected>";
P(Form_sel_option_end) = "</option>";
P(Form_sel_end) = "</select>";

P(MAC) = "MAC";
P(IP) = "IP";

P(ID) = "ID";
P(PIN) = "Pin";
P(bPULLUP) = "Pullup type";
P(bREPEAT) = "Repeat";
P(bBOUNCE_INTERVAL) = "Bounce interval (ms)";
P(bHOLD_INTERVAL) = "Hold interval (ms)";
P(bREPEAT_INTERVAL) = "Repeat interval (ms)";
P(bDCLICK_INTERVAL) = "Double-click interval (ms)";
P(bPREVENT_CLICK) = "Prevent Click";
P(bDELETE) = "!DELETE!";

#ifndef NO_PWM_LIB
P(rPWM) = "PWM Frequency (Hz)<br>0 - On/Off Switch";
#else
P(rPWM) = "PWM";
#endif

P(rON) = "On";
P(rINVERT) = "Invert State";
P(rVALUE) = "Dimmer value (0-255)";
P(rPOWERON) = "PowerOn Restore";

P(jORDER) = "Order";
P(jFINAL) = "Final";
P(jBUTTONS) = "Buttons";
P(jRELAYS) = "Relays";
P(jEVENTS) = "Events";
P(jEVENT) = "Event";
P(jACTION) = "Action";
P(jPARAM) = "Parameter";
P(jTIMEOUT) = "Timeout (ms)";
P(jCONDITION) = "Condition";

P(Config_set) = "<font size=\"3\" color=\"red\">New configuration stored!</font>";

#define NAMELEN 16
#define VALUELEN 128

void printHeader(WebServer &server, int highlight = 0)
{
  int c = 0;
  if(++c == highlight) server.printP(Header_font_start);
  server.printP(Header_Buttons);
  if(c == highlight) server.printP(Header_font_end);

  if(++c == highlight) server.printP(Header_font_start);
  server.printP(Header_Relays);
  if(c == highlight) server.printP(Header_font_end);
  
  if(++c == highlight) server.printP(Header_font_start);
  server.printP(Header_JointPoints);
  if(c == highlight) server.printP(Header_font_end);
  
  if(++c == highlight) server.printP(Header_font_start);
  server.printP(Header_Config);
  if(c == highlight) server.printP(Header_font_end);
}

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  server.httpSuccess();

  server.printP(Page_start);

  printHeader(server);

  server.printP(Page_end);
}

void configCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  boolean params_present = false;
  bool saved = false;

  if (type == WebServer::HEAD)
  {
    server.httpSuccess();
    return;
  }

  if (type != WebServer::GET)
  {
    server.httpFail();
    return;
  }

  if (!tail_complete)
    Serialprint("Request buffer too small\n");

  // check for parameters
  if (strlen(url_tail)) {
    URLPARAM_RESULT rc;
    char name[NAMELEN];
    char value[VALUELEN];

    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc == URLPARAM_OK) {
        if (strcmp(name, "saved") == 0)
        {
          saved = true;
          break;
        }

        if (!strlen(value))
          continue;

        params_present = true;

        if (strcmp(name, "m1") == 0)
          mac[0] = strtol(value, NULL, 16);
        else if (strcmp(name, "m2") == 0)
          mac[1] = strtol(value, NULL, 16);
        else if (strcmp(name, "m3") == 0)
          mac[2] = strtol(value, NULL, 16);
        else if (strcmp(name, "m4") == 0)
          mac[3] = strtol(value, NULL, 16);
        else if (strcmp(name, "m5") == 0)
          mac[4] = strtol(value, NULL, 16);
        else if (strcmp(name, "m6") == 0)
          mac[5] = strtol(value, NULL, 16);

        else if (strcmp(name, "i1") == 0)
          ip[0] = atoi(value);
        else if (strcmp(name, "i2") == 0)
          ip[1] = atoi(value);
        else if (strcmp(name, "i3") == 0)
          ip[2] = atoi(value);
        else if (strcmp(name, "i4") == 0)
          ip[3] = atoi(value);
      }
    }
  }
  
  if (params_present)
  {
    saveEthernetConfig();
    server.httpSeeOther("/config?saved=1");
    return;
  }

  server.httpSuccess();

  server.printP(Page_start);

  printHeader(server, 4);

  server.printP(br);
  server.printP(br);

  server.printP(Form_eth_start1);
  server.print("config");
  server.printP(Form_eth_start2);

  server.printP(MAC);

  for (int i=0; i<6; i++)
  {
    server.printP(Form_input_text_start);
    server.print("m"); server.print(i+1);
    server.printP(Form_input_value); server.print(mac[i], HEX);
    server.printP(Form_input_end);
  }

  server.printP(br);server.printP(br);
  server.printP(IP);
  
  for (int i=0; i<4; i++)
  {
    server.printP(Form_input_number_start);
    server.print("i"); server.print(i+1);
    server.printP(Form_input_value); server.print(ip[i]);
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_max); server.print(255);
    server.printP(Form_input_end);
  }

  server.printP(br);

  server.printP(Form_input_send);
  server.printP(Form_end);

  if (saved)
    server.printP(Config_set);

  server.printP(Page_end);
}

void stateCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  server.httpSuccess("text/plain");

  if (type != WebServer::GET)
  {
    if (type != WebServer::HEAD)
      server.httpFail();
    return;
  }

  URLPARAM_RESULT rc;
  char name[NAMELEN];
  char value[VALUELEN];
  char c[NAMELEN];
  int id = 0;
  int state = -1;
  int on = -1;
  int inc = 0;
  uint32_t timeout = 0;

  if (!strlen(url_tail))
    return;

  while (strlen(url_tail))
  {
    rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
    if (rc == URLPARAM_OK)
    {
      if (name[0] == 'c')
        strcpy(c, value);
      else if (name[0] == 'n')
        id = atoi(value);
      else if (name[0] == 'o')
        on = atoi(value);
      else if (name[0] == 'v')
        state = strtoul(value, NULL, 10);
      else if (name[0] == 'i')
        inc = atoi(value);
      else if (name[0] == 't')
        timeout = strtoul(value, NULL, 10);
    }
  }

  if (!id)
    return;

  if (strcmp(c, "get") == 0)
  {
    RelayModule *relay = jpList.findRelay(id);
    if (!relay)
    {
      server.httpNoContent();
      return;
    }

    server.print(relay->on());
    server.print(";");
    server.print(relay->value());
  }
  else if (strcmp(c, "set") == 0)
  {
    RelayModule *relay = jpList.findRelay(id);
    if (!relay)
    {
      server.httpNoContent();
      return;
    }

    if (state >= 0 )
      relayEM.queueEvent(RelayAction::Value, new RelayEventParam(relay, state, timeout));
    if (inc != 0 )
      relayEM.queueEvent(RelayAction::IncValue, new RelayEventParam(relay, inc, timeout));
    if (on >= 0 )
      relayEM.queueEvent(on ? RelayAction::On : RelayAction::Off, new RelayEventParam(relay, on, timeout));
  }
  else if (strcmp(c, "button") == 0)
  {
    BounceButton *button = jpList.findButton(id);
    if (!button)
    {
      server.httpNoContent();
      return;
    }

    server.print(button->state() & ButtonState::Down);
  }
}

BounceButton *btnByParam(const char *param, const char *name, BounceButton **nb)
{
  BounceButton *b = 0;
  if (strncmp(param, name, strlen(name)) == 0)
  {
    if (!strlen(param + strlen(name))) {
      if (!(*nb)) {
        *nb = new BounceButton();
        (*nb)->ID = DEFAULT_ID;
      }
      b = *nb;
    }
    else
    {
      b = jpList.findButton(atoi(param + strlen(name)));
    }
  }
  return b;
}

const char * pullupName(int e)
{
  switch (e) {
    case ButtonPullup::PullDown:
      return "External PULLDOWN";
    case ButtonPullup::IntPullup:
      return "Internal PULLUP";
    case ButtonPullup::ExtPullup:
      return "External PULLUP";
  }
}

void buttonsCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  boolean params_present = false;
  BounceButton *b;
  BounceButton *nb = 0;
  bool saved = false;

  if (type == WebServer::HEAD)
  {
    server.httpSuccess();
    return;
  }

  if (type != WebServer::GET)
  {
    server.httpFail();
    return;
  }

  if (!tail_complete)
    Serialprint("Request buffer too small\n");

  // check for parameters
  if (strlen(url_tail)) {
    URLPARAM_RESULT rc;
    char name[NAMELEN];
    char value[VALUELEN];

    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc == URLPARAM_OK) {
        if (strcmp(name, "saved") == 0)
        {
          saved = true;
          break;
        }

        if (!strlen(value))
          continue;


        params_present = true;

        if (b = btnByParam(name, "del", &nb))
          jpList.removeButton(b);
        else if (b = btnByParam(name, "i", &nb))
        {
          if (strlen(value))
            b->ID = atoi(value);
        }
        else if (b = btnByParam(name, "p", &nb))
          b->setPin(atoi(value));
        else if (b = btnByParam(name, "u", &nb))
          b->setPullup((ButtonPullup::PullupType)atoi(value));
        else if (b = btnByParam(name, "ib", &nb))
          b->setBounceInterval(atoi(value));
        else if (b = btnByParam(name, "ih", &nb))
          b->setHoldInterval(atoi(value));
        else if (b = btnByParam(name, "r", &nb))
          b->setRepeat(atoi(value));
        else if (b = btnByParam(name, "ir", &nb))
          b->setRepeatInterval(atoi(value));
        else if (b = btnByParam(name, "d", &nb))
          b->setDoubleClickInterval(atoi(value));
        else if (b = btnByParam(name, "c", &nb))
          b->setPreventClick(atoi(value));
      }
    }
  }


  if (params_present)
  {
    if (nb)
    {
      if (nb->ID != DEFAULT_ID)
        jpList.addButton(nb);
      else
        delete nb;
    }
    jpList.saveConfig();
    server.httpSeeOther("/buttons?saved=1");
    return;
  }

  server.httpSuccess();

  server.printP(Page_start);

  printHeader(server, 1);

  server.printP(br);
  server.printP(br);

  server.printP(Form_eth_start1);
  server.print("buttons");
  server.printP(Form_eth_start2);

  server.printP(table_start);

  // print the header
  server.printP(table_tr_start);

  server.printP(table_th_start);
  server.printP(ID);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(PIN);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(bPULLUP);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(bBOUNCE_INTERVAL);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(bHOLD_INTERVAL);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(bREPEAT);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(bREPEAT_INTERVAL);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(bDCLICK_INTERVAL);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(bPREVENT_CLICK);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(bDELETE);
  server.printP(table_th_end);

  server.printP(table_tr_end);

  for (ButtonList::iterator itr = jpList.allButtons()->begin(); itr != jpList.allButtons()->end(); ++itr)
  {
    b = (*itr);
    server.printP(table_tr_start);

    server.printP(table_td_start);
    server.print(b->ID);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("p"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->pin());
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_max); server.print(254);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_sel_start);
    server.print("u"); server.print(b->ID);
    server.printP(Form_sel_attr_end);
    for (int i = 0; i <= ButtonPullup::ExtPullup; i++)
    {
      server.printP(Form_sel_option_start); server.print(i);
      if (i == (int)b->pullup())
        server.printP(Form_sel_option_selected);
      else
        server.printP(Form_sel_attr_end);
      server.print(pullupName(i));
      server.printP(Form_sel_option_end);
    }
    server.printP(Form_sel_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("ib"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->bounceInterval());
    server.printP(Form_input_min); server.print(1);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("ih"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->holdInterval());
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("r"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->repeat());
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_max); server.print(1);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("ir"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->repeatInterval());
    server.printP(Form_input_min); server.print(1);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("d"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->doubleClickInterval());
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("c"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->preventClick());
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_max); server.print(1);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    if (!b->referenceCount())
    {
      server.printP(Form_cb_start);
      server.print("del"); server.print(b->ID);
      server.printP(Form_cb_end);
    }
    else
    {
      server.print(b->referenceCount()); server.print(" ref(s)");
    }
    server.printP(table_td_end);

    server.printP(table_tr_end);
  }

  // new button
  server.printP(table_tr_start);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("i");
  server.printP(Form_input_min); server.print(1);
  server.printP(Form_input_max); server.print(254);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("p");
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_max); server.print(254);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_sel_start);
  server.print("u");
  server.printP(Form_sel_attr_end);
  for (int i = 0; i <= ButtonPullup::ExtPullup; i++)
  {
    server.printP(Form_sel_option_start); server.print(i);
    server.printP(Form_sel_attr_end);
    server.print(pullupName(i));
    server.printP(Form_sel_option_end);
  }
  server.printP(Form_sel_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("ib");
  server.printP(Form_input_min); server.print(1);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("ih");
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("r");
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_max); server.print(1);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("ir");
  server.printP(Form_input_min); server.print(1);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("d");
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("c");
  server.printP(Form_input_value); server.print(1);
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_max); server.print(1);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.print("NEW");
  server.printP(table_td_end);

  server.printP(table_tr_end);

  // table end
  server.printP(table_end);

  server.printP(br);

  server.printP(Form_input_send);
  server.printP(Form_end);

  if (saved)
    server.printP(Config_set);

  server.printP(Page_end);
}

const char * saveStateName(int e)
{
  switch (e) {
    case RelayStateSave::None:
      return "None";
    case RelayStateSave::State:
      return "State";
    case RelayStateSave::Value:
      return "Value";
    case RelayStateSave::StateAndValue:
      return "State&Value";
  }
}

RelayModule *relayByParam(const char *param, const char *name, RelayModule **nb)
{
  RelayModule *b = 0;
  if (strncmp(param, name, strlen(name)) == 0)
  {
    if (!strlen(param + strlen(name))) {
      if (!(*nb)) {
        *nb = new RelayModule();
        (*nb)->ID = DEFAULT_ID;
      }
      b = *nb;
    }
    else
    {
      b = jpList.findRelay(atoi(param + strlen(name)));
    }
  }
  return b;
}

void relaysCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  boolean params_present = false;
  RelayModule *b;
  RelayModule *nb = 0;
  bool saved = false;

  if (type == WebServer::HEAD)
  {
    server.httpSuccess();
    return;
  }

  if (type != WebServer::GET)
  {
    server.httpFail();
    return;
  }

  if (!tail_complete)
    Serialprint("Request buffer too small\n");

  // check for parameters
  if (strlen(url_tail)) {
    URLPARAM_RESULT rc;
    char name[NAMELEN];
    char value[VALUELEN];

    jpList.externalEventsEnabled = false;
    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc == URLPARAM_OK) {
        if (strcmp(name, "saved") == 0)
        {
          saved = true;
          break;
        }

        if (!strlen(value))
          continue;

        params_present = true;

        if (b = relayByParam(name, "del", &nb))
          jpList.removeRelay(b);
        else if (b = relayByParam(name, "i", &nb))
        {
          if (strlen(value))
            b->ID = atoi(value);
        }
        else if (b = relayByParam(name, "p", &nb))
          b->setPin(atoi(value));
        else if (b = relayByParam(name, "w", &nb))
          b->setPWM(strtoul(value, NULL, 10));
        else if (b = relayByParam(name, "o", &nb))
          atoi(value) ? b->setOn() : b->setOff();
        else if (b = relayByParam(name, "r", &nb))
          b->setInvert(atoi(value));
        else if (b = relayByParam(name, "v", &nb))
          b->setValue(atoi(value));
        else if (b = relayByParam(name, "s", &nb))
          b->setSaveState((RelayStateSave::Save)atoi(value));
      }
    }
    jpList.externalEventsEnabled = true;
  }

  if (params_present)
  {
    if (nb)
    {
      if (nb->ID != DEFAULT_ID)
        jpList.addRelay(nb);
      else
        delete nb;
    }
    jpList.saveConfig();
    server.httpSeeOther("/relays?saved=1");
    return;
  }

  server.httpSuccess();

  server.printP(Page_start);

  printHeader(server, 2);

  server.printP(br);
  server.printP(br);

  server.printP(Form_eth_start1);
  server.print("relays");
  server.printP(Form_eth_start2);

  server.printP(table_start);

  // print the header
  server.printP(table_tr_start);

  server.printP(table_th_start);
  server.printP(ID);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(PIN);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(rPWM);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(rON);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(rINVERT);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(rVALUE);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(rPOWERON);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(bDELETE);
  server.printP(table_th_end);

  server.printP(table_tr_end);

  for (RelayList::iterator itr = jpList.allRelays()->begin(); itr != jpList.allRelays()->end(); ++itr)
  {
    b = (*itr);
    server.printP(table_tr_start);

    server.printP(table_td_start);
    server.print(b->ID);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("p"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->pin());
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_max); server.print(254);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("w"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->pwm());
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_max); server.print(2000000);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("o"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->on());
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_max); server.print(1);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("r"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->invert());
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_max); server.print(1);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("v"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->value());
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_max); server.print(255);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_sel_start);
    server.print("s"); server.print(b->ID);
    server.printP(Form_sel_attr_end);
    for (int i = 0; i <= RelayStateSave::StateAndValue; i++)
    {
      server.printP(Form_sel_option_start); server.print(i);
      if (i == (int)b->saveState())
        server.printP(Form_sel_option_selected);
      else
        server.printP(Form_sel_attr_end);
      server.print(saveStateName(i));
      server.printP(Form_sel_option_end);
    }
    server.printP(Form_sel_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    if (!b->referenceCount())
    {
      server.printP(Form_cb_start);
      server.print("del"); server.print(b->ID);
      server.printP(Form_cb_end);
    }
    else
    {
      server.print(b->referenceCount()); server.print(" ref(s)");
    }
    server.printP(table_td_end);

    server.printP(table_tr_end);
  }

  // new relay
  server.printP(table_tr_start);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("i");
  server.printP(Form_input_min); server.print(1);
  server.printP(Form_input_max); server.print(254);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("p");
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_max); server.print(254);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("w");
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_max); server.print(2000000);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("o");
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_max); server.print(1);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("r");
  server.printP(Form_input_value);
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_max); server.print(1);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("v");
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_max); server.print(255);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_sel_start);
  server.print("s");
  server.printP(Form_sel_attr_end);
  for (int i = 0; i <= RelayStateSave::StateAndValue; i++)
  {
    server.printP(Form_sel_option_start); server.print(i);
    server.printP(Form_sel_attr_end);
    server.print(saveStateName(i));
    server.printP(Form_sel_option_end);
  }
  server.printP(Form_sel_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.print("NEW");
  server.printP(table_td_end);

  server.printP(table_tr_end);

  // table end
  server.printP(table_end);

  server.printP(br);

  server.printP(Form_input_send);
  server.printP(Form_end);

  if (saved)
    server.printP(Config_set);

  server.printP(Page_end);
}

JointPoint *jpByParam(const char *param, const char *name, JointPoint **nb)
{
  JointPoint *b = 0;
  if (strncmp(param, name, strlen(name)) == 0)
  {
    if (!strlen(param + strlen(name))) {
      if (!(*nb)) {
        *nb = jpList.add();
      }
      b = *nb;
    }
    else
    {
      int id = atoi(param + strlen(name));
      b = jpList.find(id);
    }
  }
  return b;
}

void jointsCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  URLPARAM_RESULT rc;
  char name[NAMELEN];
  char value[VALUELEN];
  boolean params_present = false;
  int c = 0;
  JointPoint *b;
  JointPoint *nb = 0;
  bool saved = false;

  if (type == WebServer::HEAD)
  {
    server.httpSuccess();
    return;
  }

  if (type != WebServer::GET)
  {
    server.httpFail();
    return;
  }

  if (!tail_complete)
    Serialprint("Request buffer too small\n");

  // check for parameters
  if (strlen(url_tail)) {

    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc == URLPARAM_OK) {
        if (strcmp(name, "saved") == 0)
        {
          saved = true;
          break;
        }

        if (!strlen(value))
          continue;

        if (!params_present)
        {
          for (JointPointList::iterator itr = jpList.begin(); itr != jpList.end(); ++itr)
          {
            (*itr)->clearButtons();
            (*itr)->clearRelays();
          }
          params_present = true;
        }

        if (b = jpByParam(name, "del", &nb))
          jpList.remove(b);
        else if (b = jpByParam(name, "i", &nb))
        {
          if (strlen(value))
            b->ID = atoi(value);
        }
        else if (b = jpByParam(name, "o", &nb))
          b->order = (atoi(value));
        else if (b = jpByParam(name, "f", &nb))
          b->final = (atoi(value));
        else if (b = jpByParam(name, "b", &nb))
          b->addButton(atoi(value));
        else if (b = jpByParam(name, "r", &nb))
          b->addRelay(atoi(value));
      }
    }
  }


  if (params_present)
  {
    if (nb && (nb->ID == DEFAULT_ID))
      jpList.remove(nb);
    jpList.saveConfig();
    server.httpSeeOther("/joints?saved=1");
    return;
  }

  server.httpSuccess();

  server.printP(Page_start);

  printHeader(server, 3);

  server.printP(br);
  server.printP(br);

  server.printP(Form_eth_start1);
  server.print("joints");
  server.printP(Form_eth_start2);

  server.printP(table_start);

  // print the header
  server.printP(table_tr_start);

  server.printP(table_th_start);
  server.printP(ID);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(jBUTTONS);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(jEVENTS);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(jRELAYS);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(jORDER);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(jFINAL);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(bDELETE);
  server.printP(table_th_end);

  server.printP(table_tr_end);

  for (JointPointList::iterator itr = jpList.begin(); itr != jpList.end(); ++itr)
  {
    b = (*itr);
    server.printP(table_tr_start);

    server.printP(table_td_start);
    server.print(b->ID);
    server.printP(table_td_end);

    c = 0;
    server.printP(table_td_start_left);
    for (ButtonList::iterator itr = jpList.allButtons()->begin(); itr != jpList.allButtons()->end(); ++itr)
    {
      server.printP(Form_cb_start);
      server.print("b"); server.print(b->ID);
      server.printP(Form_cb_value); server.print((*itr)->ID);
      if (b->hasButton((*itr)->ID))
        server.printP(Form_cb_checked);
      server.printP(Form_cb_end);
      sprintf(value, "%-2.d", (*itr)->ID);
      server.print(value);
      if (++c % 10 == 0)
        server.printP(br);
    }
    server.printP(table_td_end);

    server.printP(table_td_start);
    for (int e = 0; e < ButtonEvent::EventsCount; e++)
    {
      int a = b->eventAction((ButtonEvent::Type)e).action;
      if (a == RelayAction::Unassigned)
        continue;
      server.print(eventName(e));
      server.print("&nbsp;-&nbsp;"); 
      if(b->eventAction((ButtonEvent::Type)e).condition.length()) {
        server.print("("); server.print(b->eventAction((ButtonEvent::Type)e).condition); server.print(")&nbsp;-&nbsp;"); 
      }
      server.print(actionName(a));
      server.printP(br);
    }
    server.print("<a href=\"/jpevents?jp="); server.print(b->ID); server.print("\">Edit</a>");
    server.printP(table_td_end);

    c = 0;
    server.printP(table_td_start_left);
    for (RelayList::iterator itr = jpList.allRelays()->begin(); itr != jpList.allRelays()->end(); ++itr)
    {
      server.printP(Form_cb_start);
      server.print("r"); server.print(b->ID);
      server.printP(Form_cb_value); server.print((*itr)->ID);
      if (b->hasRelay((*itr)->ID))
        server.printP(Form_cb_checked);
      server.printP(Form_cb_end);
      sprintf(value, "%-2.d", (*itr)->ID);
      server.print(value);
      if (++c % 10 == 0)
        server.printP(br);
    }
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("o"); server.print(b->ID);
    server.printP(Form_input_value); server.print((int)b->order);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("f"); server.print(b->ID);
    server.printP(Form_input_value); server.print(b->final);
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_max); server.print(1);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_cb_start);
    server.print("del"); server.print(b->ID);
    server.printP(Form_cb_end);
    server.printP(table_td_end);

    server.printP(table_tr_end);
  }

  // new joints
  server.printP(table_tr_start);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("i");
  server.printP(Form_input_min); server.print(1);
  server.printP(Form_input_max); server.print(254);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  c = 0;
  server.printP(table_td_start_left);
  for (ButtonList::iterator itr = jpList.allButtons()->begin(); itr != jpList.allButtons()->end(); ++itr)
  {
    server.printP(Form_cb_start);
    server.print("b");
    server.printP(Form_cb_value); server.print((*itr)->ID);
    server.printP(Form_cb_end);
    sprintf(value, "%-2.d", (*itr)->ID);
    server.print(value);
    if (++c % 10 == 0)
      server.printP(br);
  }
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(table_td_end);

  c = 0;
  server.printP(table_td_start_left);
  for (RelayList::iterator itr = jpList.allRelays()->begin(); itr != jpList.allRelays()->end(); ++itr)
  {
    server.printP(Form_cb_start);
    server.print("r");
    server.printP(Form_cb_value); server.print((*itr)->ID);
    server.printP(Form_cb_end);
    sprintf(value, "%-2.d", (*itr)->ID);
    server.print(value);
    if (++c % 10 == 0)
      server.printP(br);
  }
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("o");
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.printP(Form_input_number_start);
  server.print("f");
  server.printP(Form_input_min); server.print(0);
  server.printP(Form_input_max); server.print(1);
  server.printP(Form_input_end);
  server.printP(table_td_end);

  server.printP(table_td_start);
  server.print("NEW");
  server.printP(table_td_end);

  server.printP(table_tr_end);

  // table end
  server.printP(table_end);

  server.printP(br);

  server.printP(Form_input_send);
  server.printP(Form_end);

  if (saved)
    server.printP(Config_set);

  server.printP(Page_end);
}

const char * eventName(int e)
{
  switch (e) {
    case ButtonEvent::StateChanged:
      return "StateChanged";
    case ButtonEvent::Pressed:
      return "Pressed";
    case ButtonEvent::Released:
      return "Released";
    case ButtonEvent::Repeat:
      return "Repeat";
    case ButtonEvent::Hold:
      return "Hold";
    case ButtonEvent::LongClick:
      return "LongClick";
    case ButtonEvent::Click:
      return "Click";
    case ButtonEvent::DoubleClick:
      return "DoubleClick";
  }
}

const char * actionName(int a)
{
  switch ((RelayAction::Action)a) {
    case RelayAction::Unassigned:
      return "Unassigned";
    case RelayAction::NoAction:
      return "NoAction";
    case RelayAction::On:
      return "On";
    case RelayAction::Off:
      return "Off";
    case RelayAction::Toggle:
      return "Toggle";
    case RelayAction::Value:
      return "Value";
    case RelayAction::IncValue:
      return "IncValue";
  };
}

void jpeventsCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  boolean params_present = false;
  JointPoint *b;
  bool saved = false;

  if (type == WebServer::HEAD)
  {
    server.httpSuccess();
    return;
  }

  if (type != WebServer::GET)
  {
    server.httpFail();
    return;
  }

  if (!tail_complete)
    Serialprint("Request buffer too small\n");

  // check for parameters
  if (strlen(url_tail)) {
    URLPARAM_RESULT rc;
    char name[NAMELEN];
    char value[VALUELEN];

    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc == URLPARAM_OK) {
        if (strcmp(name, "saved") == 0)
        {
          saved = true;
          break;
        }

        if (!strlen(value))
          continue;

        if (strcmp(name, "jp") == 0)
        {
          b = jpList.find(atoi(value));
          continue;
        }

        if (!b)
          continue;

        params_present = true;
        if (name[0] == 'a')
          b->eventAction((ButtonEvent::Type)atoi(name + 1)).action = (RelayAction::Action)atoi(value);
        else if (name[0] == 'p')
          b->eventAction((ButtonEvent::Type)atoi(name + 1)).param = atoi(value);
        else if (name[0] == 't')
          b->eventAction((ButtonEvent::Type)atoi(name + 1)).timeout = strtoul(value, NULL, 10);
        else if (name[0] == 'c') {
          b->eventAction((ButtonEvent::Type)atoi(name + 1)).condition = value;
          b->eventAction((ButtonEvent::Type)atoi(name + 1)).condition.trim();
        }
      }
    }
  }


  if (params_present)
  {
    jpList.saveConfig();
    server.httpSeeOther("/joints?saved=1");
    return;
  }

  if (!b)
  {
    server.httpNoContent();
    return;
  }

  server.httpSuccess();

  server.printP(Page_start);

  printHeader(server);

  server.printP(br);
  server.printP(br);

  server.printP(Form_eth_start1);
  server.print("jpevents");
  server.printP(Form_eth_start2);

  server.print("<input type=\"hidden\" name=\"jp\" value=\""); server.print(b->ID); server.println("\" />");

  server.printP(table_start);

  // print the header
  server.printP(table_tr_start);

  server.printP(table_th_start);
  server.printP(jEVENT);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(jACTION);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(jPARAM);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(jTIMEOUT);
  server.printP(table_th_end);
  server.printP(table_th_start);
  server.printP(jCONDITION);
  server.printP(table_th_end);

  server.printP(table_tr_end);

  for (int e = 0; e < ButtonEvent::EventsCount; e++)
  {
    JointPoint::EventAction a = b->eventAction((ButtonEvent::Type)e);

    server.printP(table_tr_start);

    server.printP(table_td_start);
    server.print(eventName(e));
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_sel_start);
    server.print("a"); server.print(e);
    server.printP(Form_sel_attr_end);
    for (int i = 0; i <= RelayAction::IncValue; i++)
    {
      server.printP(Form_sel_option_start); server.print(i);
      if (i == (int)a.action)
        server.printP(Form_sel_option_selected);
      else
        server.printP(Form_sel_attr_end);
      server.print(actionName(i));
      server.printP(Form_sel_option_end);
    }
    server.printP(Form_sel_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("p"); server.print(e);
    server.printP(Form_input_value); server.print(a.param);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.printP(Form_input_number_start);
    server.print("t"); server.print(e);
    server.printP(Form_input_value); server.print(a.timeout);
    server.printP(Form_input_min); server.print(0);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_td_start);
    server.print("<input type=\"text\" style=\"width: 240px;\" name=\"");
    server.print("c"); server.print(e);
    server.printP(Form_input_value); server.print(a.condition);
    server.printP(Form_input_end);
    server.printP(table_td_end);

    server.printP(table_tr_end);
  }

  // table end
  server.printP(table_end);

  server.printP(br);

  server.printP(Form_input_send);
  server.printP(Form_end);

  if (saved)
    server.printP(Config_set);

  server.printP(Page_end);
}

void setupWeb()
{
  loadEthernetConfig();
  
  Ethernet.begin(mac, ip);
  W5100.setRetransmissionTime(TCP_RETRANSMISSION_TIME);
  W5100.setRetransmissionCount(3);

  webserver.begin();
  
  Serial.print("Server started at ");
  Serial.println(Ethernet.localIP());

  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("config", &configCmd);
  webserver.addCommand("state", &stateCmd);
  webserver.addCommand("buttons", &buttonsCmd);
  webserver.addCommand("relays", &relaysCmd);
  webserver.addCommand("joints", &jointsCmd);
  webserver.addCommand("jpevents", &jpeventsCmd);
}
