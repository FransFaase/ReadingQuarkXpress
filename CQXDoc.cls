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

class CBox
{
  public:
  	lword x1, y1, x2, y2;
  	
  	inline lword Dx()
  	{	return x2 - x1;
  	}
  	inline lword Dy()
  	{	return y2 - y1;
  	}
  	inline double Area()
  	{	return (double)(x2 - x1) * (double)(y2 - y1);
  	}

  	static CBox Intersection(CBox &box1, CBox &box2)
  	{
  		CBox box;
  		box.x1 = MAX(box1.x1, box2.x1);
  		box.x2 = MIN(box1.x2, box2.x2);
  		box.y1 = MAX(box1.y1, box2.y1);
  		box.y2 = MIN(box1.y2, box2.y2);
  		if (box.x2 < box.x1 || box.y2 < box.y1)
  		{	box.x1 = 0;
  			box.x2 = 0;
  			box.y1 = 0;
  			box.y2 = 0;
  		}
		return box;
  	}
  	static bool Equal(CBox &box1, CBox &box2)
  	{
  		return    box1.x1 == box2.x1 && box1.x2 == box2.x2 
  		       && box1.y1 == box2.y1 && box1.y2 == box2.y2;
  	}
  	static bool InsideOrEqual(CBox &box1, CBox &box2)
  	{
  		return    box1.x1 >= box2.x1 && box1.x2 <= box2.x2
  		       && box1.y1 >= box2.y1 && box1.y2 <= box2.y2;
  	}
  	static int ReadingOrder(CBox &box1, CBox &box2)
  	{
  		int inside = Inside(box1, box2);
  		if (inside != 0)
  			return inside;
  			
		if (box2.y2 < box1.y1)
			return 3;  // -- box2 is above box1
		if (box1.y2 < box2.y1)
			return -3; // -- box1 is above box2

		lword dy_tol = min(box1.Dy(), box2.Dy()) / 10;
		if (box2.y2 < box1.y1 + dy_tol)
			return 2;  // -- box2 is visually above box1
		if (box1.y2 < box2.y1 + dy_tol)
			return -2; // -- box1 is visually above box2
		// -- they are overlapping in y-direction

		if (box2.x2 < box1.x1)
			return 3;  // -- box2 is left of box1
		if (box1.x2 < box2.x1)
			return -3; // -- box1 is left of box2
		
		lword dy_inter = MIN(box1.y2, box2.y2) - MAX(box1.y1, box2.y1);
		if (dy_inter + dy_tol > max(box1.Dy(), box2.Dy()))
		{	// -- boxes are visually of the same height and on the same height
		
			lword dx_tol = min(box1.Dx(), box2.Dx()) / 2;
			lword dx_inter = MIN(box1.x2, box2.x2) - MAX(box1.x1, box2.x1);
			bool half_overlap = dx_inter < dx_tol; 
		
			if (box2.x1 < box1.x1 && box2.x2 <= box1.x2)
				return half_overlap ? 3 : 2;    // -- box2 is left of box1
			if (box1.x1 < box2.x1 && box1.x2 <= box2.x2)
				return half_overlap ? -3 : -2;  // -- box1 is left of box2
		}
		else
		{
			lword dx_tol = min(box1.Dx(), box2.Dx()) / 10;
			if (box2.x2 < box1.x1 + dx_tol)
				return 2;  // -- box2 is visually left of box1
			if (box1.x2 < box2.x1 + dx_tol)
				return -2; // -- box1 is visually left of box2
		}	
		// -- no clear relationship

		if (box2.y1 < box1.y1)
			return 1;   // -- top-left corner of box2 is above of that of box1
		if (box1.y1 < box2.y1)
			return -1;  // -- top-left corner of box1 is above of that of box2
		if (box2.x1 < box1.x1)
			return 1;   // -- top-left corner of box2 is left of that of box1
		if (box1.x1 < box2.x1)
			return -1;  // -- top-left corner of box1 is left of that of box2

		return 0; // -- not very likely to occur
	}

  	static const char *ReadingOrderDescription(CBox &box1, CBox &box2)
  	{
  		if (Equal(box1, box2))
  			return "equal";
  		if (InsideOrEqual(box1, box2))
  			return "box1 inside box2";
  		if (InsideOrEqual(box2, box1))
  			return "box2 inside box1";
  			
		if (box2.y2 < box1.y1)
			return "box2 is above box1";
		if (box1.y2 < box2.y1)
			return "box1 is above box2";

		lword dy_tol = min(box1.Dy(), box2.Dy()) / 10;
		if (box2.y2 < box1.y1 + dy_tol)
			return "box2 is visually above box1";
		if (box1.y2 < box2.y1 + dy_tol)
			return "box1 is visually above box2";
		// -- they are overlapping in y-direction

		if (box2.x2 < box1.x1)
			return "box2 is left of box1";
		if (box1.x2 < box2.x1)
			return "box1 is left of box2";
		
		lword dy_inter = MIN(box1.y2, box2.y2) - MAX(box1.y1, box2.y1);
		if (dy_inter + dy_tol > max(box1.Dy(), box2.Dy()))
		{	// -- boxes are visually of the same height and on the same height
		
			lword dx_tol = min(box1.Dx(), box2.Dx()) / 2;
			lword dx_inter = MIN(box1.x2, box2.x2) - MAX(box1.x1, box2.x1);
			bool half_overlap = dx_inter < dx_tol; 
		
			if (box2.x1 < box1.x1 && box2.x2 <= box1.x2)
				return half_overlap 
				       ? "box2 is visually left of box1" 
				       : "box2 is vaguelly left of box1";
			if (box1.x1 < box2.x1 && box1.x2 <= box2.x2)
				return half_overlap
				       ? "box1 is visually left of box2" 
				       : "box1 is vaguelly left of box2";
		}
		else
		{
			lword dx_tol = min(box1.Dx(), box2.Dx()) / 10;
			if (box2.x2 < box1.x1 + dx_tol)
				return "box2 is visually left of box1";
			if (box1.x2 < box2.x1 + dx_tol)
				return "box1 is visually left of box2";
		}	
		// -- no clear relationship

		if (box2.y1 < box1.y1)
			return "top-left corner of box2 is above of that of box1";
		if (box1.y1 < box2.y1)
			return "top-left corner of box1 is above of that of box2";
		if (box2.x1 < box1.x1)
			return "top-left corner of box2 is left of that of box1";
		if (box1.x1 < box2.x1)
			return "top-left corner of box1 is left of that of box2";

		return "no clear relationship";
	}

  	static int Inside(CBox &box1, CBox &box2)
  	{
  		if (Equal(box1, box2))
  			return 0;
  		if (InsideOrEqual(box1, box2))
  			return 4;   // box1 inside box2
  		if (InsideOrEqual(box2, box1))
  			return -4;  // box2 inside box1
  		
  		return 0;
  	}
};




class CQXDoc : public CMMFObject
{
  public:
	CQXDoc(char *new_name)
	{	nr_template_pages = 0;
		nr_pages = 0;
		name.StrCopy(new_name);
	}
	~CQXDoc()
	{	DELETE_LIST(CFontFamily, all_font_families)
	    DELETE_LIST(CParStyleDef, all_par_style_defs)
		DELETE_LIST(CParStyle, all_par_styles)
		DELETE_LIST(CCharStyle, all_char_styles)
		DELETE_LIST(CPage, all_pages)
		DELETE_LIST(CText, all_texts)
	}

	void AppendNewFontFamily(word id, char *family_name)
	{	FOREACH_IN_LIST_REF(CFontFamily, r_font_family, all_font_families);
		*r_font_family = new CFontFamily(id, family_name);
	}
	CFontFamily *GetFontFamily(word id)
	{	FOREACH_IN_LIST(CFontFamily, font_family, all_font_families)
			if (font_family->id == id)
				return font_family;
		return NULL;
	}
	
	void AppendNewParStyleDef(char *name, CFontFamily *font_family)
	{	FOREACH_IN_LIST_REF(CParStyleDef, r_par_style_def, all_par_style_defs);
		*r_par_style_def = new CParStyleDef(name, font_family);
	}
	CParStyleDef *GetParStyleDef(word nr)
	{	NTH1_IN_LIST(CParStyleDef, par_style_def, nr, all_par_style_defs);
		return par_style_def;
	}

	void AppendNewParStyle(CParStyleDef *par_style_def)
	{	FOREACH_IN_LIST_REF(CParStyle, r_par_style, all_par_styles);
		*r_par_style = new CParStyle(par_style_def);
	}
	CParStyle *GetParStyle(word nr)
	{	NTH0_IN_LIST(CParStyle, par_style, nr, all_par_styles);
		return par_style;
	}

	void AppendNewCharStyle(CFontFamily *font, word mode, word size)
	{	FOREACH_IN_LIST_REF(CCharStyle, r_char_style, all_char_styles);
		*r_char_style = new CCharStyle(font, mode, size);
	}
	CCharStyle *GetCharStyle(word nr)
	{	NTH0_IN_LIST(CCharStyle, char_style, nr, all_char_styles);
		return char_style;
	}

	CText *GetText(lword text_id)
	{	FOREACH_IN_LIST_REF(CText, r_texts, all_texts)
			if ((*r_texts)->text_id == text_id)
				return (*r_texts);
		
		*r_texts = new CText(text_id, (*r_texts));
		return (*r_texts);
	}
	
	CPage *NewAppendedPage()
	{	word page_nr = 1;
		FOREACH_IN_LIST_REF(CPage, r_page, all_pages)
			page_nr++;;
		return *r_page = new CPage(this, page_nr, (*r_page));
	}

	CMMFString name;
	CMMFPointer<CFontFamily> all_font_families;
	CMMFPointer<CParStyleDef> all_par_style_defs;
	CMMFPointer<CParStyle> all_par_styles;
	CMMFPointer<CCharStyle> all_char_styles;
	CMMFPointer<CPage> all_pages;
	word nr_template_pages;
	word nr_pages;	
	CMMFPointer<CText> all_texts;
	CMMFPointer<CQXDoc> next;
};

class CFontFamily : public CMMFObject
{
  public:
	CFontFamily(word new_id, char *new_family_name)
	{	id = new_id;
		family_name.StrCopy(new_family_name);
		next = NULL;
	}
	~CFontFamily()
	{	//FREE(family_name);
	}
	
	word id;
	CMMFString family_name;
	CMMFPointer<CFontFamily> next;
};

class CParStyleDef : public CMMFObject
{
  public:
	CParStyleDef(char *new_name, CFontFamily *new_font_family)
	{	name.StrCopy(new_name);
		font_family = new_font_family;
		next = NULL;
	}
	~CParStyleDef()
	{	//FREE(name);
	}
	
	CMMFString name;
	CMMFPointer<CFontFamily> font_family;
	CMMFPointer<CParStyleDef> next;
};

class CParStyle : public CMMFObject
{
  public:
	CParStyle(CParStyleDef *new_par_style_def)
	{	par_style_def = new_par_style_def;
		next = NULL;
	}
	
	CMMFPointer<CParStyleDef> par_style_def;
	CMMFPointer<CParStyle> next;
};

class CCharStyle : public CMMFObject
{
  public:
	CCharStyle(CFontFamily *new_font, word new_mode, word new_size)
	{	font = new_font;
		mode = new_mode;
		size = new_size;
	}
	inline char *FontFamilyName(char *default_name)
	{	return !font.IsNull() ? (*font).family_name : default_name;
	}
	
	CMMFPointer<CFontFamily> font;
	word mode;
	word size;
	CMMFPointer<CCharStyle> next;
};
	
	

class CPage : public CMMFObject
{
  public:
	CPage(CQXDoc *new_doc, word new_page_nr, CPage *new_next) 
	{	doc = new_doc;
		page_nr = new_page_nr;
		side_by_side = FALSE;
		next = new_next;
	}
	~CPage()
	{	DELETE_LIST(CFrame, all_frames);
	}
	
	CFrame *NewAddedFrame(lword frame_nr, CBox &box)
	{
		FOREACH_IN_LIST_REF(CFrame, r_frame, all_frames)
			if (CBox::ReadingOrder((*r_frame)->box, box) >= 0)
				break;
		
		*r_frame = new CFrame(this, frame_nr, box, (*r_frame));
		return (*r_frame);
	}
	void InsertFrame(CFrame *frame)
	{
		FOREACH_IN_LIST_REF(CFrame, r_frame, all_frames)
			if (CBox::ReadingOrder((*r_frame)->box, frame->box) >= 0)
				break;

		frame->next = *r_frame;
		*r_frame = frame;
	}		
	void InsertFrame(CFrame *frame,
	                 int (*compare)(CFrame *f1, CFrame *f2))
	{
		FOREACH_IN_LIST_REF(CFrame, r_frame, all_frames)
			if ((*compare)((*r_frame), frame) >= 0)
				break;

		frame->next = *r_frame;
		*r_frame = frame;
	}		
	CFrame *GetFrameWithNr(lword frame_nr)
	{
		FOREACH_IN_LIST(CFrame, frame, all_frames)
			if (frame->frame_nr == frame_nr)
				return frame;

		return NULL;
	}

	CMMFPointer<CQXDoc> doc;
	word page_nr;
	bool side_by_side;
	CMMFPointer<CFrame> all_frames;
	CMMFPointer<CPage> next;
};

class CFrame : public CMMFObject
{
  public:
	CFrame(CPage *new_page, lword new_frame_nr, CBox &new_box, CFrame *new_next)
	{	page = new_page;
		frame_nr = new_frame_nr;
		box = new_box;
		geom_type = 0;
		nr_sub_frames = 0;
		alloc_nr_sub_frames = 0;
		next = new_next;
	}
	
	void AllocSubFrames(lword nr)
	{	alloc_nr_sub_frames = nr;
		sub_frame_nrs.Allocate(nr);
	}
	void AppendSubFrame(lword frame_nr)
	{	ASSERT(nr_sub_frames < alloc_nr_sub_frames)
		sub_frame_nrs[nr_sub_frames++] = frame_nr;
	}
	
	void InsertChild(CFrame *child,
	                 int (*compare)(CFrame *f1, CFrame *f2))
	{
		ASSERT(child->parent.IsNull())
		FOREACH_IN_LIST_REF_N(CFrame, r_frame, children, next_child)
			if ((*compare)(child, (*r_frame)) <= 0)
				break;
		
		child->next_child = (*r_frame);
		*r_frame = child;
		child->parent = this;
	}

	bool HasText()
	{	return !text_fragment.IsNull() && !text_fragment->IsEmpty();
	}
	bool TextIsNumber()
	{	CTextAccessor text_acc(text_fragment);
		text_acc.Init();
		while (text_acc.More() && text_acc.ch == ' ') 
			text_acc.Next();
		if (!(text_acc.More() && '0' <= text_acc.ch && text_acc.ch <= '9'))
			return FALSE;
		while (text_acc.More() && '0' <= text_acc.ch && text_acc.ch <= '9')
			text_acc.Next();
		while (text_acc.More() && text_acc.ch == ' ')
			text_acc.Next();
		return !text_acc.More() || text_acc.ch == '\0';
	}
	lword NumberValueText()
	{	lword val = 0;
		CTextAccessor text_acc(text_fragment);
		for (text_acc.Init(); text_acc.More(); text_acc.Next())
			if ('0' <= text_acc.ch && text_acc.ch <= '9')
				val = 10 * val + (text_acc.ch - '0');
		return val;
	}
	char *FirstParStyle()
	{	if (text_fragment.IsNull())
			return NULL;
		CTextAccessor text_acc(text_fragment);
		for (text_acc.Init(); text_acc.More(); text_acc.Next())
		{	if (text_acc.par_style.ToOpen())
				return text_acc.par_style.open_value;
		}
		return NULL;
	}
	

	CMMFPointer<CPage> page;
	CBox box;
	CMMFPointer<CTextFragment> text_fragment;
	lword frame_nr;
	byte geom_type;                 // -- to be used freely to set some geommetrical type
	CMMFLWordArray sub_frame_nrs;   // -- only used during reading
	lword nr_sub_frames;            // -- only used during reading
	CMMFPointer<CFrame> parent;
	CMMFPointer<CFrame> children;
	CMMFPointer<CFrame> next_child;
	CMMFPointer<CFrame> next;
	CMMFPointer<CFrame> next_reading_order;
  private:
	lword alloc_nr_sub_frames;
};

struct TParStyle
{
  	lword len;
  	CMMFPointer<CParStyleDef> par_style;
};

struct TCharStyle
{
  	lword len;
  	CMMFPointer<CCharStyle> char_style;
};


class CText : public CMMFObject
{
  public:
	CText(lword new_text_id, CText *new_next)
	{
		text_len = 0L;
		text_id = new_text_id;
		nr_char_styles = 0L;
		nr_par_styles = 0L;
		alloc_text_len = 0L;
		all_text_fragments = NULL;
		next = new_next;
	}
	~CText()
	{
		//FREE(text);
		//FREE(char_styles);
		//FREE(par_styles);
		DELETE_LIST(CTextFragment, all_text_fragments)
	}

	void AllocText(lword new_alloc_text_len)
	{
		alloc_text_len = new_alloc_text_len;
		text.Allocate(alloc_text_len);
		text_len = 0;
	}
	
	bool AddText(lword len, byte *new_text)
	{
		if (text_len + len > alloc_text_len)
			return FALSE;
		memcpy(&text[text_len], new_text, len);
		text_len += len;
		
		return TRUE;
	}
	
	bool CloseText()
	{	return text_len == alloc_text_len;
	}
		

	void AllocCharStyles(lword nr_styles)
	{
		char_styles.Allocate(nr_styles);
		nr_char_styles = 0;
	}
	
	void AddCharStyle(CCharStyle *char_style, lword len)
	{
		char_styles[nr_char_styles].char_style = char_style;
		char_styles[nr_char_styles].len = len;
		nr_char_styles++;
	}

	
	void AllocParStyles(lword nr_styles)
	{
		par_styles.Allocate(nr_styles);
		nr_par_styles = 0;
	}
	
	void AddParStyle(CParStyleDef *par_style, lword len)
	{
		par_styles[nr_par_styles].par_style = par_style;
		par_styles[nr_par_styles].len = len;
		nr_par_styles++;
	}

	CTextFragment *InsertTextFragmentAt(lword start)
	{
		FOREACH_IN_LIST_REF(CTextFragment, r_text_fragment, all_text_fragments)
			if ((*r_text_fragment)->start == start)
				return (*r_text_fragment);
			else if ((*r_text_fragment)->start > start)
				break;
		return *r_text_fragment = new CTextFragment(this, start, (*r_text_fragment));
	}
	
	CMMFByteArray text;
	lword text_len;
	lword text_id;
	CMMFArray<TCharStyle> char_styles;
	lword nr_char_styles;
	CMMFArray<TParStyle> par_styles;
	lword nr_par_styles;
	CMMFPointer<CTextFragment> all_text_fragments;
	CMMFPointer<CText> next;
	
  private:
    lword alloc_text_len;
};



class CTextFragment : public CMMFObject
{
  public:
	CTextFragment(CText *new_text, lword new_start, CTextFragment *new_next)
	{	text = new_text;
		start = new_start;
		next = new_next;
	}

	lword End()
	{	return !next.IsNull() ? (*next).start : (*text).text_len;
	}

	bool IsEmpty()
	{	if (text.IsNull())
			return FALSE;
		
		byte *text_text = text->text;
		if (text_text == NULL)
			return FALSE;
			
		lword end = End();
		for (lword i = start; i < end; i++)
			if (text_text[i] >= ' ')
				return FALSE;
		return TRUE;
	}
	
	CMMFPointer<CText> text;
	lword start;
	CMMFPointer<CTextFragment> next;
};


class CChange
{
  public:
	CChange()
	{	outer = NULL;
		Init();
	}
	inline void InnerOf(CChange *new_outer)
	{	outer = new_outer;
	}
	
	inline void Init()
	{	was_open = FALSE;
		to_open = FALSE;
		to_close = FALSE;
	}

	inline void Next()
	{	if (to_open)
			was_open = TRUE;
		else if (to_close)
			was_open = FALSE;
		to_open = FALSE;
		to_close = FALSE;
	}
	
	inline bool WasOpen()
	{	return was_open;
	}
	inline bool ToOpen()
	{	return to_open || (outer && outer->Changed());
	}
	inline bool ToClose()
	{	return to_close || (outer && outer->Changed());
	}
	inline void Set()
	{	to_close = was_open;
		to_open = TRUE;
	}
	inline void Reset()
	{	to_close = was_open;
		to_open = FALSE;
	}
	inline void Unchanged()
	{	to_close = FALSE;
		to_open = FALSE;
	}
	
  private:
	bool Changed()
	{	return to_open || to_close || (outer && outer->Changed());
	}
	CChange *outer;
	bool to_open;
	bool to_close;
	bool was_open;
};

class CChangeWord : public CChange
{
  public:
	CChangeWord()
	{	Init();
	}
	
	inline void Init()
	{	CChange::Init();
		close_value = 0;
		open_value = 0;
	}
	inline void Next()
	{	CChange::Next();
		close_value = open_value;
	}
	
	inline void Set(word new_value)
	{	open_value = new_value;
		CChange::Set();
	}
	inline void ChangeTo(word new_value)
	{	open_value = new_value;
		if (WasOpen() && new_value == close_value)
	    	CChange::Unchanged();
	    else
	    	CChange::Set();
	}

	word open_value;
	word close_value;	
};
	
class CChangeString : public CChange
{
  public:
	CChangeString()
	{	Init();
	}
	
	inline void Init()
	{	CChange::Init();
		close_value = NULL;
		open_value = NULL;
	}
	inline void Next()
	{	CChange::Next();
		close_value = open_value;
	}
	
	inline void Set(char *new_value)
	{	open_value = new_value;
		if (new_value == NULL)
			CChange::Reset();
		else
			CChange::Set();
	}
	inline void ChangeTo(char *new_value)
	{	open_value = new_value;
		if (new_value == NULL)
			CChange::Reset();
	    else if (WasOpen() && strcmp(new_value, close_value) == 0)
	    	CChange::Unchanged();
	    else
	    	CChange::Set();
	}
	
	char *open_value;
	char *close_value;
};

class CTextAccessor
{
  public:
	CTextAccessor(CTextFragment *fragment)
	{
		if (   fragment == NULL 
		    || fragment->text.IsNull() 
		    || fragment->text->text.IsNull())
		{	text = NULL;
			start = 0;
			end = 0;
			return;
		}

		text = fragment->text->text;
		start = fragment->start;
		end = fragment->End();
		char_styles = fragment->text->char_styles;
		nr_char_styles = fragment->text->nr_char_styles;
		par_styles = fragment->text->par_styles;
		nr_par_styles = fragment->text->nr_par_styles;
	}	
	
	void Init()
	{
		pos = 0;
		if (end == 0)
			return;

		ch = text[pos];
		is_tab = ch == 9;
		is_newline = ch == 13;

		par_style.Init();
		font_family_name.Init();
		size.Init();
		mode.Init();

		cur_par_style = 0;
		NewParStyle();
		
		cur_char_style = 0;
		NewCharStyle();
		
		while (pos < start)
			MoveNext();
	}

	bool More()
	{	return pos <= end;
	}
	void Next()
	{	par_style.Next();
		font_family_name.Next();
		mode.Next();
		size.Next();
		
		MoveNext();
	}
	bool ParseNumber(word &nr)
	{
		bool found_digit = FALSE;
		nr = 0;
	
		for (; More(); Next())
		{	if (ch < '0' || ch > '9')
				break;
			found_digit = TRUE;
			nr = nr * 10 + ch - '0';
		}
	
		return found_digit;
	}


	byte ch;
	bool is_tab;
	bool is_newline;
	CChangeString par_style;
	CChangeString font_family_name;
	CChangeWord size;
	CChangeWord mode;

  private:
  	byte *text;
  	lword start;
  	lword end;
  	lword pos;
	TCharStyle *char_styles;
	lword nr_char_styles;
	lword cur_char_style;
	lword char_style_chars_left;
	TParStyle *par_styles;
	lword nr_par_styles;
	lword cur_par_style;
	lword par_style_chars_left;

  private:	
	void MoveNext()
	{	pos++;
		if (pos > end)
			return;
		
		if (pos == end)
		{	ch = 0;
			
			par_style.Reset();
			font_family_name.Reset();
			mode.Reset();
			size.Reset();
		}
		else
		{	ch = text[pos];

			if (--par_style_chars_left == 0)
			{	cur_par_style++;
				NewParStyle();
			}
		
			if (--char_style_chars_left == 0)
			{	cur_char_style++;
				NewCharStyle();
			}
		}

		is_tab = ch == 9;
		is_newline = ch == 13;
	}

	void NewParStyle()
	{
		CParStyleDef *par_style_def = NULL;
		while (   cur_par_style < nr_par_styles
			   && par_styles[cur_par_style].len == 0)
			cur_par_style++;
		if (cur_par_style < nr_par_styles)
		{	par_style_chars_left = par_styles[cur_par_style].len;
			par_style_def = par_styles[cur_par_style].par_style;
		}
		else
			par_style_chars_left = (lword)-1;

		par_style.ChangeTo(par_style_def ? par_style_def->name.A() : NULL);
	}
	
	void NewCharStyle()
	{
		CCharStyle *char_style = NULL;
		while (   cur_char_style < nr_char_styles
			   && char_styles[cur_char_style].len == 0)
			cur_char_style++;
		if (cur_char_style < nr_char_styles)
		{	char_style_chars_left = char_styles[cur_char_style].len;
			char_style = char_styles[cur_char_style].char_style;
		}
		else
			char_style_chars_left = (lword)-1;

		if (char_style)
		{	font_family_name.ChangeTo(char_style->FontFamilyName(0));
			size.ChangeTo(char_style->size);
			mode.ChangeTo(char_style->mode);
		}
		else
		{	font_family_name.Reset();
			size.Reset();
			mode.Reset();
		}
	}
};	
		
class CTextOnFramesAccessor
{
  public:
	CTextOnFramesAccessor(CFrame **new_r_cur_frame) 
	{	r_cur_frame = new_r_cur_frame;
		cur_frame = *r_cur_frame;
		Init();
	}	
	
	void Init()
	{
		CTextFragment *fragment = cur_frame->text_fragment;
		if (   fragment == NULL 
		    || fragment->text.IsNull() 
		    || fragment->text->text.IsNull())
		{	text = NULL;
			start = 0;
			end = 0;
			more = FALSE;
			return;
		}
		text = fragment->text->text;
		start = fragment->start;
		end = fragment->End();
		if (start >= end)
		{	more = FALSE;
			return;
		}
		more = TRUE;
		char_styles = fragment->text->char_styles;
		nr_char_styles = fragment->text->nr_char_styles;
		par_styles = fragment->text->par_styles;
		nr_par_styles = fragment->text->nr_par_styles;

		pos = 0;
		ch = text[pos];
		is_tab = ch == 9;
		is_newline = ch == 13;

		cur_par_style = 0;
		NewParStyle();
		
		cur_char_style = 0;
		NewCharStyle();
		
		while (pos < start)
			Next();
		in_new_frame = FALSE;
	}
	bool More()
	{	return more;
	}
	void Next()
	{	pos++;
		in_new_frame = FALSE;
		if (pos >= end)
		{	if (   cur_frame->next_reading_order == NULL
		        || cur_frame->geom_type != cur_frame->next_reading_order->geom_type)
			{	more = FALSE;
				return;
			}
			*r_cur_frame = cur_frame->next_reading_order;
			cur_frame = *r_cur_frame;
			Init();
			in_new_frame = TRUE;
			return;
		}	
		
		ch = text[pos];

		if (--par_style_chars_left == 0)
		{	cur_par_style++;
			NewParStyle();
		}
	
		if (--char_style_chars_left == 0)
		{	cur_char_style++;
			NewCharStyle();
		}

		if (font_family != 'G' && (ch == 0xCA || ch == 255))
		    ch = ' ';
		is_tab = ch == 9;
		is_newline = ch == 13;
	}
	void SkipSpaces()
	{	for (; More(); Next())
			if (ch != ' ')
				break;
	}
	void SkipSpacesAndControl()
	{	for (; More(); Next())
			if (ch > ' ')
				break;
	}
	bool ParseNumber(word &nr)
	{
		bool found_digit = FALSE;
		nr = 0;
	
		for (; More(); Next())
		{	if (ch < '0' || ch > '9')
				break;
			found_digit = TRUE;
			nr = nr * 10 + ch - '0';
		}
	
		return found_digit;
	}
	bool ParseChar(const char this_ch)
	{
		if (More() && ch == this_ch)
		{	Next();
			return TRUE;
		}
		else
			return FALSE;
	}
	bool ParseLiteral(const char *s)
	{
		if (!StartsWith(s))
			return FALSE;
			
		for (; *s != '\0' &&  More(); Next(), s++);
		
		return TRUE;
	}
	bool StartsWith(const char *s)
	{
		// Look ahead in text buffer
		lword i = pos;
		for (i = pos; *s != '\0' && i < end; i++, s++)
			if (text[i] != (byte)*s)
				return FALSE;
		return *s == '\0';
	}
	void SkipTillNewline()
	{	for (; More(); Next())
			if (is_newline)
				break;
	}
	void SkipLine()
	{	SkipTillNewline();
		if (More() && is_newline)
			Next();
	}
	void SkipEmptyLines()
	{	while(More() && is_newline)
			Next();
	}
	bool SingleCharTab()
	{	return    More() && ch >= 'a' && ch <= 'z' 
			   && pos+1 < end && text[pos+1] == '\t';
	}
	
	byte ch;
	bool is_tab;
	bool is_newline;
	bool in_new_frame;
	const char *par_style;
	char font_family;
	word size;
	word mode;

  private:
  	bool more;
  	CFrame **r_cur_frame;
	CFrame *cur_frame;
  	byte *text;
  	lword start;
  	lword end;
  	lword pos;
	TCharStyle *char_styles;
	lword nr_char_styles;
	lword cur_char_style;
	lword char_style_chars_left;
	TParStyle *par_styles;
	lword nr_par_styles;
	lword cur_par_style;
	lword par_style_chars_left;

  private:	

	void NewParStyle()
	{
		CParStyleDef *par_style_def = NULL;
		while (   cur_par_style < nr_par_styles
			   && par_styles[cur_par_style].len == 0)
			cur_par_style++;
		if (cur_par_style < nr_par_styles)
		{	par_style_chars_left = par_styles[cur_par_style].len;
			par_style_def = par_styles[cur_par_style].par_style;
		}
		else
			par_style_chars_left = (lword)-1;

		par_style = par_style_def ? par_style_def->name.A() : "<none>";
	}
	
	void NewCharStyle()
	{
		CCharStyle *char_style = NULL;
		while (   cur_char_style < nr_char_styles
			   && char_styles[cur_char_style].len == 0)
			cur_char_style++;
		if (cur_char_style < nr_char_styles)
		{	char_style_chars_left = char_styles[cur_char_style].len;
			char_style = char_styles[cur_char_style].char_style;
		}
		else
			char_style_chars_left = (lword)-1;

		if (char_style)
		{	SetFontFamily(char_style->FontFamilyName(0));
			size = char_style->size;
			mode = char_style->mode;
		}
		else
		{	font_family = ' ';
			size = 0;
			mode = 0;
		}
	}
	
	void SetFontFamily(char *font)
	{	font_family = '?';		
		
		if (font == NULL)
			font_family = ' ';
		else if (strstr(font, "Symbol"))
			font_family = 'S';
		else if (strstr(font, "Times"))
			font_family = 'T';
		else if (strstr(font, "Helvetica"))
			font_family = 'H';
	}
};	


