void dump_text_fragment(CTextFragment *text_fragment, FILE *fout)
{
	if (text_fragment == NULL)
		fprintf(fout, "<text fragment is null>");
	else
	{
		lword i, j;
		lword start = text_fragment->start,
		      end = text_fragment->End();
		fprintf(fout, "range[%ld %ld] ", start, end);
		if (end < start) printf("EMPTY");
		if (text_fragment->text.IsNull())
			fprintf(fout, "<text is null>");
		else if (text_fragment->text->text.IsNull())
			fprintf(fout, "<text->text is null>");
		else
		{	CText *text = text_fragment->text;
			fprintf(fout, "\n|");
			for (i = 0, j = start; i < 30 && j < end; i++, j++)
			{	byte ch = text->text[j];
				fprintf(fout, ch < ' ' || ch > 127 || ch == '#' ? "#%02X" : "%c", ch);
			}
			if (j < end)
				fprintf(fout, "...");
			fprintf(fout, "|");
			fprintf(fout,"\n");
			for (i = 0; i < text->nr_char_styles && i < 20; i++)
			{	fprintf(fout, "%s%ld", i == 0 ? "" : ", ",
					    text->char_styles[i].len);
				CCharStyle *char_style = text->char_styles[i].char_style;
				if (char_style)
					fprintf(fout, " %s:%d:%d",
					        char_style->FontFamilyName(0), char_style->size, char_style->mode);
			}
			fprintf(fout,"\n");
			for (i = 0; i < text->nr_par_styles && i < 20; i++)
			{	fprintf(fout, "%s%ld", i == 0 ? "" : ", ",
					    text->par_styles[i].len);
				CParStyleDef *par_style = text->par_styles[i].par_style;
				if (par_style)
					fprintf(fout, " %s", par_style->name.A());
			}

			fprintf(fout, "\n");
			CTextAccessor text_acc(text_fragment);
			//text_acc.mode.InnerOf(&text_acc.size);
			//text_acc.size.InnerOf(&text_acc.font_family_name);
			//text_acc.font_family_name.InnerOf(&text_acc.par_style);
			
			for (text_acc.Init(); text_acc.More(); text_acc.Next())
			{
				if (text_acc.mode.ToClose() /*&& text_acc.mode.close_value != 0*/)
					fprintf(fout, "</M>");
				if (text_acc.size.ToClose())
					fprintf(fout, "</S>");
				if (text_acc.font_family_name.ToClose())
					fprintf(fout, "</F>");
				if (text_acc.par_style.ToClose())
					fprintf(fout, "</P>\n");
				if (text_acc.par_style.ToOpen())
					fprintf(fout, "<P T=\"%s\">", text_acc.par_style.open_value);
				if (text_acc.font_family_name.ToOpen())
					fprintf(fout, "<F N=\"%s\">", text_acc.font_family_name.open_value);
				if (text_acc.size.ToOpen())
					fprintf(fout, "<S V=\"%d\">", text_acc.size.open_value);
				if (text_acc.size.ToOpen() /*&& text_acc.mode.open_value != 0*/)
					fprintf(fout, "<M V=\"%d\">", text_acc.mode.open_value);
				if (text_acc.ch != 0)
				{	byte ch = text_acc.ch;
					fprintf(fout, ch < ' ' || ch > 127 || ch == '#' ? "#%02X" : "%c", ch);
				}
			}
						
		}	
	}
}



void dump_frame(CFrame *frame, CPage *page, int depth, FILE *fout)
{
	fprintf(fout, "%*.*s  frame %3ld [%11ld,%11ld - %11ld,%11ld] ",
			depth, depth, "",
			frame->frame_nr,
	        frame->box.x1,
	        frame->box.y1,
	        frame->box.x2,
	        frame->box.y2);
	dump_text_fragment(frame->text_fragment, fout);

	if (frame->nr_sub_frames > 0)
	{
		fprintf(fout, "sub");
		for (word i = 0; i < frame->nr_sub_frames; i++)
		{	fprintf(fout, " %ld", frame->sub_frame_nrs[i]);
			CFrame *aframe = NULL;
			for (aframe = page->all_frames; aframe != NULL; aframe = aframe->next)
				if (aframe->frame_nr == frame->sub_frame_nrs[i])
				{	fprintf(fout, "*");
					break;
				}
		}					
	}
	fprintf(fout, "\n");
	fflush(fout);
	
	FOREACH_IN_LIST_N(CFrame, sub_frame, frame->children, next_child)
		dump_frame(sub_frame, page, depth+2, fout);
}

void dumpQXDoc(CQXDoc *doc, FILE *fout)
{

	word page_nr = 0;
	FOREACH_IN_LIST(CPage, page, doc->all_pages)
	{	page_nr++;
	
		if (!page->all_frames.IsNull())
		{
			fprintf(fout, "page %d", page_nr);
			if (page_nr <= doc->nr_template_pages) fprintf(fout, " template");
			fprintf(fout, "\n");
			fflush(fout);
			
			FOREACH_IN_LIST(CFrame, frame, page->all_frames)
				if (frame->parent.IsNull())
					dump_frame(frame, page, 0, fout);
		}
	}
}

void dump_text_fragment_to_HTML(CTextFragment *text_fragment, FILE *fout)
{
	if (text_fragment == NULL)
		fprintf(fout, "\n<I>text_fragment pointer is NULL<I>");
	else
	{
		lword start = text_fragment->start,
		      end = text_fragment->End();
		if (end < start) fprintf(fout, "\n<I>end before start<I>");
		if (text_fragment->text.IsNull())
			fprintf(fout, "\n<I>No Text</I>");
		else if (text_fragment->text->text.IsNull())
			fprintf(fout, "\n<I>No Text</I>");
		else
		{	
			fprintf(fout, "\n");
			CTextAccessor text_acc(text_fragment);
			
			for (text_acc.Init(); text_acc.More(); text_acc.Next())
			{
				if (text_acc.mode.ToClose())
				{	if (text_acc.mode.close_value & 288)
						fprintf(fout, "</SUP>");
					if (text_acc.mode.close_value & 2)
						fprintf(fout, "</I>");
					if (text_acc.mode.close_value & 1)
						fprintf(fout, "</B>");
					if (text_acc.mode.close_value & ~259)
						fprintf(fout, "</FONT>");	
				}
				if (text_acc.par_style.ToOpen())
					fprintf(fout, "<FONT COLOR=\"409090\">%s</FONT>", 
					        text_acc.par_style.open_value
					        ? text_acc.par_style.open_value 
					        : "***");
				if (text_acc.font_family_name.ToOpen()||text_acc.size.ToOpen())
				{	fprintf(fout, "<FONT COLOR=\"F02020\">%s:%d:%d</FONT>", 
					          text_acc.font_family_name.open_value
					        ? text_acc.font_family_name.open_value : "&lt;None&gt;", 
					        text_acc.size.open_value,
					        text_acc.mode.open_value);
				}
				if (text_acc.mode.ToOpen())
				{	
					if (text_acc.mode.open_value & 2048)
						fprintf(fout, "<FONT SIZE=-1>");	
					if (text_acc.mode.open_value & 1)
						fprintf(fout, "<B>");
					if (text_acc.mode.open_value & 2)
						fprintf(fout, "<I>");
					if (text_acc.mode.open_value & 288)
						fprintf(fout, "<SUP>");
				}
				if (text_acc.ch != 0)
				{	byte ch = text_acc.ch;
					if (text_acc.mode.open_value & 3072)
						ch = _toupper(ch);
					if (text_acc.is_newline || ch == 0x0d)
						fprintf(fout, "<BR>\n");
					else if (text_acc.is_tab)
						fprintf(fout, "&nbsp;&nbsp;");
					else if (ch == '<')
						fprintf(fout, "&lt;");
					else if (ch == '>')
						fprintf(fout, "&gt;");
					else if (ch == '&')
						fprintf(fout, "&amp;");
					else if (ch == 0xA9)
						fprintf(fout, "&copy;");
					else if (ch == 0xD0)
						fprintf(fout, "--");
					else if (ch == 0xD4 || ch == 0xD5)
						fprintf(fout, "'");
					else if (ch == 0xD2 || ch == 0xD3)
						fprintf(fout, "\"");
					else if (ch == 0xF8)
						fprintf(fout, "<SUP>-</SUP>");
					else if (ch == 0xCA)
						fprintf(fout, "&nbsp;");
					else if (ch == 0xE9 || ch == 0x91)
						fprintf(fout, "&euml;");
					else if (ch == 0x95)
						fprintf(fout, "&iuml;");
					else if (ch == 0x88)
						fprintf(fout, "&agrave;");
					else if (ch == 0x8A)
						fprintf(fout, "&auml;");
					else if (ch == 0x9A)
						fprintf(fout, "&ouml;");
					else if (ch == 0x86 || ch == 0x9F)
						fprintf(fout, "&uuml;");
					else if (ch == 0x97)
						fprintf(fout, "&oacute;");
					else if (ch == 0x90)
						fprintf(fout, "&ecirc;");
					else if (ch == 0x8E)
						fprintf(fout, "&eacute;");
					else if (ch == 0x8F)
						fprintf(fout, "&egrave;");
					else if (ch == 0x8D)
						fprintf(fout, "&ccedil;");
					else if (ch == 4)
						fprintf(fout, "<BIG>[]</BIG>");
					else if (ch >= ' ')	
						fprintf(fout, ch > 127 || ch == '#' ? "#%02X" : "%c", ch);
				}
			}
		}	
	}
}



void dump_frame_to_HTML(CFrame *frame, CPage *page, int depth, FILE *fout)
{
	fprintf(fout, "\n<LI><B>frame %3ld</B> [%11ld,%11ld - %11ld,%11ld]<BR>",
			frame->frame_nr,
	        frame->box.x1,
	        frame->box.y1,
	        frame->box.x2,
	        frame->box.y2);

	if (!frame->children.IsNull())
	{	fprintf(fout, "\n<UL>");	        
		FOREACH_IN_LIST_N(CFrame, sub_frame, frame->children, next_child)
			dump_frame_to_HTML(sub_frame, page, depth+2, fout);
		fprintf(fout, "\n</UL>");
	}
	dump_text_fragment_to_HTML(frame->text_fragment, fout);

}

void dumpQXDoc_to_HTML(CQXDoc *doc, FILE *fout)
{
	
	word page_nr = 0;
	FOREACH_IN_LIST(CPage, page, doc->all_pages)
	{	page_nr++;
	
		if (!page->all_frames.IsNull() && page_nr > doc->nr_template_pages)
		{
			fprintf(fout, "\n<H1> page (%d) %s</H1>\n\n<UL>", 
				    page_nr, page->side_by_side ? "side-by-side" : "");

			FOREACH_IN_LIST(CFrame, frame, page->all_frames)
				if (frame->parent.IsNull())
					dump_frame_to_HTML(frame, page, 0, fout);

			fprintf(fout, "\n</UL>\n");
		}
	}
}
