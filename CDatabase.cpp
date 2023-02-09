
// Classes defined in this file


#line 1 "CDatabase.cls"
class CDatabase;
#line 14 "CDatabase.cls"
class CBook;

// Interfaces for the classes


#line 1 "CDatabase.cls"
class CDatabase : public CMMFObject
{
  public:

	CBook *AppendNewBook(char *name);
#line 10 "CDatabase.cls"

	CMMFPointer<CBook> all_books;
};

#line 14 "CDatabase.cls"
class CBook : public CMMFObject
{
  public:
  
  	CBook(char *new_name);
#line 21 "CDatabase.cls"
	CQXDoc *AppendNewQXDoc(char *name);
#line 26 "CDatabase.cls"

	CMMFString name;
	CMMFPointer<CQXDoc> all_QX_docs;
	CMMFPointer<CFrame> first_frame_reading_order;
	CMMFPointer<CBook> next;
};

// Implementation of the class methodes


// Methods for the class CDatabase

#line 5 "CDatabase.cls"
	CBook *CDatabase::AppendNewBook(char *name)
	{	FOREACH_IN_LIST_REF(CBook, r_book, all_books);
		*r_book = new CBook(name);
		return (*r_book);
	}
#line 13 "CDatabase.cls"


// Methods for the class CBook

#line 18 "CDatabase.cls"
  	CBook::CBook(char *new_name)
  	{	name.StrCopy(new_name);
  	}
	CQXDoc *CBook::AppendNewQXDoc(char *name)
	{	FOREACH_IN_LIST_REF(CQXDoc, r_doc, all_QX_docs);
		*r_doc = new CQXDoc(name);
		return (*r_doc);
	}