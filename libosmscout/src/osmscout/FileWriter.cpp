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

#include <osmscout/FileWriter.h>

#include <osmscout/Util.h>

namespace osmscout {

  FileWriter::FileWriter()
   : file(NULL),
     hasError(true)
  {
    // no code
  }

  FileWriter::~FileWriter()
  {
    if (file!=NULL) {
      fclose(file);
    }
  }

  bool FileWriter::Open(const std::string& filename)
  {
    if (file!=NULL) {
      return false;
    }

    file=fopen(filename.c_str(),"w+b");

    hasError=file==NULL;

    return !hasError;
  }

  bool FileWriter::IsOpen() const
  {
    return file!=NULL;
  }

  bool FileWriter::Close()
  {
    if (file==NULL) {
      return false;
    }

    hasError=fclose(file)!=0;

    if (!hasError) {
      file=NULL;
    }

    return !hasError;
  }

  bool FileWriter::HasError() const
  {
    return file==NULL || hasError;
  }

  bool FileWriter::GetPos(long& pos)
  {
    if (file==NULL || hasError) {
      return false;
    }

    pos=ftell(file);

    hasError=pos==-1;

    return !hasError;
  }

  bool FileWriter::SetPos(long pos)
  {
    if (file==NULL || hasError) {
      return false;
    }

    hasError=fseek(file,pos,SEEK_SET)!=0;

    return !hasError;
  }

  bool FileWriter::Write(const std::string& value)
  {
    size_t length=value.length();

    hasError=fwrite(value.c_str(),sizeof(char),length+1,file)!=length+1;

    return !hasError;
  }

  bool FileWriter::Write(bool boolean)
  {
    if (file==NULL || hasError) {
      return false;
    }

    char value=boolean ? 1 : 0;

    hasError=fwrite((const char*)&value,sizeof(char),1,file)!=1;

    return !hasError;
  }

  bool FileWriter::Write(unsigned long number)
  {
    if (file==NULL || hasError) {
      return false;
    }

    char     buffer[sizeof(unsigned long)];
    unsigned long mask=0xff;

    for (size_t i=0; i<sizeof(unsigned long); i++) {
      buffer[i]=(number >> (i*8)) & mask;
    }

    hasError=fwrite(buffer,sizeof(char),sizeof(unsigned long),file)!=sizeof(unsigned long);

    return !hasError;
  }

  bool FileWriter::Write(unsigned int number)
  {
    if (file==NULL || hasError) {
      return false;
    }

    char         buffer[sizeof(unsigned int)];
    unsigned int mask=0xff;

    for (size_t i=0; i<sizeof(unsigned int); i++) {
      buffer[i]=(number >> (i*8)) & mask;
    }

    hasError=fwrite(buffer,sizeof(char),sizeof(unsigned int),file)!=sizeof(unsigned int);

    return !hasError;
  }

  /**
    Write a numeric value to the file using same internal encoding
    to reduce storage size. Note that this works only if the average number
    is small. Don't use this method for storing ids, latitude or longitude.
    */
  bool FileWriter::WriteNumber(unsigned long number)
  {
    char   buffer[5];
    size_t bytes;

    if (file==NULL || hasError) {
      return false;
    }

    if (!EncodeNumber(number,5,buffer,bytes)) {
      hasError=true;
      return false;
    }

    hasError=fwrite(buffer,sizeof(char),bytes,file)!=bytes;

    return !hasError;
  }
}

