/*
 * tarball.h
 *
 *  Created on: Jul 28, 2010
 *      Author: Pierre Lindenbaum PhD
 *              plindenbaum@yahoo.fr
 *              http://plindenbaum.blogspot.com
 *
 * MIT license
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

#ifndef LINDENB_IO_TARBALL_H_
#define LINDENB_IO_TARBALL_H_

#include <iostream>


/**
 *  A Tar Archive
 */
class Tar
    {
    private:
	bool _finished;
    protected:
	std::ostream& out;
	void _init(void* header);
	void _checksum(void* header);
	void _size(void* header,unsigned long fileSize);
	void _filename(void* header,const char* filename);
	void _endRecord(std::size_t len);
    public:
	Tar(std::ostream& out);
	virtual ~Tar();
	/** writes 2 empty blocks. Should be always called before closing the Tar file */
	void finish();
	void put(const char* filename,const std::string& s);
	void put(const char* filename,const char* content);
	void put(const char* filename,const char* content,std::size_t len);
	void putFile(const char* filename,const char* nameInArchive);
    };


#endif
