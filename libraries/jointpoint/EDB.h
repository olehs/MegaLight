/*
  EDB.h
  Extended Database Library for Arduino

  http://www.arduino.cc/playground/Code/ExtendedDatabaseLibrary

  Based on code from:
  Database library for Arduino 
  Written by Madhusudana das
  http://www.arduino.cc/playground/Code/DatabaseLibrary

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef EDB_PROM
#define EDB_PROM

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

struct EDB_Header
{
  uint32_t n_recs;
  unsigned int rec_size;
  uint32_t table_size;
};

typedef enum EDB_Status { 
                          EDB_OK,
                          EDB_OUT_OF_RANGE,
                          EDB_TABLE_FULL
                        };

typedef byte* EDB_Rec;

#define EDB_REC (byte*)(void*)&

class EDB {
  public:
    typedef void EDB_Write_Handler(uint32_t, const uint8_t);
    typedef uint8_t EDB_Read_Handler(uint32_t);
    EDB(EDB_Write_Handler *, EDB_Read_Handler *);
    EDB_Status create(uint32_t, uint32_t, unsigned int);
    EDB_Status open(uint32_t);
    EDB_Status readRec(uint32_t, EDB_Rec);
    EDB_Status deleteRec(uint32_t);	
    EDB_Status insertRec(uint32_t, const EDB_Rec);
    EDB_Status updateRec(uint32_t, const EDB_Rec);
    EDB_Status appendRec(EDB_Rec rec);
    uint32_t limit();
    uint32_t count();
    void clear();
  private:
    uint32_t EDB_head_ptr;
    uint32_t EDB_table_ptr;
    EDB_Write_Handler *_write_byte;
    EDB_Read_Handler *_read_byte;
    EDB_Header EDB_head;
    void edbWrite(uint32_t ee, const byte* p, unsigned int);
    void edbRead(uint32_t ee, byte* p, unsigned int);
    void writeHead();
    void readHead();
    EDB_Status writeRec(uint32_t, const EDB_Rec);
};

extern EDB edb;

#endif
