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

word version = 0;
#define VER_1    1
#define VER_2    2
#define VER_3    3
#define VER_4    4



#define debug_mode 1
#define DEBUG_MODE ((mode & debug_mode)!=0)
#define alloc_mode 2
#define ALLOC_MODE ((mode & alloc_mode)!=0)
#define add_mode   4
#define ADD_MODE ((mode & add_mode)!=0)


word error = 0;

void print_as_bits(lword v, word nr_bits)
{
	for ( ; nr_bits > 0; nr_bits--)
		printf("%c", v & (1 << (nr_bits-1)) ? '1' : '0');
}

void dump_bytes(CReadBuf &read_buf, lword len)
{
	printf("[");
	if(len>5000)len=5000;
	for (lword i = 0; i < len; i++)
	{	byte ch = read_buf[i];
		if(i == read_buf.pos) printf("##");
		printf(ch > ' ' && ch < 127?" %c":"%02X", ch);
	}
	printf("]");
}

void dump_bytes_hex(CReadBuf &read_buf, lword len)
{
	printf("[");
	if(len>5000)len=5000;
	for (lword i = 0; i < len; i++)
	{	byte ch = read_buf[i];
		if(i == read_buf.pos) printf("##");
		printf("%02X", ch);
	}
	printf("]");
}

void dump_bytes_from_cur_pos(CReadBuf &read_buf, lword len)
{
	printf("[");
	if(len>5000)len=5000;
	for (lword i = 0; i < len && read_buf.pos+i < read_buf.Length(); i++)
	{	byte ch = read_buf[read_buf.pos + i];
		if(i == read_buf.pos) printf("##");
		printf(ch > ' ' && ch < 127?" %c":"%02X", ch);
	}
	printf("]");
}

bool scan_chain_from_with_mode(int mode,
							   lword start_block, lword start_len, 
							   CReadBufWithBlocks &read_buf,
							   CBuf &data_buf)
{
	lword max_block_nr = read_buf.Length() / 256 + 1;
	lword cur_block = start_block;
	lword len = start_len;
	lword data_pos = (cur_block - 1) * 256;
	lword data_len = len * 256 - 4;

	read_buf.Reset();
	
	if (DEBUG_MODE)
	{	printf("\nLength = %ld, max block nr = %ld remains %ld, start = %ld, len=%ld", 
			   read_buf.Length(), 
			   max_block_nr, 
			   read_buf.Length() % 256,
			   start_block, start_len);
	 	fflush(stdout);    
	}
	if (ADD_MODE)
		data_buf.Allocate();
	
	for(;;)
	{
		if (ALLOC_MODE)
			data_buf.Extend(data_len);
		if (ADD_MODE)
			data_buf.AddBytes(read_buf.GetBytes(data_pos, data_len), data_len);
		if (dump_br)
		{	for (word i = 0; i < data_len; i++)
			{	if (i % 256 == 0) printf("\n");
				printf("%02X", read_buf[data_pos + i]);
			}
		}
		if (ALLOC_MODE && !read_buf.IsFree(cur_block, len))
		{	
			if (DEBUG_MODE)
			{	printf(" could not claim %ld blocks from %ld\n", 
					   len, cur_block);
				fflush(stdout);
			}
			return (version == VER_4) ? TRUE : FALSE;
		}
		if (ADD_MODE && !read_buf.ClaimBlocks(cur_block, len))
		{	printf("\nError: ClaimBlocks returned FALSE");
			return TRUE;
		}

		lword cur_block_end = cur_block + len;
		if (dump_br) printf("\n cb=%ld.%ld", cur_block, len);
		if (dump_br) printf(".%ld", (cur_block_end - 1)*256 - 4);
		long next_block = read_buf.GetLWord((cur_block_end - 1)*256 - 4);
		if (dump_br) printf(" nb=%lx", next_block);
		
		if (next_block == 0)
			// -- end of the main chain
			break; 

		if (next_block < 0)
		{
			next_block = -next_block;
			if (dump_br) printf(" neg nextblock = %ld", next_block);

			if ((lword)next_block <= max_block_nr)
			{	len = read_buf.GetWord((lword)(next_block-1)*256);

				data_pos = (lword)(next_block-1)*256 + 2;
				data_len = len * 256 - 6;
			}
		}
		else
		{
			if (dump_br) printf(" pos nextblock = %ld", next_block);
			len = 1;
			data_pos = (lword)(next_block-1)*256;
			data_len = 256 - 4;
		}
		if (dump_br) printf(" %ld+%ld=%ld\n", data_pos, data_len, data_pos + data_len);
		
		if ((lword)next_block + len-1 > max_block_nr)
		{
			if (DEBUG_MODE)
			{	printf(" illegal block_end at %ld: %ld (%ld) is after end\n", 
					   cur_block_end, next_block, len);
				fflush(stdout);
			}
			return TRUE;
		}

		if ((lword)next_block < cur_block)
		{
			if (DEBUG_MODE)
			{	printf(" illegal block_end at %ld: %ld before cur %ld\n", 
			           cur_block_end, next_block, cur_block);
				fflush(stdout);
			}
			return FALSE;
		}

		if (DEBUG_MODE && (lword)next_block != cur_block_end)
		{	printf(" %ld-%ld", cur_block_end, next_block);
			fflush(stdout);
		}
		/*{	word nr;
			for (nr = cur_block_end; nr < next_block; nr++)
				printf(" %d", nr);
		}*/
		cur_block = (lword)next_block;
	}

	if (DEBUG_MODE)
		printf(" Done\n");
	
	if (ADD_MODE)
		return data_buf.Full();
	
	return TRUE;
}

bool scan_chain_from(lword start_block, lword start_len, 
					 CReadBufWithBlocks &read_buf,
					 CBuf &data_buf)
{
	if (!scan_chain_from_with_mode(alloc_mode|(echo_br?debug_mode:0), start_block, start_len, read_buf, data_buf))
	{	scan_chain_from_with_mode(debug_mode, start_block, start_len, read_buf, data_buf);
		return FALSE;
	}
	else if (!scan_chain_from_with_mode(add_mode, start_block, start_len, read_buf, data_buf))
	{
		printf("\nInternal error\n");
		exit(1);
	}
	return TRUE;
}                     


bool scan_main_chain(CReadBufWithBlocks &read_buf, CBuf &main_chain_buf)
{
	return scan_chain_from(3, 1, read_buf, main_chain_buf);
}                     


bool printable_string(char *s)
{
	for ( ;*s;s++)
		if (*s < ' '&& *s >= 127)
			return FALSE;
	return TRUE;
}

#define _SUB_BUFFER(B,S,L,N) \
	CReadBuf S(B,B.pos,L,N);B.pos+=L;
#define _SAFE_SUB_BUFFER(B,S,L,N) \
    _ASSERT(B.pos+L<=B.Length(),B,("Sub buffer too large %ld+%ld>%ld", B.pos, L, B.Length()))\
    _SUB_BUFFER(B,S,L,N)
#define _ASSERT(C,B,M) \
	if(!(C)){\
		if(error||echo)printf("\nERROR: ");\
		if(echo||dump){printf M;dump_bytes_from_cur_pos(B,1000);}\
		error=!error;return FALSE;}
#define _ASSERTB(B) \
	if(B.Error()){\
		if(echo||dump)dump_bytes_from_cur_pos(B,1000);\
		error=!error;return FALSE;}
#define _ASSERT_MULTIPLE(L,M,B) _ASSERT(L%M==0,B,("Not a multiple of "#M))
#define _VAR2(T,M,F,C,V,B,N) T V=B.M();if(dump||echo)printf(" "N"="F,C V,C V);_ASSERTB(B)
#define _VAR(T,M,F,C,V,B,N) T V=B.M;if(dump||echo)printf(" "N"="F,C V);_ASSERTB(B)
#define _SKIPBYTE(B) {_VAR2(byte,ReadByte,"%3d [%02X]",(byte),v,B,"b")}
#define _SKIPWORD(B) {_VAR2(word,ReadWord,"%5d [%04X]",(word),v,B,"w")}
#define _SKIPLWORD(B) {_VAR2(lword,ReadLWord,"%11ld [%08lX]",(lword),v,B,"l")}
#define _SKIPBYTES(B,A) if(dump||echo){printf(" S:%ld ",(lword)A);if(dump)dump_bytes_from_cur_pos(B,A);}B.SkipBytes(A);_ASSERTB(B)
#define _SKIPBYTES_S(B,A) if(dump||echo){printf(" S:%ld ",(lword)A);dump_bytes_from_cur_pos(B,A);}B.SkipBytes(A);_ASSERTB(B)
#define _BYTE(V,B,N) _VAR2(,ReadByte,"%3d [%02X]",(byte),V,B,N)
#define _WORD(V,B,N) _VAR2(,ReadWord,"%5d [%04X]",(word),V,B,N)
#define _LWORD(V,B,N) _VAR2(,ReadLWord,"%11ld [%08lX]",(lword),V,B,N)
#define _VARBYTE(V,B,N) _VAR2(byte,ReadByte,"%3d [%02X]",(byte),V,B,N)
#define _VARWORD(V,B,N) _VAR2(word,ReadWord,"%5d [%04X]",(word),V,B,N)
#define _VARLWORD(V,B,N) _VAR2(lword,ReadLWord,"%11ld [%08lX]",(lword),V,B,N)
#define _VARPASCALSTRING(V,B,N) _VAR(char *,ReadPascalString(),"|%s|",,V,B,N)
#define _VARPASCAL2STRING(V,B,N) _VAR(char *,ReadPascal2String(),"|%s|",,V,B,N)
#define _VARPASCALFIXSTRING(V,B,N,L) _VAR(char *,ReadPascalFixedString(L),"|%s|",,V,B,N)
#define _CALL(C,B) \
	{lword cur_pos=B.pos;\
	 if(!C){if(error){B.pos=cur_pos;B.Reset();echo+=10;C;echo-=10;}return FALSE;}}
#define _CALL_IC(C,B) \
	if(!C)return FALSE;
#define _PRINTF if (echo||dump) printf

#define _EXPECT2(T,M,F,G,V,B,N) \
	{	T v=B.M();if(dump||echo)printf(" "N"="F,v,v);\
		_ASSERTB(B)_ASSERT(v==V,B,(G,V))}
#define _EXPECTBYTE(V,B,N) _EXPECT2(byte,ReadByte,"%d [%02X]","Expect %d",V,B,N)
#define _EXPECTWORD(V,B,N) _EXPECT2(word,ReadWord,"%d [%04X]","Expect %d",V,B,N)
#define _EXPECTLWORD(V,B,N) _EXPECT2(lword,ReadLWord,"%ld [%08lX]","Expect %ld",V,B,N)

#define _DONE(B)  B.Done();_ASSERTB(B)

int open_level = 0;
      
#define _OPEN if(dump||echo)printf("(");
#define _OPEN_LEVEL_NAME(N) if(dump||echo){open_level++;printf("\n%*.*s"N"(",open_level,open_level,"");}
#define _OPEN_LEVEL _OPEN_LEVEL_NAME("")
#define _CLOSE if(dump||echo)printf(")");
#define _CLOSE_LEVEL if(dump||echo){open_level--;printf(")");}
#define _NEWLINE if(dump||echo){printf("\n%*.*s ",open_level,open_level,"");}
#define _RULE_NO_LEVEL(N,B) \
	if(dump||echo)printf(N"(len=%ld,pos=%ld ", B.Length(),B.pos);\
	/*if(dump)dump_bytes(B,B.Length());*/
#define _RULE(N,B) \
	if(dump||echo){open_level++;printf("\n%*.*s"N"(len=%ld,pos=%ld",open_level,open_level,"",B.Length(),B.pos);}
	/*if(dump)dump_bytes(B,B.Length());*/
#define _END_RULE_NO_LEVEL _CLOSE;return TRUE;
#define _END_RULE _CLOSE_LEVEL;return TRUE;


bool scan_font_families(CReadBuf &buffer, CQXDoc *doc)
{
	_RULE("font_families", buffer)

	_VARWORD(nr, buffer, "nr")
	
	for (word i = 0; i < nr; i++)
	{
		_OPEN_LEVEL
		_VARWORD(font_id, buffer, "font_id")
		if (version == VER_4)
		{	_VARWORD(w2, buffer, "w2")
		}
		_VARPASCALSTRING(family_name, buffer, "family_name")
		_ASSERT(printable_string(family_name), buffer, ("-- not printable"))
		_VARPASCALSTRING(name, buffer, "name")
		_ASSERT(printable_string(name), buffer, ("-- not printable"))
		
		doc->AppendNewFontFamily(font_id, family_name);
		
		FREE(family_name);
		FREE(name);
		
		_CLOSE_LEVEL
	}
	
	_DONE(buffer)
	_END_RULE
}

bool scan_fonts(CReadBuf &buffer)
{
	_RULE("fonts", buffer)

	_SKIPBYTES(buffer,12);

	while (!buffer.EOB())
	{
		_OPEN_LEVEL
		_VARWORD(x1, buffer, "x1");
		_VARWORD(x2, buffer, "x2");
		_VARWORD(font_id, buffer, "font id");
		_VARWORD(w1, buffer, "w1");
		_VARWORD(w2, buffer, "w2");
		_VARBYTE(nr0, buffer, "nr0");
		_VARBYTE(nr1, buffer, "nr1");
		_VARBYTE(nr2, buffer, "nr2");
		_VARBYTE(nr3, buffer, "nr3");
		_VARPASCALSTRING(s1, buffer, "s1");
		_ASSERT(printable_string(s1), buffer, ("-- not printable"))
		_VARPASCALSTRING(s2, buffer, "s2");
		_ASSERT(printable_string(s2), buffer, ("-- not printable"))
		_VARPASCALSTRING(s3, buffer, "s3");
		_ASSERT(printable_string(s3), buffer, ("-- not printable"))
		
		FREE(s1);
		FREE(s2);
		FREE(s3);
		_CLOSE_LEVEL
	}
		/*
		font = get_font(font_id);
		if (font != NULL)
		{ int j;
		
		   ?? this is not complete correct ?? 
		  s[0] = font->names[0];
		  for (j = 0; j < 4; j++)
		    if (nr[j] >= 1 && nr[j] <= 4)
		      font->names[j] = s[nr[j]-1];
		    else
		      font->names[j] = "";
		  break;
		}
		*/
		
	_DONE(buffer)
	_END_RULE
	
}

bool scan_colours(CReadBuf &buffer)
{
	_RULE("colors", buffer)
	
	_SKIPBYTES(buffer, 34)

	while (!buffer.EOB())
	{
		_SKIPBYTES(buffer, 50)
		_VARPASCAL2STRING(w1, buffer, "w1")
		_ASSERT(printable_string(w1), buffer, ("-- not printable"))
		FREE(w1);
	}		

	_DONE(buffer)
	_END_RULE
}

bool scan_par_style_defs(CReadBuf &buffer, CQXDoc *doc)
{
  	_RULE("Paragraph style definitions", buffer);

  	word i;
  	i = 0;

	while (!buffer.EOB())
	{
		_OPEN_LEVEL
		_SKIPBYTES(buffer, 2);
		_VARWORD(font_id, buffer, "font_id");
		_SKIPBYTES(buffer, 302);
		_VARPASCAL2STRING(name, buffer, "name");
		_ASSERT(printable_string(name), buffer, ("-- not printable"))
	    { char *s;
	
	      for (s = name; *s != '\0'; s++)
	        if (*s == '<' || *s == '>')
	          *s = '_';
	    }
	    
	    doc->AppendNewParStyleDef(name, doc->GetFontFamily(font_id));
	    
	    FREE(name);
	    _CLOSE_LEVEL
	}

	_DONE(buffer)
	_END_RULE
}

bool scan_par_style_defs4(CReadBuf &buffer, CQXDoc *doc)
{
  	_RULE("Paragraph style definitions", buffer);

  	word i;
  	i = 0;

	while (!buffer.EOB())
	{
		_OPEN_LEVEL
		_VARPASCALFIXSTRING(name, buffer, "name", 64);
		_SKIPBYTES_S(buffer, 124)
		_VARWORD(font_id, buffer, "font_id");
		_SKIPBYTES_S(buffer, 54);
		_ASSERT(printable_string(name), buffer, ("-- not printable"))
	    { char *s;
	
	      for (s = name; *s != '\0'; s++)
	        if (*s == '<' || *s == '>')
	          *s = '_';
	    }
	    
	    doc->AppendNewParStyleDef(name, doc->GetFontFamily(font_id));
	    
	    FREE(name);
	    _CLOSE_LEVEL
	}

	_DONE(buffer)
	_END_RULE
}

bool scan_char_styles(CReadBuf &buffer, CQXDoc *doc)
{
	_RULE("Character styles", buffer);

	while (!buffer.EOB())
	{
		_OPEN_LEVEL
		_SKIPBYTES_S(buffer, 2)
		_VARWORD(font_id, buffer, "font_id")
		_VARWORD(mode, buffer, "mode")
		_VARWORD(size, buffer, "size")
		_SKIPBYTES_S(buffer, 38)
		
		doc->AppendNewCharStyle(doc->GetFontFamily(font_id), mode, size);

      	_CLOSE_LEVEL
    }
    
    _DONE(buffer)
    _END_RULE
}

bool scan_char_styles4(CReadBuf &buffer, CQXDoc *doc)
{
	_RULE("Character styles", buffer);

	while (!buffer.EOB())
	{
		_OPEN_LEVEL
		_SKIPBYTES_S(buffer, 8)
		_VARWORD(font_id, buffer, "font_id")
		_VARWORD(mode, buffer, "mode")
		_VARWORD(size, buffer, "size")
		_SKIPBYTES_S(buffer, 50)
		
		doc->AppendNewCharStyle(doc->GetFontFamily(font_id), mode, size);

      	_CLOSE_LEVEL
    }
    
    _DONE(buffer)
    _END_RULE
}

bool scan_par_styles(CReadBuf &buffer, CQXDoc *doc)
{
	_RULE("Paragraph styles", buffer);

	while (!buffer.EOB())
	{
		_OPEN_LEVEL
		_SKIPBYTES_S(buffer, 250)
		_VARBYTE(par_style_def_id, buffer, "par_style_def_id")
		_SKIPBYTES_S(buffer, 5)
		doc->AppendNewParStyle(doc->GetParStyleDef(par_style_def_id));
      	_CLOSE_LEVEL
    }
    
    _DONE(buffer)
    _END_RULE
}

bool scan_par_styles4(CReadBuf &buffer, CQXDoc *doc)
{
	_RULE("Paragraph styles", buffer);

	while (!buffer.EOB())
	{
		_OPEN_LEVEL
		_SKIPBYTES_S(buffer, 4)
		_VARWORD(par_style_def_id, buffer, "par_style_def_id")
		_SKIPBYTES_S(buffer, 94)
		doc->AppendNewParStyle(doc->GetParStyleDef(par_style_def_id));
      	_CLOSE_LEVEL
    }
    
    _DONE(buffer)
    _END_RULE
}


bool scan_text(CReadBuf &buffer, CReadBufWithBlocks &read_buf, CText *text, CQXDoc *doc)
{
	_NEWLINE
	_RULE("text", buffer)
	
	lword style_size = version < VER_4 ? 6 : 8;

	_VARLWORD(text_len, buffer, "text_len")
	if (text) text->AllocText(text_len);
			
	{	_VARLWORD(len, buffer, "len_text_seg")
		_SAFE_SUB_BUFFER(buffer, seg_buffer, len, "seg_buffer")

		lword prev_block_nr = 0L;
		
		_OPEN_LEVEL
		while (!seg_buffer.EOB())
		{
			_NEWLINE
			_VARLWORD(block_nr, seg_buffer, "block_nr")
			if (text == NULL)
			{	if (!read_buf.IsFree(block_nr))
					return FALSE;
				/*if (block_nr <= prev_block_nr)
					return FALSE;*/
				prev_block_nr = block_nr;
			}
			else
			{	read_buf.ClaimBlocks(block_nr, 1);
			}
			
			lword seg_len;
			if (version < VER_4)
			{	_WORD(seg_len, seg_buffer, "seg_len")
			}
			else
			{
				_LWORD(seg_len, seg_buffer, "seg_len")
			}
			_ASSERT(seg_len <= 256, seg_buffer, ("seg_len > 256"))
			
			if (text) text->AddText(seg_len, read_buf.GetBytes((block_nr-1)*256, seg_len));
			
			if (echo)
			{	printf("|");
				for (word i = 0; i < seg_len; i++)
				{
					byte ch = read_buf[(block_nr-1)*256 + i];
					if (ch < ' ' || ch > 127)
						printf("#%02X", ch);
					else
						printf("%c", ch);
				}
				printf("|");
			}
		}
		if (text) 
		{	_ASSERT(text->CloseText(), seg_buffer, ("Text length %ld differs from allocated length", text->text_len));
		}
		
		_CLOSE_LEVEL
	}
	
	_NEWLINE
	{	_VARLWORD(len, buffer, "len_char_styles")
	
		if (text)
			text->AllocCharStyles(len / style_size);
			
		_SAFE_SUB_BUFFER(buffer, styles_buffer, len, "char_styles_buffer")
		
		_OPEN_LEVEL
		while (!styles_buffer.EOB())
		{
			_NEWLINE
			lword code;
			if (version < VER_4)
			{	_WORD(code, styles_buffer, "code")
			}
			else
			{
				_LWORD(code, styles_buffer, "code")
			}
			_VARLWORD(len, styles_buffer, "len")

			if (doc)
			{	
				CCharStyle *char_style = doc->GetCharStyle(code);
				
				if (echo)
					if (char_style != NULL)
						printf(" %s:%d:%d", 
						       char_style->FontFamilyName(""),
						       char_style->mode,
						       char_style->size);
					else
						printf(" <unknown>");
					
				if (text)
					text->AddCharStyle(char_style, len);
			}
		}
		_CLOSE_LEVEL
	}

	_NEWLINE
	{	_VARLWORD(len, buffer, "len_par_styles")

		if (text)
			text->AllocParStyles(len / style_size);

		if (buffer.pos + len > buffer.Length())
		{	lword new_len = ((buffer.Length() - buffer.pos)/style_size)*style_size;
			if (echo)
				printf("\nError: len truncated from %ld to %ld (%ld+%ld=%ld > %ld)",
				       len, new_len, buffer.pos, len, buffer.pos+len, buffer.Length());
			len = new_len;
		};
		
		_SAFE_SUB_BUFFER(buffer, styles_buffer, len, "par_styles_buffer")
		
		_OPEN_LEVEL
		while (!styles_buffer.EOB())
		{
			_NEWLINE
			lword code;
			if (version < VER_4)
			{	_WORD(code, styles_buffer, "code")
			}
			else
			{
				_LWORD(code, styles_buffer, "code")
			}
			_VARLWORD(len, styles_buffer, "len")
			if (doc)
			{	
				CParStyle *par_style = doc->GetParStyle(code);
				CParStyleDef *par_style_def = par_style ? par_style->par_style_def.A() : NULL;
				
				if (echo)
					printf(" %s", par_style_def != NULL ? par_style_def->name.A() : "<unknown>");

				if (text)
					text->AddParStyle(par_style_def, len);
			}
		}
		_CLOSE_LEVEL
	}

	_END_RULE	
}


bool scan_box(CReadBuf &buffer, CBox *box)
{
	_RULE_NO_LEVEL("box", buffer)
	_VARLWORD(y1, buffer, "y1")
	_VARLWORD(x1, buffer, "x1")
	_VARLWORD(y2, buffer, "y2")
	_VARLWORD(x2, buffer, "x2")
	
	if (box != NULL)
	{	box->x1 = x1;
		box->y1 = y1;
		box->x2 = x2;
		box->y2 = y2;
	}
	
	_END_RULE_NO_LEVEL
}

#define VER_IS_NFT (version == VER_1 || version == VER_2)

word ran;
word ran_increment;
word frame_nr;

short block_nrs[8] = { 0, 0, 0, 45, 0, 43, 0, 63 };

word decode_word_with_ran(word value)
{
   	for (word r = 0; r < 32000; r++)
       	if (value == (word)(ran + r - (r & ran) * 2))
       		return r;
    
    printf("\nError: Could not decode %04X for ran %04X!", value, ran);
    
    return 0;
}

byte decode_byte_with_ran(byte value)
{
   	for (byte r = 0; r < 256; r++)
       	if (value == (byte)(ran + r - (r & ran) * 2))
       		return r;
    
    printf("\nError: Could not decode %04X for ran %04X!", value, ran);
    
    return 0;
}

bool scan_frame(CReadBuf &buffer, CReadBufWithBlocks &read_buf, CQXDoc *doc, CPage *page)
{
	_NEWLINE
	_RULE("frame", buffer)
	_OPEN_LEVEL_NAME("frame")

	_VARBYTE(frame_type, buffer, "frame_type")
	frame_type = decode_byte_with_ran(frame_type);
	_PRINTF("= %02X ", frame_type);
	_ASSERT(   frame_type == 0 || frame_type == 1 || frame_type == 3
	        || frame_type == 11 || frame_type == 12, buffer, ("Unknown frame type %d", frame_type));
	
	_VARBYTE(b2, buffer, "b2")
	_VARWORD(flags1, buffer, "flags1")
	_VARWORD(flags2, buffer, "flags2")

	_VARLWORD(raw_index, buffer, "index")
	word index = decode_word_with_ran(raw_index);
	_PRINTF("= %04X ", index);

	_VARWORD(w2, buffer, "w2")
	_VARWORD(w3, buffer, "w3")
	/*_ASSERT(w3 == 0 || w3 == 0xFFA6, buffer, ("must be 0 or FFA6"));*/
	_VARWORD(w4, buffer, "w4")
	_EXPECTLWORD(0L, buffer, "w5")
	_VARLWORD(text_id, buffer, "text id")
	_EXPECTLWORD(0L, buffer, "lw6")
	_VARWORD(t0, buffer, "t0")
	_VARWORD(w7, buffer, "w7")
	_VARWORD(w8, buffer, "w8")

	byte frame_sub_type = 0;
	if (!VER_IS_NFT)
	{	_BYTE(frame_sub_type, buffer, "sub type")
		_ASSERT((frame_sub_type >= 1 && frame_sub_type <= 3) || frame_sub_type == 5, 
	    	    buffer, ("unknown frame sub type"))
		_VARBYTE(t2, buffer, "t2")
		_EXPECTLWORD(0L, buffer, "lw9")
	}

	CBox box;
	_CALL_IC(scan_box(buffer, &box), buffer)

	if (frame_type == 0 || frame_type == 1)
	{
		_VARLWORD(lw10, buffer, "lw10")
		_EXPECTWORD(0, buffer, "w11")
	}
	else
	{
		if (version == VER_2)
		{	_VARLWORD(s41, buffer, "s4.1")
			_VARLWORD(s42, buffer, "s4.2")
		}
		else
		{	_EXPECTLWORD(0L, buffer, "s4.1")
			_EXPECTLWORD(0x10000L, buffer, "s4.2")
		}
		_VARWORD(s43, buffer, "s4.3") // 0x0180 or 0x0

		if (frame_type == 3) 
		{
			_VARWORD(has_text2, buffer, "has_text")
			_VARWORD(w12, buffer, "w12")
			_EXPECTLWORD(0L, buffer, "lw13")
			_VARLWORD(offset, buffer, "offset")
			_VARWORD(t3, buffer, "t3")
			_VARWORD(t4, buffer, "t4")
			_VARLWORD(w13, buffer, "w13") /* 000BFFC2 */
			_VARLWORD(s1, buffer, "s1")
			_ASSERT(s1 == 0, buffer, ("Expect 0"))
			_EXPECTLWORD(0L, buffer, "s2")
			_VARLWORD(s3, buffer, "s3")
			_EXPECTLWORD(0L, buffer, "s4")
			_EXPECTLWORD(0L, buffer, "s5")
			_EXPECTLWORD(0L, buffer, "s6")
			_VARWORD(nrcol, buffer, "nrcol")
			_ASSERT(nrcol == 0x100 || nrcol == 0x200 || (VER_IS_NFT && nrcol == 4), buffer, ("should be 1 or 2 (or 4)"))
			_EXPECTLWORD(0L, buffer, "s7")
			_EXPECTLWORD(0L, buffer, "s8")
			_VARLWORD(frame_id, buffer, "frame_id")
			_VARLWORD(segoff, buffer, "segoff")
			_VARLWORD(s9, buffer, "s9")

			bool has_text = FALSE;
			bool read_text = FALSE;
			
			if (segoff != 0)
				has_text = TRUE;
			else
			{
				_VARLWORD(a1, buffer, "a1")
				_ASSERT(VER_IS_NFT || a1 == 0, buffer, ("Expect 0"))
				_VARLWORD(edata, buffer, "edata")
				_VARLWORD(a3, buffer, "a3")
				if (edata != 0)
				{
					_VARLWORD(edatalen, buffer, "len")
					_SKIPBYTES_S(buffer, (word)edatalen)
				}
				
				if (index != 0)
				{	has_text = TRUE;
					read_text = TRUE;
				}
				else
				{
					_VARWORD(e1, buffer, "e1")
					_VARLWORD(e2, buffer, "e2")
					_VARWORD(e3, buffer, "e3")
					_VARLWORD(e4, buffer, "e4")
				}				
			}

			if (has_text)
			{
				CText *text = doc->GetText(text_id);
				CFrame *frame = page->NewAddedFrame(frame_nr, box);
				frame->text_fragment = text->InsertTextFragmentAt(offset);

				if (read_text)
				{	CBuf text_chain_buf;
					if (scan_chain_from(index, 1, read_buf, text_chain_buf))
					{
						CReadBuf t_buffer(text_chain_buf, "t_buffer");
					
						if (!scan_text(t_buffer, read_buf, text, doc))
							printf("\nError: For frame %d no text at %d.",
							       frame_nr, index);
					}
					else
						printf("\nError: For frame %d no chain at %d.",
						       frame_nr, index);
				}
			}			
		}
		else if (frame_type == 11)
		{
			_VARWORD(nr_sub_frames, buffer, "nr sub frames")
			_SKIPWORD(buffer)
			_VARLWORD(len_extra, buffer, "len")
			_ASSERT((lword)nr_sub_frames * 4 == len_extra, buffer, ("nr sub frames and len don't match"))

			CFrame *frame = page->NewAddedFrame(frame_nr, box);
			frame->AllocSubFrames(nr_sub_frames);
			
			for (word i = 0; i < nr_sub_frames; i++)
			{	_VARLWORD(sub_frame_nr, buffer, "sub_fr_id")
				frame->AppendSubFrame(sub_frame_nr);
			}
		}
		else if (frame_type == 12)
		{
			if (VER_IS_NFT)
			{
				_SKIPBYTES_S(buffer, 78)
				_VARLWORD(edatalen, buffer, "len")
				_SKIPBYTES_S(buffer, (word)edatalen)
			}
			else 
			{
				lword f1 = buffer.GetLWord(buffer.pos+9);
				_SKIPBYTES_S(buffer, 82)
				if (f1 != 0)
				{
					_VARLWORD(edatalen, buffer, "len")
					_SKIPBYTES_S(buffer, (word)edatalen)
				}
			}

			// Get graphical data
			if (index > 0)
			{
				CBuf img_chain_buf;
				scan_chain_from(index, 1, read_buf, img_chain_buf);

				if (dump)
				{
					for (lword j = 0; j < buffer.Length();)
					{
						_NEWLINE
						printf("[");
						for (lword j2 = 0 ; j2 < 100 && j < buffer.Length(); j2++, j++)
							printf("%02X", buffer[j]);
						printf("]");
					}
					_NEWLINE
				}
			}
		}
		else
		{
			_ASSERT(0, buffer, ("Unknown frame type %d", frame_type))
		}
	}
	_CLOSE_LEVEL

	frame_nr++;
	ran += ran_increment;
	
	_END_RULE	
}

bool scan_frame4(CReadBuf &buffer, CReadBufWithBlocks &read_buf, CQXDoc *doc, CPage *page, bool no_text)
{
	_NEWLINE
	_RULE("frame4", buffer)
	
	_VARLWORD(flags, buffer, "flags")
	_VARWORD(w1, buffer, "w1")
	//_ASSERT((flags == 0x20000001L) == (w1 == 1), buffer, ("flags and w1 not matching"))
	_VARLWORD(lw2, buffer, "lw2")
	_VARWORD(w3, buffer, "w3")
	_VARLWORD(raw_index, buffer, "raw_index") // ??
	word index = (word)~raw_index;
	_PRINTF("= %04X ", index);

	if (raw_index != 0L)
	{	_VARLWORD(lw4, buffer, "z4")
		_EXPECTLWORD(0L, buffer, "z5")
	}
	_VARLWORD(text_id, buffer, "text_id")
	_VARLWORD(lw7, buffer, "lw7")
	_EXPECTWORD(0, buffer, "z8")
	_VARLWORD(lw9, buffer, "lw9") // text id??
	_VARWORD(w10, buffer, "w10")
	_VARWORD(w11, buffer, "w11")
	_VARBYTE(b12, buffer, "b12")
	_VARBYTE(b13, buffer, "b13")
	_VARLWORD(lw14, buffer, "lw14")
	_VARWORD(w15, buffer, "w15")
	_EXPECTLWORD(1L, buffer, "c16")
	_VARLWORD(lw17, buffer, "lw17")
	_VARLWORD(lw18, buffer, "lw18")
	_EXPECTLWORD(0L, buffer, "z19")
	_VARLWORD(lw20, buffer, "lw20")
	_VARLWORD(lw21, buffer, "lw21")
	_VARLWORD(lw22, buffer, "lw22")
	_VARLWORD(lw23, buffer, "lw23")
	_VARLWORD(lw24, buffer, "lw24")
	_VARLWORD(lw25, buffer, "lw25")
	_VARLWORD(lw26, buffer, "lw26")
	_EXPECTLWORD(0L, buffer, "z27")
	_EXPECTLWORD(0L, buffer, "z28")
	_EXPECTLWORD(0L, buffer, "z29")
	_VARWORD(w30, buffer, "w30")   // 1999 or 0000
	_EXPECTLWORD(0L, buffer, "z31")
	CBox box;
	_CALL_IC(scan_box(buffer, &box), buffer)
	_EXPECTLWORD(0L, buffer, "z32")
	_VARLWORD(lw33, buffer, "lw33")
	_EXPECTLWORD(0L, buffer, "z34")
	_EXPECTLWORD(0L, buffer, "z35")
	_EXPECTLWORD(0L, buffer, "z36")
	_EXPECTLWORD(0L, buffer, "z37")
	if (flags == 0)
	{
		_VARWORD(nr_sub_frames, buffer, "nr sub frames")
		_VARLWORD(lws1, buffer, "lws1")
		_EXPECTWORD(0, buffer, "zs2")
		_VARLWORD(lensub, buffer, "lensub")
		_ASSERT((lword)nr_sub_frames * 4 == lensub, buffer, ("nr*4 != len"))
		if (nr_sub_frames > 0)
		{
			CFrame *frame = page->NewAddedFrame(frame_nr, box);
			frame->AllocSubFrames(nr_sub_frames);
			
			for (word i = 0; i < nr_sub_frames; i++)
			{	_VARLWORD(sub_frame_nr, buffer, "sub_fr_id")
				frame->AppendSubFrame(sub_frame_nr);
			}
		}
		else
		{
			_EXPECTLWORD(0L, buffer, "zs3")
			_EXPECTLWORD(0L, buffer, "zs4")
		}
	}
	else if (w1 == 0 && flags == 0x20000000)
	{
		_SKIPBYTES_S(buffer, 20)
	}
	else if (w1 == 0) //&& flags != 0x20000001)
	{
		if (buffer.GetByte() == 0xFF)
		{
			_PRINTF("\n(special)");
			_VARBYTE(b38, buffer, "b38")
			_VARLWORD(s1, buffer, "s1")
			_SKIPBYTES_S(buffer, 111)
			_VARLWORD(s2, buffer, "s2")
			_EXPECTLWORD(0L, buffer, "zs2")
			if (s1 != 0L)
			{
				_VARLWORD(slen, buffer, "slen")
				_SKIPBYTES_S(buffer, slen)
				if (s2 != 0L) // was: buffer.GetLWord() != 0 && buffer.GetLWord() % 2 == 0)
				{
					_LWORD(slen, buffer, "slen2")
					_SKIPBYTES_S(buffer, slen)
				}
			}

			// Get graphical data
			/* the following code is incorrect. Index is not the starting block nr
			if (index > 0)
			{
				CBuf img_chain_buf;
				scan_chain_from(index, 1, read_buf, img_chain_buf);

				if (dump)
				{
					for (lword j = 0; j < buffer.Length();)
					{
						_NEWLINE
						printf("[");
						for (lword j2 = 0 ; j2 < 100 && j < buffer.Length(); j2++, j++)
							printf("%02X", buffer[j]);
						printf("]");
					}
					_NEWLINE
				}
			}*/
		}
		else
		{	_VARLWORD(offset, buffer, "offset")
			_VARBYTE(b38, buffer, "b38")
			_VARWORD(w40, buffer, "w40")
			_EXPECTBYTE(0, buffer, "z41")
			_VARLWORD(lw42, buffer, "lw42")  // 000BFFC2
			_EXPECTLWORD(0L, buffer, "z43")
			_EXPECTLWORD(0L, buffer, "lw44")
			_EXPECTLWORD(0L, buffer, "z45")
			_EXPECTLWORD(0L, buffer, "z46")
			_EXPECTLWORD(0L, buffer, "z47")
			_EXPECTLWORD(0L, buffer, "z48")
			_VARBYTE(nrcol, buffer, "nrcol")
			_EXPECTLWORD(0L, buffer, "z49")
			_EXPECTLWORD(0L, buffer, "z50")
			_EXPECTWORD(0, buffer, "z51")
			_EXPECTBYTE(0, buffer, "z52")
			_VARLWORD(segnr, buffer, "segnr")   // from scan_frame
			_VARLWORD(segoff, buffer, "segoff")
			_VARLWORD(s9, buffer, "s9")
			_EXPECTWORD(0x0101, buffer, "c54")  // these three are added for scan_frame4
			_VARWORD(w54, buffer, "w54") // 0300 or 0200
			_EXPECTLWORD(0L, buffer, "z55")

			bool has_text = FALSE;
			bool read_text = FALSE;

			if (segoff != 0)
				has_text = TRUE;
			else
			{
				_VARLWORD(a1, buffer, "a1")
				_VARLWORD(edata, buffer, "edata")
				_VARLWORD(a3, buffer, "a3")
				if (edata != 0)
				{
					_VARLWORD(edatalen, buffer, "len")
					_SKIPBYTES_S(buffer, (word)edatalen)
				}

				if (   (   buffer.GetLWord(buffer.pos) < 256L
				        && buffer.GetLWord(buffer.pos+4) == 0L
				        && buffer.GetLWord(buffer.pos+8) < 256
				        && buffer.GetLWord(buffer.pos+12) == 0L))
				{
					_VARLWORD(e1, buffer, "e1")   // frame4: LWORD ipv WORD
					_VARLWORD(e2, buffer, "e2")
					_VARLWORD(e3, buffer, "e3")   // frame4: LWORD ipv WORD
					_VARLWORD(e4, buffer, "e4")
					//printf("\nEXTRA %04x %08lx %04x %08lx", e1, e2, e3, e4);
					//if (index != 0)
					//	printf("INDEX!=0");
				}
				else
				{	has_text = TRUE;
					read_text = TRUE;
				}
			}

			if (has_text && !no_text)
			{
				CText *text = doc->GetText(text_id);
				CFrame *frame = page->NewAddedFrame(frame_nr, box);
				frame->text_fragment = text->InsertTextFragmentAt(offset);

				if (read_text)
				{	CBuf text_chain_buf;
					if (scan_chain_from(index, 1, read_buf, text_chain_buf))
					{
						CReadBuf t_buffer(text_chain_buf, "t_buffer");
				
						if (!scan_text(t_buffer, read_buf, text, doc))
							printf("\nError: For frame %d no text at %d.",
						    	   frame_nr, index);
					}
					else
						printf("\nError: For frame %d no chain at %d.",
					    	   frame_nr, index);
				}
			}
		}
	}
	
	frame_nr++;
		
	_END_RULE
}

bool scan_page(CReadBuf &buffer, CReadBufWithBlocks &read_buf, word page_nr, CQXDoc *doc)
{
	CPage *page = doc->NewAppendedPage();

	frame_nr = 0;
		
	/*echo = 1;*/
	_NEWLINE
	_RULE("page", buffer)
	//dump_bytes_from_cur_pos(buffer, 1000);

	_OPEN_LEVEL_NAME("page")	
	_OPEN_LEVEL_NAME("sides")
	
	//dump_bytes(buffer, 16);
	_VARWORD(w1, buffer, "w1")
	
	if (version == VER_4 && w1 != 0)
		doc->nr_template_pages = page_nr;
	
	_VARLWORD(esize, buffer, "esize")
	_SUB_BUFFER(buffer, p_buffer, esize, "p_buffer")
	_VARWORD(nr_sides, p_buffer, "nr_sides")
	word lenpage = version == VER_4 ? 64 : 60;
	_ASSERT((lword)nr_sides * lenpage + 4 == esize, buffer, ("nr_sides * %d + 4 = %ld != esize", lenpage, (lword)nr_sides * lenpage + 4))
	_VARWORD(w2, p_buffer, "w2")

	for (word j = 0; j < nr_sides; j++)
	{
		_OPEN_LEVEL_NAME("t1")
		
		_VARWORD(w3, p_buffer, "w3")
		_VARWORD(w4, p_buffer, "w4")
		_NEWLINE
		_CALL_IC(scan_box(p_buffer, NULL), p_buffer)
		_NEWLINE
		_VARLWORD(lw5, p_buffer, "lw5")
		_VARWORD(w6, p_buffer, "w6")
		_VARLWORD(lw7, p_buffer, "lw7")
		_VARWORD(w8, p_buffer, "w8")
		_VARWORD(w9, p_buffer, "w9")
		_NEWLINE
		_CALL_IC(scan_box(p_buffer, NULL), p_buffer)
		_NEWLINE
		_VARWORD(w10, p_buffer, "w10")
		_VARWORD(w11, p_buffer, "w11")
		_VARWORD(w12, p_buffer, "w12")
		_VARBYTE(b13, p_buffer, "b13")
		_VARBYTE(b14, p_buffer, "b14")
		_VARWORD(w15, p_buffer, "w15")
		if (version == VER_4)
			_EXPECTLWORD(0L, p_buffer, "lw16")
				
		_CLOSE_LEVEL
	}
	_DONE(p_buffer)
	_CLOSE_LEVEL

	_OPEN_LEVEL_NAME("xinfo")
	for (word j = 0; j < nr_sides; j++)
	{
		_VARLWORD(esize, buffer, "esize")
		_SUB_BUFFER(buffer, x_buffer, esize, "x_buffer")
		_VARWORD(nr_x, x_buffer, "nr_x")
		_VARWORD(nr_y, x_buffer, "nr_y")
		_ASSERT((lword)(nr_x+nr_y) * 14 + 4 == esize, buffer, ("(nr_x+nr_y) * 14 + 4 = %ld != esize", (lword)(nr_x+nr_y) * 14 + 4))
		
		for (word j2 = 0; j2 < nr_x+nr_y; j2++)
		{
			_OPEN_LEVEL_NAME("t2")

			_VARLWORD(l1, x_buffer, "l1")
			_VARLWORD(l2, x_buffer, "l2")
			_VARLWORD(l3, x_buffer, "l3")
			_VARWORD(w4, x_buffer, "w4")
			
			_CLOSE_LEVEL
		}
		_DONE(x_buffer)
		_EXPECTLWORD(0L, buffer, "t5.1");
	}
	_CLOSE_LEVEL
	_NEWLINE

	_VARLWORD(ysize, buffer, "ysize")
	_SUB_BUFFER(buffer, y_buffer, ysize, "y_buffer")
	_VARLWORD(ynr, y_buffer, "ynr")
	_ASSERT((lword)ynr * 14 + 4 == ysize, buffer, ("ynr * 14 + 4 = %ld != ysize", (lword)ynr * 14 + 4))
	
	for (word j3 = 0; j3 < ynr; j3++)
	{
		_OPEN_LEVEL_NAME("y")

		_VARLWORD(yl1, y_buffer, "yl1")
		_VARLWORD(yl2, y_buffer, "yl2")
		_VARLWORD(yl3, y_buffer, "yl3")
		_VARWORD(yw4, y_buffer, "yw4")
		
		_CLOSE_LEVEL
	}
	_DONE(y_buffer)
	_EXPECTLWORD(0L, buffer, "t5.4");

	_EXPECTLWORD(4L, buffer, "t5.5");
	_EXPECTLWORD(0L, buffer, "t5.6");
	_VARLWORD(nr_frames, buffer, "nr_frames");

	if (version != VER_4)
	{	
		for (word j = 0; j < nr_frames; j++)
		{	_CALL_IC(scan_frame(buffer, read_buf, doc, page), buffer)
		}
	}
	else
	{
		//printf("\nFrame? "); dump_bytes_from_cur_pos(buffer, 32);
        while (   !buffer.EOB() 
	           && buffer.GetLWord(buffer.pos) != 0xE5
	           && buffer.GetLWord(buffer.pos+4) != 0x9E
	           && buffer.GetLWord(buffer.pos+2) != (lword)buffer.GetWord(buffer.pos+6) * 64 + 4)
		{
			_CALL_IC(scan_frame4(buffer, read_buf, doc, page, w1 != 0), buffer)
			//printf("\nFrame? "); dump_bytes_from_cur_pos(buffer, 32);
		}
	}
		
	
	_CLOSE_LEVEL
	_END_RULE
}


bool scan_file(CReadBufWithBlocks &read_buf, CQXDoc *doc)
{
	open_level = 0;

	lword nr_pages = 0;

	read_buf.ClaimBlocks(1,2);
	{	CReadBuf header_buffer(read_buf);
	
		lword first = header_buffer.GetLWord(0);
		if (first == 0x00360036)
			version = VER_1;
		else
		{
			//dump_bytes_from_cur_pos(header_buffer, 12);
			if (first != 0x00004D4D)
				return FALSE;

			if (header_buffer[9] == 0x3F)
				version = VER_3;
			else if (header_buffer[9] == 0x41)
				version = VER_4;
			else
				version = VER_2;
		}
		
		word start = version == VER_1 ? 0 : 16;
		
		if (version < VER_4)
		{
			doc->nr_template_pages = header_buffer.GetByte(start+101);
			doc->nr_pages = header_buffer.GetWord(start+48);
			nr_pages = doc->nr_template_pages + doc->nr_pages;
			//printf("nr_pages = %ld ", nr_pages);
		}

		frame_nr = 0;
		ran = 0;
		ran_increment = 0;
		if (version > VER_1)
		{	ran = header_buffer.GetWord(start + 256);
			ran_increment = header_buffer.GetWord(start + 258);
			//printf(" %04X %04X", ran, ran_increment); 
		}
		
		if (dump)		
		{	printf("[");
			for (word i = 0; i < 450; i++)
				printf("%02X", header_buffer[i]);
			printf("] ");
		}
	}
	
	//printf(" ver=%d ", version);
	
	//echo++;
	
	CBuf main_chain_buf;
	if (!scan_main_chain(read_buf, main_chain_buf))
		return FALSE;
	
	//echo--;
	printf("\n");
	
	CReadBuf buffer(main_chain_buf, "main_chain_buf");

	lword len;
	if (version != VER_1)
	{
		_LWORD(len, buffer, "len1")
		_SKIPBYTES_S(buffer, len);
	}
	
	_LWORD(len, buffer, "len2")
	_SKIPBYTES_S(buffer, len);

	// Printer information
	_LWORD(len, buffer, "len3")
	_SKIPBYTES_S(buffer, len);
	_EXPECTLWORD(0L, buffer, "z")
	if (version == VER_4)
	{
		_EXPECTLWORD(0L, buffer, "z")
	}

	// Font families
	{
		_LWORD(len, buffer, "len4");
		_SUB_BUFFER(buffer, ff_buffer, len, "ff_buffer");
		_CALL(scan_font_families(ff_buffer, doc), ff_buffer);
	}

	if (version > VER_2)
	{
		_LWORD(len, buffer, "len5");
		_SUB_BUFFER(buffer, f_buffer, len, "f_buffer");
		_CALL(scan_fonts(f_buffer), f_buffer);
		
	}


	// echo++;

	if (version != VER_4)
	{

		_NEWLINE
		{	_LWORD(len, buffer, "len6");
			_SUB_BUFFER(buffer, c_buffer, len, "c_buffer");
			_CALL(scan_colours(c_buffer), c_buffer);
		}
		
		_NEWLINE
		{	_LWORD(len, buffer, "len7");
			_SKIPBYTES_S(buffer, len)
		}

		_NEWLINE
		{	_LWORD(len, buffer, "len8");
			//_SKIPBYTES_S(buffer, len);
			_SUB_BUFFER(buffer, pd_buffer, len, "pd_buffer");
			_CALL(scan_par_style_defs(pd_buffer, doc), pd_buffer);
		}
		
		_NEWLINE
		{	_LWORD(len, buffer, "len9");
			_SKIPBYTES_S(buffer, len);
		}
		
		_NEWLINE
		{	_LWORD(len, buffer, "len10");
			_SKIPBYTES_S(buffer, len);
		}

		_NEWLINE
		{	_LWORD(len, buffer, "len11");
			_SUB_BUFFER(buffer, cs_buffer, len, "cs_buffer");
			_CALL(scan_char_styles(cs_buffer, doc), cs_buffer);
		}
		
		_NEWLINE
		{	_LWORD(len, buffer, "len12");
			_SUB_BUFFER(buffer, ps_buffer, len, "ps_buffer");
			_CALL(scan_par_styles(ps_buffer, doc), ps_buffer);
		}

		if (version != VER_1)
 		{	_LWORD(len, buffer, "len13");
 			_SKIPBYTES_S(buffer, len);
		}

		// scan pages 
		for (lword page_nr = 1; page_nr <= nr_pages; page_nr++)
		{	_CALL(scan_page(buffer, read_buf, page_nr, doc), buffer)
		}

	}
	else if (version == VER_4)
	{
		//echo++;
		// colors ???
		_PRINTF("\ncolors");
		_LWORD(len, buffer, "len6");
		_SKIPBYTES_S(buffer, len)

#define _SKIPBYTES_M(M) for(;len>0;len-=M){if(echo||dump)printf("\n   ");_SKIPBYTES_S(buffer,M)}

		// par styles (fixed length, multiple of 244)		
		_PRINTF("\npar styles ");
		{	_LWORD(len, buffer, "len7");
			_SUB_BUFFER(buffer, sd_buffer, len, "sd_buffer");
			_CALL(scan_par_style_defs4(sd_buffer, doc), sd_buffer);
		}
		
		// ?? any number of things multiple of 8 but not of 140
		_LWORD(len, buffer, "len7b")
		while (len % 140 != 0)
		{
			_ASSERT_MULTIPLE(len, 8, buffer)
			_SKIPBYTES_M(8)
			_LWORD(len, buffer, "len7b")
		}
		
		//  (fixed length, multiple of 140??)
		_PRINTF("\n140: ");
		_SKIPBYTES_M(140)

		
		// ?? x style (fixed length, multiple of 112)
		_PRINTF("\n");
		_LWORD(len, buffer, "len8")
		_ASSERT_MULTIPLE(len, 112, buffer)
		_SKIPBYTES_M(112)
		
		// line styles (fixed length, multiple of 252)
		_PRINTF("\n");
		_LWORD(len, buffer, "len10")
		_ASSERT_MULTIPLE(len, 252, buffer)
		_SKIPBYTES_M(252)

		_PRINTF("\n");
		_EXPECTLWORD(0L, buffer, "len11")

		_PRINTF("\n");
		// ??? n , n * 8 bytes: word, word, byte, byte, byte, byte
		{	_LWORD(len, buffer, "len12")
			_SKIPBYTES_S(buffer, len)
		}
		
		// seven multiple of 216 
		_PRINTF("\n");
		_LWORD(len, buffer, "len13")
		while(len % 216 == 0)
		{
			_SKIPBYTES_M(216);
			_LWORD(len, buffer, "len13")
		}
		
		// seven multiple of 272
		_PRINTF("\n");
		while(len % 272 == 0)
		{
			_SKIPBYTES_M(272);
			_LWORD(len, buffer, "len13")
		}
		
		// four multiples of 148
		_PRINTF("\n");
		while(len % 148 == 0)
		{
			_SKIPBYTES_M(148);
			_LWORD(len, buffer, "len13")
		}
		
		// four multiples of 216
		_PRINTF("\n");
		while(len % 216 == 0)
		{
			_SKIPBYTES_M(216);
			_LWORD(len, buffer, "len13")
		}
		
		// 12
		_ASSERT(len == 12, buffer, ("expect value 12"))
		_SKIPBYTES_S(buffer, 12)
		
		// multiple of 64
		_NEWLINE
		{	_LWORD(len, buffer, "len15");
			_SUB_BUFFER(buffer, cs_buffer, len, "cs_buffer");
			_CALL(scan_char_styles4(cs_buffer, doc), cs_buffer);
		}
		
		// multiples of 8 (but not of 100) or at least on zero
		_PRINTF("\n");
		_LWORD(len, buffer, "len16")
		if (len != 0)
		{
			while (len % 100 != 0)
			{
				_ASSERT_MULTIPLE(len, 8, buffer)
				_SKIPBYTES_M(8)
				_PRINTF("\n");
				_LWORD(len, buffer, "len16")
			}
		}
		if (len == 0)
		{	_PRINTF("\n");
			_LWORD(len, buffer, "len16")
		}
		
		// multiple of 100
		{	_SUB_BUFFER(buffer, ps_buffer, len, "ps_buffer");
			_CALL(scan_par_styles4(ps_buffer, doc), ps_buffer);
		}
		
		// 960
		_PRINTF("\n");
		_EXPECTLWORD(960L, buffer, "len18")
		_SKIPBYTES_S(buffer, 960)

		word page_nr = 0;
		while(   !buffer.EOB()
		      && buffer.GetLWord(buffer.pos+2) 
		         == (lword)buffer.GetWord(buffer.pos+6) * 64 + 4)
		{	page_nr++;
			_CALL(scan_page(buffer, read_buf, page_nr, doc), buffer)
		}
		doc->nr_pages = page_nr;
		printf(" nr_pages=%ld ", nr_pages);
	}

	if(echo)
	{	bool blocks_left = FALSE;
	
		for(;;)
		{	lword block_nr = read_buf.NextFreeBlock();
			if (block_nr == 0)
				break;
		    if (!blocks_left)
		    	printf("\nBlocks left:\n");
			blocks_left = TRUE;
			printf("%4ld ", block_nr);
			for (word i = 0; i < 256; i++)
			{	byte ch = read_buf[(block_nr-1)*256 + i];
				printf(ch <= ' ' || ch > 127?"%02X":" %c", ch);
			}
			printf("\n");  
			read_buf.ClaimBlocks(block_nr, 1);
		}
	}
		
	return TRUE;
}

void scan_QX_doc(CQXDoc *doc)
{
	CBuf buf;
	if (buf.ReadFile(doc->name.A()))
	{
		CReadBufWithBlocks read_buf(buf, "read_buf");

		scan_file(read_buf, doc);
	}
	else
	{
		printf("Error: Could not open %s\n", doc->name.A());
	}
}

