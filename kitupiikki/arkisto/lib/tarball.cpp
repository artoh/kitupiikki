/*
 * tarball.cpp
 *
 *  Created on: Jul 28, 2010
 *      Author: Pierre Lindenbaum PhD
 *              plindenbaum@yahoo.fr
 *
 *
 *
 */

//The MIT License

//Copyright 2017 Pierre Lindenbaum
//https://github.com/lindenb/cclindenb

//Permission is hereby granted, free of charge, to any person obtaining
//a copy of this software and associated documentation files (the
//"Software"), to deal in the Software without restriction, including
//without limitation the rights to use, copy, modify, merge, publish,
//distribute, sublicense, and/or sell copies of the Software, and to
//permit persons to whom the Software is furnished to do so, subject to
//the following conditions:

//The above copyright notice and this permission notice shall be
//included in all copies or substantial portions of the Software.

//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
//LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
//WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include <cstdio>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <unistd.h>
#include <iostream>
#include "tarball.h"

#include <QDebug>

#define TARHEADER static_cast<PosixTarHeader*>(header)

#define THROW(msg)  { qDebug() << __LINE__ << msg; return; }


struct PosixTarHeader
	{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char typeflag[1];
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char pad[12];
	};



void Tar::_init(void* header)
    {
    std::memset(header,0,sizeof(PosixTarHeader));
    std::strcpy(TARHEADER->magic,"ustar");
    std::strcpy(TARHEADER->version, " ");
    std::sprintf(TARHEADER->mtime,"%011lo",time(NULL));
    std::sprintf(TARHEADER->mode,"%07o",0644);

#ifdef Q_OS_LINUX
    char * s = ::getlogin();
    if(s!=NULL)  std::snprintf((char*) header,32,"%s",s);
#endif

    std::sprintf(TARHEADER->gname,"%s","users");
    }

void Tar::_checksum(void* header)
    {
    unsigned int sum = 0;
    char *p = (char *) header;
    char *q = p + sizeof(PosixTarHeader);
    while (p < TARHEADER->checksum) sum += *p++ & 0xff;
    for (int i = 0; i < 8; ++i)  {
	  sum += ' ';
	  ++p;
	}
    while (p < q) sum += *p++ & 0xff;

    std::sprintf(TARHEADER->checksum,"%06o",sum);
    }

void Tar::_size(void* header,unsigned long fileSize)
    {
    std::sprintf(TARHEADER->size,"%011llo",(long long unsigned int)fileSize);
    }

void Tar::_filename(void* header,const char* filename)
    {
    if(filename==NULL || filename[0]==0 || std::strlen(filename)>=100)
	{
	THROW("invalid archive name \"" << filename << "\"");
	}
    std::snprintf(TARHEADER->name,100,"%s",filename);
    }

void Tar::_endRecord(std::size_t len)
    {
    char c='\0';
    while((len%sizeof(PosixTarHeader))!=0)
	    {
	    out.write(&c,sizeof(char));
	    ++len;
	    }
    }


Tar::Tar(std::ostream& out):_finished(false),out(out)
    {
    if(sizeof(PosixTarHeader)!=512)
	{
	THROW(sizeof(PosixTarHeader));
	}
    }

Tar::~Tar()
    {
    if(!_finished)
	{
     qDebug() << "[warning]tar file was not finished.";
	}
    }

/** writes 2 empty blocks. Should be always called before closing the Tar file */
void Tar::finish()
    {
    _finished=true;
    //The end of the archive is indicated by two blocks filled with binary zeros
    PosixTarHeader header;
    std::memset((void*)&header,0,sizeof(PosixTarHeader));
    out.write((const char*)&header,sizeof(PosixTarHeader));
    out.write((const char*)&header,sizeof(PosixTarHeader));
    out.flush();
    }

void Tar::put(const char* filename,const std::string& s)
    {
    put(filename,s.c_str(),s.size());
    }
void Tar::put(const char* filename,const char* content)
    {
    put(filename,content,std::strlen(content));
    }

void Tar::put(const char* filename,const char* content,std::size_t len)
    {
    PosixTarHeader header;
    _init((void*)&header);
    _filename((void*)&header,filename);
    header.typeflag[0]=0;
    _size((void*)&header,len);
    _checksum((void*)&header);
    out.write((const char*)&header,sizeof(PosixTarHeader));
    out.write(content,len);
    _endRecord(len);
    }

void Tar::putFile(const char* filename,const char* nameInArchive)
    {
    char buff[BUFSIZ];
    std::FILE* in=std::fopen(filename,"rb");
    if(in==NULL)
	{
	THROW("Cannot open " << filename << " "<< std::strerror(errno));
	}
    std::fseek(in, 0L, SEEK_END);
    long int len= std::ftell(in);
    std::fseek(in,0L,SEEK_SET);

    PosixTarHeader header;
    _init((void*)&header);
    _filename((void*)&header,nameInArchive);
    header.typeflag[0]=0;
    _size((void*)&header,len);
    _checksum((void*)&header);
    out.write((const char*)&header,sizeof(PosixTarHeader));


    std::size_t nRead=0;
    while((nRead=std::fread(buff,sizeof(char),BUFSIZ,in))>0)
	{
	out.write(buff,nRead);
	}
    std::fclose(in);

    _endRecord(len);
    }

