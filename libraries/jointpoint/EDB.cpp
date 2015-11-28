/*
  EDB.cpp
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

 #include "EDB.h"

/**************************************************/
// private functions

// low level byte write
void EDB::edbWrite(uint32_t ee, const byte* p, unsigned int recsize)
{
  for (unsigned int i = 0; i < recsize; i++)
    _write_byte(ee++, *p++);
}

// low level byte read
void EDB::edbRead(uint32_t ee, byte* p, unsigned int recsize)
{  
  for (unsigned i = 0; i < recsize; i++)
    *p++ = _read_byte(ee++);
}

// writes EDB_Header
void EDB::writeHead()
{
  edbWrite(EDB_head_ptr, EDB_REC EDB_head, (uint32_t)sizeof(EDB_Header));
}

// reads EDB_Header
void EDB::readHead()
{
  edbRead(EDB_head_ptr, EDB_REC EDB_head, (uint32_t)sizeof(EDB_Header));
}

/**************************************************/
// public functions

EDB::EDB(EDB_Write_Handler *w, EDB_Read_Handler *r)
{
  _write_byte = w;
  _read_byte = r;
}

// creates a new table and sets header values 
EDB_Status EDB::create(uint32_t head_ptr, uint32_t tablesize, unsigned int recsize)
{
  EDB_head_ptr = head_ptr;
  EDB_table_ptr = sizeof(EDB_Header) + EDB_head_ptr;
  EDB_head.n_recs = 0;
  EDB_head.rec_size = recsize;
  EDB_head.table_size = tablesize;
  writeHead();
  return EDB_OK;
}

// reads an existing edb header at a given recno and sets header values
EDB_Status EDB::open(uint32_t head_ptr)
{
  EDB_head_ptr = head_ptr;
  EDB_table_ptr = sizeof(EDB_Header) + EDB_head_ptr;
  readHead();
  return EDB_OK;
}

// writes a record to a given recno
EDB_Status EDB::writeRec(uint32_t recno, const EDB_Rec rec)
{
  edbWrite(EDB_table_ptr + ((recno - 1) * EDB_head.rec_size), rec, EDB_head.rec_size);
  return EDB_OK;
}

// reads a record from a given recno
EDB_Status EDB::readRec(uint32_t recno, EDB_Rec rec)
{
  if (recno < 1 || recno > EDB_head.n_recs) return EDB_OUT_OF_RANGE;
  edbRead(EDB_table_ptr + ((recno - 1) * EDB_head.rec_size), rec, EDB_head.rec_size);
  return EDB_OK;
}

// Deletes a record at a given recno
// Becomes more inefficient as you the record set increases and you delete records 
// early in the record queue.
EDB_Status EDB::deleteRec(uint32_t recno)
{
  if (recno < 0 || recno > EDB_head.n_recs) return  EDB_OUT_OF_RANGE;
  EDB_Rec rec = (byte*)malloc(EDB_head.rec_size);
  for (uint32_t i = recno + 1; i <= EDB_head.n_recs; i++)
  {
    readRec(i, rec);
    writeRec(i - 1, rec);
  }  
  free(rec);
  EDB_head.n_recs--;
  writeHead();
  return EDB_OK;
}

// Inserts a record at a given recno, increasing all following records' recno by 1.
// This function becomes increasingly inefficient as it's currently implemented and 
// is the slowest way to add a record.
EDB_Status EDB::insertRec(uint32_t recno, EDB_Rec rec)
{
  if (count() == limit()) return EDB_TABLE_FULL;
  if (count() > 0 && (recno < 0 || recno > EDB_head.n_recs)) return EDB_OUT_OF_RANGE;
  if (count() == 0 && recno == 1) return appendRec(rec);

  EDB_Rec buf = (byte*)malloc(EDB_head.rec_size);
  for (uint32_t i = EDB_head.n_recs; i >= recno; i--)
  {
    readRec(i, buf);
    writeRec(i + 1, buf);
  }
  free(buf);
  writeRec(recno, rec);  
  EDB_head.n_recs++;
  writeHead();
  return EDB_OK;
}

// Updates a record at a given recno
EDB_Status EDB::updateRec(uint32_t recno, EDB_Rec rec)
{
  if (recno < 0 || recno > EDB_head.n_recs) return EDB_OUT_OF_RANGE;
  writeRec(recno, rec);  
  return EDB_OK;
}

// Adds a record to the end of the record set.
// This is the fastest way to add a record.
EDB_Status EDB::appendRec(EDB_Rec rec)
{
  if (EDB_head.n_recs + 1 > limit()) return EDB_TABLE_FULL;
  EDB_head.n_recs++;
  writeRec(EDB_head.n_recs,rec);
  writeHead();
  return EDB_OK;
}

// returns the number of queued items
uint32_t EDB::count()
{
  return EDB_head.n_recs;
}

// returns the maximum number of items that will fit into the queue
uint32_t EDB::limit()
{
   return (EDB_head.table_size - sizeof(EDB_Header)) / EDB_head.rec_size;
}

// truncates the queue by resetting the internal pointers
void EDB::clear()
{
  readHead();
  create(EDB_head_ptr, EDB_head.table_size, EDB_head.rec_size);
}
