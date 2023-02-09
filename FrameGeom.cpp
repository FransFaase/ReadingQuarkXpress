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

#define MID_OF_PAGE 70000000

/* a collection of geometrical frame types */
const byte gt_empty  = 0;
const byte gt_normal = 1;
const byte gt_header = 2;

const char *GeomTypeName(byte gt)
{	switch (gt)
	{	case gt_empty:  return "empty";
		case gt_normal: return "normal";
		case gt_header: return "header";
	}
	return "???";
}
 
bool InRange(lword v, lword f, lword t)
{	return v >= f && v <= t;
}

bool InXRange(CBox *box, lword x1f, lword x1t, lword x2f, lword x2t)
{	return InRange(box->x1, x1f, x1t) && InRange(box->x2, x2f, x2t);
}

void SetGeomType(CFrame *frame)
{	
	byte geom_type = gt_normal;
	/* Some algorithm to set the geometrical frame type
	   bases on the geometrical properties of the frame.
	   For example: */
	if (!frame->HasText())
		geom_type = gt_empty;

	frame->geom_type = geom_type;
}

int FrameReadingOrder(CFrame *f1, CFrame *f2)
{	return CBox::ReadingOrder(f1->box, f2->box);
}

int FrameReadingOrderPageByPage(CFrame *f1, CFrame *f2)
{	
	bool f1_on_the_left = f1->box.x1 < MID_OF_PAGE;
	bool f2_on_the_left = f2->box.x1 < MID_OF_PAGE;
	if (f1_on_the_left && !f2_on_the_left)
		return -1;
	if (!f1_on_the_left && f2_on_the_left)
		return 1;
	return CBox::ReadingOrder(f1->box, f2->box);
}

void CheckSuccesiveFrames(CFrame *prev_frame, CFrame *frame, word page_nr)
{	int comp = CBox::ReadingOrder(prev_frame->box, frame->box);
	if (comp < -2)
		; // -- in correct order
	else if (comp == 0 && CBox::Equal(prev_frame->box, frame->box))
		/*printf("\n  Warning: frames %ld and %ld are equal",
			   prev_frame->frame_nr, frame->frame_nr)*/;
	else
	{
		if (comp == -2)
			/*printf("\n  Error: frames %ld and %ld not in strict reading order",
			   	   prev_frame->frame_nr, frame->frame_nr)*/;
		else if (   !prev_frame->text_fragment.IsNull()
				 && !frame->text_fragment.IsNull()
				 && !prev_frame->text_fragment->IsEmpty()
				 && !frame->text_fragment->IsEmpty())
		{ 	printf("\n  Error: On page %d frames %ld and %ld in wrong order: '%s'",
			   	   page_nr, prev_frame->frame_nr, frame->frame_nr,
			   	   CBox::ReadingOrderDescription(prev_frame->box, frame->box));
			CBox inter_box = CBox::Intersection(prev_frame->box, frame->box);
			if (inter_box.Dx() != 0 && inter_box.Dy() != 0)
				printf(" Overlap: [%6.3f %6.3f %f]",
					   inter_box.Area()/prev_frame->box.Area(),
					   inter_box.Area()/frame->box.Area(),
					   inter_box.Area());
			else
				printf(" No overlap");
			printf(" [%ld, %ld - %ld, %ld]",
				   prev_frame->box.x1, prev_frame->box.y1, 
				   prev_frame->box.x2, prev_frame->box.y2);
			printf(" [%ld, %ld - %ld, %ld]",
				   frame->box.x1, frame->box.y1, 
				   frame->box.x2, frame->box.y2);
			printf("\n     ");
			dump_text_fragment(prev_frame->text_fragment, stdout);		   
			printf("\n     ");
			dump_text_fragment(frame->text_fragment, stdout); 
		}
	}
}

void CheckSubFrames(CFrame *parent_frame, word page_nr)
{
	CFrame *prev_frame = NULL;
	if (!parent_frame->children.IsNull())
	{	FOREACH_IN_LIST_N(CFrame, frame, parent_frame->children, next_child)
		{	if (prev_frame)
				CheckSuccesiveFrames(prev_frame, frame, page_nr);
			CheckSubFrames(frame, page_nr);
			prev_frame = frame;
		}
	}
}

bool IsSideBySide(CPage *page)
{
	return TRUE; // -- dummy implementation
	
	/* Function which determines whether the pages should
	   be read side by side. Example code:
	   
	FOREACH_IN_LIST(CFrame, frame, page->all_frames)
	{	char *par_style = frame->FirstParStyle();
		if (...some test...)
			return TRUE;
	}
	
	return FALSE;
	*/
}

void PrintFrameId(CFrame *frame)
{	printf("frame %ld on page %d of %s",
		   frame->frame_nr,
		   frame->page->page_nr,
		   frame->page->doc->name.A());
}

void ReorganizeInsideFrames(CMMFPointer<CFrame> *r_first_frame,
							int (*order)(CFrame *f1, CFrame *f2))
{
	CMMFPointer<CFrame> *r_frame = r_first_frame;
	while (!(*r_frame).IsNull())
	{
		CFrame *frame;
		for (frame = (*r_first_frame); frame != NULL; frame = frame->next_child)
			if (CBox::Inside((*r_frame)->box, frame->box) > 0)
				break;
		
		if (frame != NULL)
		{	
			printf("\nInfo: Put child frame inside other child:\n - ");
			PrintFrameId((*r_frame));
			printf("\n - ");
			PrintFrameId(frame);
			printf("\n");
			fflush(stdout);
			
			// remove the current frame
			CFrame *cur_frame = (*r_frame);
			*r_frame = (*r_frame)->next_child;
			
			// insert it as a child of the frame found
			cur_frame->parent = NULL;
			frame->InsertChild(cur_frame, order);
		}
		else
			r_frame = &(*r_frame)->next_child;
	}
	
	// Recursively call ReorganizeInsideFrame
	CFrame *frame;
	for (frame = (*r_first_frame); frame != NULL; frame = frame->next_child)
		ReorganizeInsideFrames(&frame->children, order);
}

void AnalyzeFrameGeometry(CPage *page, word page_nr)
{
	page->side_by_side = IsSideBySide(page);

	int (*order)(CFrame *, CFrame *);
	if (page->side_by_side)
		order = FrameReadingOrder;
	else
		order = FrameReadingOrderPageByPage;
	
	// Determine geometric type of each frame
	FOREACH_IN_LIST(CFrame, frame, page->all_frames)
		SetGeomType(frame);
			
	// First put all sub frames under their correct parent
	FOREACH_IN_LIST(CFrame, frame, page->all_frames)
		if (frame->nr_sub_frames > 0)
			for (lword i = 0; i < frame->nr_sub_frames; i++)
			{	CFrame *sub_frame = page->GetFrameWithNr(frame->sub_frame_nrs[i]);
				if (sub_frame != NULL)
					if (!sub_frame->parent.IsNull())
						if (sub_frame->parent->frame_nr == frame->frame_nr)
							printf("\n  Warning: sub_frane %ld already inserted into frame %ld",
								   sub_frame->frame_nr, frame->frame_nr);
						else
							printf("\n  Error: cannot insert sub_frane %ld into frame %ld because it is already inserted in %ld",
								   sub_frame->frame_nr, frame->frame_nr, sub_frame->parent->frame_nr);
					else
					{	//printf("\n  Insert sub_frame %ld into frame %ld", sub_frame->frame_nr, frame->frame_nr);
						frame->InsertChild(sub_frame, order);
					}
			}
			
   // See, if any of the remaining frames fit inside one of the other frames			
	FOREACH_IN_LIST(CFrame, frame, page->all_frames)
		if (frame->parent.IsNull())
		{	CFrame *candidate_parent_frame = NULL;
			FOREACH_IN_LIST(CFrame, other_frame, page->all_frames)
				if (   other_frame != frame
					&& CBox::Inside(frame->box, other_frame->box) > 0
					&& (   candidate_parent_frame == NULL
						|| CBox::Inside(other_frame->box, candidate_parent_frame->box) > 0))
					candidate_parent_frame = other_frame;
			if (candidate_parent_frame)
			{	//printf("\n  Insert frame %ld into frame %ld because it is inside", 
				//       frame->frame_nr, candidate_parent_frame->frame_nr);
				candidate_parent_frame->InsertChild(frame, order);
			}
		}

	// Also does this for all child frames
	FOREACH_IN_LIST(CFrame, frame, page->all_frames)
		if (frame->parent.IsNull())
			ReorganizeInsideFrames(&frame->children, order);	
 
 	// Sort all the top level frames, leaving out the frames that have a parent
	CFrame *frame = page->all_frames, *next_frame;
	page->all_frames = NULL;
	for (; frame; frame = next_frame)
	{	next_frame = frame->next;
		frame->next = NULL;	
		if (frame->parent.IsNull())
			page->InsertFrame(frame, order);
	}
}

void AnalyzeFrameGeometry(CQXDoc *doc)
{
	word page_nr = 1;
	FOREACH_IN_LIST(CPage, page, doc->all_pages)
	{
		if (page_nr > doc->nr_template_pages)
		{	//printf("\nPage %d", page_nr);
			AnalyzeFrameGeometry(page, page_nr);
		}

		page_nr++;	
	}
}

CMMFPointer<CFrame> *r_cur_frame_reading_order;

void SetReadingOrder(CFrame *frame)
{
	FOREACH_IN_LIST_N(CFrame, sub_frame, frame->children, next_child)
		SetReadingOrder(sub_frame);
		
	if (frame->geom_type != gt_empty && frame->geom_type != gt_header)
	{	(*r_cur_frame_reading_order) = frame;
		r_cur_frame_reading_order = &frame->next_reading_order;
	}
}

void SetReadingOrder(CBook *book)
{
	r_cur_frame_reading_order = &book->first_frame_reading_order;
	
	FOREACH_IN_LIST(CQXDoc, doc, book->all_QX_docs)
	{
		FOREACH_IN_LIST(CPage, page, doc->all_pages)
		{
			if (page->page_nr > doc->nr_template_pages)
			{
				FOREACH_IN_LIST(CFrame, frame, page->all_frames)
				{
					if (frame->parent.IsNull())
						SetReadingOrder(frame);
				}
			}
		}
	}
}
	
