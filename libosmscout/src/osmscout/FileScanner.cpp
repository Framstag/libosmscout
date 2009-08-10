/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscout/FileScanner.h>

#include <cassert>
#include <limits>

#include <osmscout/Util.h>

FileScanner::FileScanner()
 : file(NULL),
   hasError(true)
{
  // no code
}

FileScanner::~FileScanner()
{
  if (file!=NULL) {
    fclose(file);
  }
}

bool FileScanner::Open(const std::string& filename, bool readOnly)
{
  if (file!=NULL) {
    return false;
  }

  this->readOnly=readOnly;

  if (readOnly) {
    file=fopen(filename.c_str(),"rb");
  }
  else {
    file=fopen(filename.c_str(),"r+b");
  }

  hasError=file==NULL;

  return !hasError;
}

bool FileScanner::Close()
{
  bool result;

  if (file==NULL) {
    return false;
  }

  result=fclose(file)==0;

  if (result) {
    file=NULL;
  }

  return result;
}

bool FileScanner::HasError() const
{
  return file==NULL || hasError;
}

bool FileScanner::SetPos(long pos)
{
  if (file==NULL || hasError) {
    return false;
  }

  hasError=fseek(file,pos,SEEK_SET)!=0;

  return !hasError;
}

bool FileScanner::GetPos(long& pos)
{
  if (file==NULL || hasError) {
    return false;
  }

  pos=ftell(file);

  hasError=pos==-1;

  return !hasError;
}

bool FileScanner::Read(std::string& value)
{
  if (file==NULL || hasError) {
    return false;
  }

  value.clear();

  char character;

  hasError=fread(&character,sizeof(char),1,file)!=1;

  if (hasError) {
    return false;
  }

  while (character!='\0') {
    value.append(1,character);

    hasError=fread(&character,sizeof(char),1,file)!=1;

    if (hasError) {
      return false;
    }
  }

  return true;
}

bool FileScanner::Read(bool& boolean)
{
  if (file==NULL || hasError) {
    return false;
  }

  char value;

  hasError=fread(&value,sizeof(char),1,file)!=1;

  if (hasError) {
    return false;
  }

  boolean=value!=0;

  return true;
}

bool FileScanner::Read(unsigned long& number)
{
  if (file==NULL || hasError) {
    return false;
  }

  hasError=fread(&number,sizeof(unsigned long),1,file)!=1;
  /*
  unsigned long value=0;

  for (size_t i=0; i<sizeof(unsigned long); i++) {
    value=value | (buffer[offset+i] << (i*8));
  }*/

  return !hasError;
}

bool FileScanner::Read(unsigned int& number)
{
  if (file==NULL || hasError) {
    return false;
  }

  hasError=fread(&number,sizeof(unsigned int),1,file)!=1;

  /*
  unsigned int value=0;

  for (size_t i=0; i<sizeof(unsigned int); i++) {
    value=value | (buffer[offset+i] << (i*8));
  }*/

  return true;
}

bool FileScanner::ReadNumber(unsigned long& number)
{
  if (file==NULL || hasError) {
    return false;
  }

  char buffer;

  if (fread(&buffer,sizeof(char),1,file)!=1) {
    hasError=true;
    return false;
  }

  number=0;

  if (buffer==0) {
    return true;
  }
  else {
    size_t idx=0;

    while (true) {
      unsigned long add=(buffer & 0x7f);

      number=number | (add << (idx*7));

      if ((buffer & 0x80)==0) {
        return true;
      }

      if (fread(&buffer,sizeof(char),1,file)!=1) {
        hasError=true;
        return false;
      }

      idx++;
    };
  }
}

bool FileScanner::ReadNumber(unsigned int& number)
{
  unsigned long value;

  if (!ReadNumber(value)) {
    return false;
  }

  if (value>(unsigned long)std::numeric_limits<int>::max()) {
    hasError=true;
    return false;
  }

  number=(unsigned int)value;

  return true;
}

bool FileScanner::ReadNumber(NodeCount& number)
{
  unsigned long value;

  if (!ReadNumber(value)) {
    return false;
  }

  if (value>(unsigned long)std::numeric_limits<NodeCount>::max()) {
    hasError=true;
    return false;
  }

  number=(NodeCount)value;

  return true;
}

bool FileScanner::Write(bool boolean)
{
  if (file==NULL || hasError || readOnly) {
    hasError=true;
    return false;
  }

  char value=boolean ? 1 : 0;

  hasError=fwrite((const char*)&value,sizeof(char),1,file)!=1;

  return !hasError;
}

bool FileScanner::Write(unsigned long number)
{
  if (file==NULL || hasError || readOnly) {
    hasError=true;
    return false;
  }

  hasError=fwrite((const char*)&number,sizeof(unsigned long),1,file)!=1;

  return !hasError;

  /*
  char buffer[sizeof(unsigned long)];

  for (size_t i=0; i<sizeof(unsigned long); i++) {
    buffer[i]=(number >> (i*8)) && 0xff;
  }

  hasError=fwrite(buffer,sizeof(char),sizeof(unsigned long),file)!=sizeof(unsigned long);

  return !hasError;*/
}

bool FileScanner::Write(unsigned int number)
{
  if (file==NULL || hasError || readOnly) {
    hasError=true;
    return false;
  }

  hasError=fwrite((const char*)&number,sizeof(unsigned int),1,file)!=1;

  return !hasError;

  /*
  char buffer[sizeof(unsigned int)];

  for (size_t i=0; i<sizeof(unsigned int); i++) {
    buffer[i]=(number >> (i*8)) && 0xff;
  }

  hasError=fwrite(buffer,sizeof(char),sizeof(unsigned int),file)!=sizeof(unsigned int);

  return !hasError;*/
}

