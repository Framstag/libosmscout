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

#include <osmscout/FileReader.h>

#include <cassert>
#include <limits>

#include <osmscout/Util.h>

FileReader::FileReader()
 : file(NULL),
   buffer(NULL),
   offset(0),
   hasError(true)
{
  // no code
}

FileReader::~FileReader()
{
  if (file!=NULL) {
    fclose(file);
  }
}

bool FileReader::Open(const std::string& filename)
{
  if (file!=NULL || buffer!=NULL) {
    return false;
  }

  file=fopen(filename.c_str(),"rb");

  hasError=file==NULL;

  return !hasError;
}

bool FileReader::Close()
{
  bool result;

  if (file==NULL) {
    return false;
  }

  delete buffer;
  buffer=NULL;

  result=fclose(file)==0;

  if (result) {
    file=NULL;
  }

  return result;
}

bool FileReader::IsOpen() const
{
  return file!=NULL;
}

bool FileReader::HasError() const
{
  return file==NULL || buffer==NULL || hasError;
}

bool FileReader::ReadFileToBuffer()
{
  if (file==NULL) {
    return false;
  }

  delete buffer;
  buffer=NULL;

  long size;

  if (fseek(file,0L,SEEK_END)!=0) {
    hasError=true;
    return false;
  }

  size=ftell(file);

  if (size==-1) {
    hasError=true;
    return false;
  }

  if (fseek(file,0L,SEEK_SET)!=0) {
    hasError=true;
    return false;
  }

  buffer=new char[size];

  if (fread(buffer,sizeof(char),(size_t)size,file)!=(size_t)size) {
    hasError=true;

    delete buffer;
    buffer=NULL;

    return false;
  }

  this->size=(size_t)size;
  offset=0;
  hasError=false;

  return true;
}

bool FileReader::ReadPageToBuffer(unsigned long offset, unsigned long size)
{
  assert(offset>=0);
  assert(size>0);

  if (file==NULL) {
    return false;
  }

  delete buffer;
  buffer=NULL;

  if (fseek(file,offset,SEEK_SET)!=0) {
    hasError=true;
    return false;
  }

  buffer=new char[size];

  size_t res=0;

  if ((res=fread(buffer,sizeof(char),(size_t)size,file))!=(size_t)size) {
    hasError=true;

    delete buffer;
    buffer=NULL;

    return false;
  }

  this->size=(size_t)size;
  this->offset=0;
  hasError=false;

  return true;
}

bool FileReader::Read(std::string& value)
{
  if (file==NULL || buffer==NULL || hasError || offset>=size) {
    hasError=true;
    return false;
  }

  value.clear();

  while (offset<size && buffer[offset]!='\0') {
    value.append(1,buffer[offset]);

    offset++;
  }
  offset++;

  return true;
}

bool FileReader::Read(bool& boolean)
{
  if (file==NULL || buffer==NULL || hasError || offset+sizeof(char)>size) {
    hasError=true;
    return false;
  }

  boolean=buffer[offset]!=0;

  offset+=sizeof(char);

  return true;
}

bool FileReader::Read(unsigned long& number)
{
  if (file==NULL || buffer==NULL || hasError || offset+sizeof(unsigned long)>size) {
    hasError=true;
    return false;
  }

  number=*(unsigned long*)&buffer[offset];

  /*
  unsigned long value=0;

  for (size_t i=0; i<sizeof(unsigned long); i++) {
    value=value | (buffer[offset+i] << (i*8));
  }*/

  offset+=sizeof(unsigned long);

  return true;
}

bool FileReader::Read(unsigned int& number)
{
  if (file==NULL || buffer==NULL || hasError || offset+sizeof(unsigned int)>size) {
    hasError=true;
    return false;
  }

  number=*(unsigned int*)&buffer[offset];

  /*
  unsigned int value=0;

  for (size_t i=0; i<sizeof(unsigned int); i++) {
    value=value | (buffer[offset+i] << (i*8));
  }*/

  offset+=sizeof(unsigned int);

  return true;
}

bool FileReader::ReadNumber(unsigned long& number)
{
  if (file==NULL || buffer==NULL || hasError || offset>=size) {
    hasError=true;
    return false;
  }

  size_t bytes;

  if (DecodeNumber(&buffer[offset],number,bytes)) {
    offset+=bytes;

    return true;
  }
  else {
    hasError=true;

    return false;
  }
}

bool FileReader::ReadNumber(unsigned int& number)
{
  if (file==NULL || buffer==NULL || hasError || offset>=size) {
    hasError=true;
    return false;
  }

  unsigned long value;
  size_t        bytes;

  if (DecodeNumber(&buffer[offset],value,bytes)) {
    offset+=bytes;

    if (value>(unsigned long)std::numeric_limits<unsigned int>::max()) {
      return false;
    }

    number=(unsigned int)value;

    return true;
  }
  else {
    hasError=true;

    return false;
  }
}

bool FileReader::ReadNumber(NodeCount& number)
{
  if (file==NULL || buffer==NULL || hasError || offset>=size) {
    hasError=true;
    return false;
  }

  unsigned long value;
  size_t        bytes;

  if (DecodeNumber(&buffer[offset],value,bytes)) {
    offset+=bytes;

    if (value>(unsigned long)std::numeric_limits<NodeCount>::max()) {
      return false;
    }

    number=(unsigned int)value;

    return true;
  }
  else {
    hasError=true;

    return false;
  }
}

