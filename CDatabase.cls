class CDatabase : public CMMFObject
{
  public:

	CBook *AppendNewBook(char *name)
	{	FOREACH_IN_LIST_REF(CBook, r_book, all_books);
		*r_book = new CBook(name);
		return (*r_book);
	}

	CMMFPointer<CBook> all_books;
};

class CBook : public CMMFObject
{
  public:
  
  	CBook(char *new_name)
  	{	name.StrCopy(new_name);
  	}
	CQXDoc *AppendNewQXDoc(char *name)
	{	FOREACH_IN_LIST_REF(CQXDoc, r_doc, all_QX_docs);
		*r_doc = new CQXDoc(name);
		return (*r_doc);
	}

	CMMFString name;
	CMMFPointer<CQXDoc> all_QX_docs;
	CMMFPointer<CFrame> first_frame_reading_order;
	CMMFPointer<CBook> next;
};
