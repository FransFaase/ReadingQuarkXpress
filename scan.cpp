/* RQXD -- Reading Quark Xpress Documents 
   Copyright (C) 2002 Frans Faase

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version, but
   
        WITH THE EXPLICT AMMENDMENT:
        that any additional discoveries you make about the Quark Xpress 
        file formats are made public under the GNU General Public License.
        (The most convient manner to do this, it to mail me the updated
        sources, and I will publish them on my website.)

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

GNU General Public License:
   http://home.planet.nl/~faase009/GNU.txt
   
Latest version and any additional documentation can be found on:
   http://home.planet.nl/~faase009/QX.html   
*/


#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "stddef.c"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef USE_MMFILE
#include "MMFile.cpp"
#else
#include "MMFileDummy.cpp"
#endif
#include "CQXDoc.cpp"
#include "CBuf.cpp"
word dump_br = 0;
word echo_br = 0;
word dump = 0;
word echo = 0;
#include "CReadBuf.cpp"
#include "scanQXDoc.cpp"
#include "DumpQXDoc.cpp"
#include "CDatabase.cpp"
#include "FrameGeom.cpp"


void DumpBookToHTML(CBook *book, FILE *fout)
{
	FOREACH_IN_LIST_N(CFrame, frame, 
	                  book->first_frame_reading_order, next_reading_order)
	{
		fprintf(fout, "<FONT COLOR=\"904090\">%s</FONT>",
				GeomTypeName(frame->geom_type));
		dump_text_fragment_to_HTML(frame->text_fragment.A(), fout);
		fprintf(fout, "<BR>");
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{	printf("Usages:\n\n   %s <filename>\n\n", argv[0]);
		return 0;
	}
		
	char *fn = argv[1];

	// Open a new database file
	if (!mmfile.Open("Mydatabase.dat", 40000000))
		return 0;
	
	CDatabase *database = (CDatabase*)mmfile.FirstObject();
	if (database == NULL)
	{	
		mmfile.Reset();
		database = new CDatabase;
	}

	// Append a book to the database
	CBook *book = database->AppendNewBook("a book");
	
	// For each document (in this example just one):

		// Append it to a book under the given file name
		CQXDoc *doc = book->AppendNewQXDoc(fn);
		// Scan it
		scan_QX_doc(doc);

		// Analyze geometrical properties of the frames
		// and set the geometrical type accordingly
		AnalyzeFrameGeometry(doc);
		
		// (Optional) Dump the contents of the file with:
		//   dumpQXDoc_to_HTML(doc, stdout);
		// Or:
		//   dumpQXDoc(doc, stdout);

	// -- finish reading all the documents of the book

	// Set all the frame in reading order.
	SetReadingOrder(book);

	// Dump all the documents of the file to HTML
	DumpBookToHTML(book, stdout);
			
	mmfile.Flush();
	mmfile.Close();
	
	return 0;
}


