/*	
	MMFile.cpp: This file demonstrates how an object-oriented
    persistant store can be implemented using Memory Mapped Files.

	Copyright (c) 2002 Frans J. Faase (http://home.planet.nl/~faase009)

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation - version 2. 

	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License in the file COPYING
	for more details. 

	You should have received a copy of the GNU General Public
	License along with this program; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA

	GNU GPL can be found at http://home.planet.nl/~faase009/GNU.txt
	
---------------------------------------------------------------------------	
	
	Introduction
	~~~~~~~~~~~~
	This file includes a number of classes for working
	with objects inside a Memory Mapped File (MMF). This
	implements a simple persistent store for objects.
   
	A Memory Mapped File is a facility which allows a
	file to be treated as in memory. The operating
	system is responsable for reading and writing 
	as a result of reading and modifying the date in
	the memory.

	Object to be stored in the MMF should be derived
	from the class CMMFObject. Any pointers inside
	these classes should be implemented with one of:
	- CMMFPointer<T> 
	- CMMFArray<T>
	- CMMFByteArray
	- CMMFWordArray
	- CMMFLWordArray
	- CMMFString
	These classes take care that memory addresses are
	translated to file offsets and vice-versa. This
	is needed because the base address of a memory
	mapped file can change at each execution of the
	program using the MMF. Storing memory addresses
	inside a file will thus never work if one wants
	to use the file as a persistent store.
	
	Limitations:
	- When the file is opened one has to specify a
	  maximum size of the persistent store. This
	  space should be available on the disk where
	  the file is located.
	- The implementation allows only one persistent
	  store to be opened at the same time. There is
	  no simple solution to extending this to more
	  than one store. (One of the reasons is that
	  the new operator does not take any arguments.)
	- Although a conversion operator is supplied,
	  in case a CMMFPointer<T> value is assigned to
	  a T* variable, in some case (for example, when
	  used as an argument in a print statement) the
	  pointer to the underlying object need to be
	  explicitly retrieved using the A() method.
	- Assumes that lword, word and byte have been
	  defined as the unsigned long, short and char.
	- The neccesary include files are not included 
	  in this file.
	- This file only has only been tested under Win98 SE
	  with the Cygnus gcc compiler.
*/   


// Singelton object for storing objects in a MMF
class CMMFile
{
  public:
  	bool trace;
  	
  	CMMFile() 
  	{	open = 0;
  		trace = FALSE;
  	}

	//***  Public methodes 
  	
	bool Open(char *fn, lword alloc_size)
	// Opens the persistant store under for the file
	// specified with fn with a maximum allocated size
	// of alloc_size. If the file does not exist yet,
	// it is created and initialized.
	{
		if (open)
		{	fprintf(stderr, "\nFatal: Already open");
			return FALSE;
		}
		
		f = CreateFile(fn, 
			GENERIC_READ | GENERIC_WRITE,
			0, // don't share
			NULL, // no security attributes
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
			NULL); // no template
		if (f == INVALID_HANDLE_VALUE)
		{	printf("\nFatal: Failed to open %s", fn);
			return FALSE;
		}

	    file_size = GetFileSize(f, NULL);
	    printf("\nInfo: Open file size = %ld\n", file_size);
	    if (file_size > alloc_size)
	    {	fprintf(stderr, "\nFatal: The size of the file (=%ld) is larger than allocated size (=%ld)",
	                file_size, alloc_size);
	    }
		
		fm = CreateFileMapping(f,
			NULL, // no security attributes
			PAGE_READWRITE,	0, alloc_size,
			NULL); // no name for mapping
	    if (fm == NULL)
	    {	fprintf(stderr, "\nFatal: Failed to open %s for size %ld", fn, alloc_size);
			CloseHandle(f);
	    	return FALSE;
	    }

	    base_address = (lword)MapViewOfFile(fm, FILE_MAP_WRITE,
	    	0, 0, // start at the beginning
	    	alloc_size);
	    if (base_address == 0)
	    {	fprintf(stderr, "\nFatal: Failed to retrieve base address for %s with size %ld", fn, alloc_size);
			CloseHandle(fm);
			CloseHandle(f);
	    	return FALSE;
	    }
	    open = TRUE;
	    
	    if (file_size == 0)
	    	Reset();
	    else
	    	file_size = GetCurrentFileSize();
	    printf("\nInfo: Now file size = %ld\n", file_size);
	    
	    max_size = alloc_size;
	    
	    return TRUE;
	}

	void *FirstObject()
	// Returns a pointer to the first object, if any.
	{
		if (file_size <= 6*sizeof(lword))
			return NULL;
		else
			return ToPointer(6*sizeof(lword));
	}
	
	void Reset()
	// Resets the persistent store, such that it will
	// not contain any objects at all.
	{	if (!open)
			return;
		
		// Allocate room for file size and root free mem object
		file_size = 5*sizeof(lword);
		// Initialize root free mem object
		memset(ToPointer(4), 0, 4*sizeof(lword));
		
		SetCurrentFileSize(file_size);
	}
	
	void Flush()
	// Write all modifications made to the file to the disk.
	{	if (!open)
			return;

		FlushViewOfFile((void*)base_address, file_size);
	}

	inline void CompactFreeSpace()
	// Tries to combine
	{	CheckAllBlocks(FALSE, TRUE);
	
	}
	
	void Close()
	// Closes the persistent store.
	{	if (!open)
			return;

		SetCurrentFileSize(file_size);
    	printf("\nInfo: File size = %ld\n", GetCurrentFileSize());

		UnmapViewOfFile((void*)base_address);
		CloseHandle(fm);

		// adjust file size
		SetFilePointer(f, file_size, 0, FILE_BEGIN);
		SetEndOfFile(f);

		CloseHandle(f);
		open = FALSE;
	}
	
	void *Allocate(size_t size)
	// To allocate an object inside the persistent store. returns pointer.
	{	void *p = InternalAllocate(size);
		memset((byte*)p, 0, size);
		return p;
	}

	inline void Free(void *p)
	// Frees memory allocated earlier from the persistent store.
	{	InternalFree(p);
	}

	// *** Public methods used by the auxilary classes
	
	lword AllocateOffset(size_t size)
	// Same as Allocate, except that it returns the offset from the start 
	// of the file. (Used by array and string classes.)
	{	void *p = InternalAllocate(size);
		memset((byte*)p, 0, size);
		return ToOffset(p);
	}
	
	// Null representation and test
	static const lword Null = (lword)-1;
	inline bool IsNull(lword offset)
	{	return offset == Null;
	}
	
	// Converting offset to pointer
	inline void *ToPointer(lword offset)
	{	return (void*)(base_address + offset);
	}
	inline void *ToPointerWithNull(lword offset)
	{	return IsNull(offset) ? NULL : ToPointer(offset);
	}
	
	// Converting pointer to offset
	inline lword ToOffset(void *p)
	{	return (lword)p - base_address;
	}
	inline lword ToOffsetWithNull(void *p)
	{	return (p == NULL) ? Null : ToOffset(p);
	}
	inline void SetOffset(lword &offset, void *p)
	{	if (p == NULL)
			offset = Null;
		else
			offset = ToOffset(p);
	}

	// Debuggin function
	bool Dump()
	{	return    CheckAllBlocks(TRUE, FALSE)
			   && CheckFreeMem(TRUE);
	}
	
  private:
  	bool open;
	void *f;
	void *fm;
	lword max_size;
	lword file_size;
	lword base_address;
  
  	inline void SetCurrentFileSize(lword new_file_size)
  	{	*(lword*)base_address = file_size;
  	}
  	inline lword GetCurrentFileSize()
  	{	return *(lword*)base_address;
  	}

	inline lword RootFreeMemOffset() { return sizeof(lword); }

	struct CFreeMem 
	{
		lword size;
		lword next;
		lword next_larger;
		lword back;
		inline CFreeMem *Next() { return ToFreeMem(next); }
		inline void SetNext(lword offset)
		{	if ((next = offset) != 0) ToFreeMem(next)->back = ToOffset(this);
		}
		inline CFreeMem *NextLarger() { return ToFreeMem(next_larger); }
		inline void SetNextLarger(lword offset)
		{	if ((next_larger = offset) != 0) ToFreeMem(next_larger)->back = ToOffset(this);
		}
		inline CFreeMem *Back() { return ToFreeMem(back); }
		void *Address() { return (void*)((long)this + 4); }
	  private:
		static inline CFreeMem *ToFreeMem(lword offset);   // -- see below for 
		static inline lword ToOffset(CFreeMem *p);         //    implementation
	};
	
	inline CFreeMem *ToFreeMem(lword offset) { return (CFreeMem*)ToPointer(offset); }
	
	void *InternalAllocate(lword size)
	{	
		// normalize size as multiple of 4 and at least 16 (and incremented with 4)
		size = (size + 7) & ~3L;
		if (size < 16) size = 16;
		
	    CFreeMem *r = ToFreeMem(RootFreeMemOffset());

		for (;;)
		{	r = r->NextLarger();
			if (ToOffset(r) == 0)
				return AllocateByExtending(size);
			if (r->size >= size)
				break;
		}

		// use if size matches (with atmost 12.5% space to be unused)
		if (r->size <= size + size/8)
		{	ExtractWithThisSize(r);

			r->size = ~r->size;
			return r->Address();
		}

		// search for block that we can split
		lword size2 = size < 50 ? size * 2 : size + 100;
		for (;;)
		{	r = r->NextLarger();
			if (ToOffset(r) == 0)
				return AllocateByExtending(size);
			if (r->size >= size2)
				break;
		}

		ExtractWithThisSize(r);
		lword org_size = r->size;
		CFreeMem *second_part = (CFreeMem*)((lword)r + size);
		second_part->size = org_size - size;
		Insert(second_part);
		
		r->size = ~size;
		return r->Address();
	}
	inline void ExtractWithThisSize(CFreeMem *&r)
	{
	    if (r->next != 0)
		{	CFreeMem *next = r->Next();
			r->SetNext(next->next);
			r = next;
		}
		else
		{	CFreeMem *back = r->Back();
			back->SetNextLarger(r->next_larger);
		}
	}
	inline void Extract(CFreeMem *r)
	{
		CFreeMem *back = r->Back();
		if (back->size == r->size)
		{	ASSERT(back->Next() == r);
			back->SetNext(r->next);
		}
		else 
		{	ASSERT(back->NextLarger() == r);
		
			if (r->next == 0)
				back->SetNextLarger(r->next_larger);
			else
			{	CFreeMem *next = r->Next();
				back->SetNextLarger(r->next);
				next->SetNextLarger(r->next_larger);
			}
		}
	}
	inline void *AllocateByExtending(lword size)
	{	if (file_size + size >= max_size)
		{	fprintf(stderr, "\nFatal: Out of memory");
			exit(1);
		}
		CFreeMem *r = ToFreeMem(file_size);
		file_size += size;

		r->size = ~size;
		return r->Address();
	}

	inline void InternalFree(void *p)
	{
		CFreeMem *r = (CFreeMem*)((lword)p - 4);
		ASSERT((r->size & 3) == 3);
		r->size = ~r->size;
		lword offset_next = ToOffset(r) + r->size;
		if (offset_next < file_size)
		{	CFreeMem *next = ToFreeMem(offset_next);
			if ((next->size & 3) == 0)
			{	// -- next sequential block is also free
				
				Extract(next);
				r->size += next->size;
			}
		}
		Insert(r);
	}

	inline void Insert(CFreeMem *insert)
	{	lword size = insert->size;

	    CFreeMem *r = ToFreeMem(RootFreeMemOffset());
	    
	    CFreeMem *r_next_larger;
	    for (;;)
	    {	if (r->next_larger == 0)
			{	r->next_larger = ToOffset(insert);
				insert->next = 0;
				insert->next_larger = 0;
				insert->back = ToOffset(r);
				return;
			}
	    	r_next_larger = r->NextLarger();
	    	if (r_next_larger->size >= size)
	    		break;
	    	r = r_next_larger;
	    }
		
		if (r_next_larger->size == size)
		{	insert->next_larger = 0;
			insert->SetNext(r_next_larger->next);
			r_next_larger->SetNext(ToOffset(insert));
		}
		else // r_next_larger->size > size
		{	insert->SetNextLarger(r->next_larger);
			r->SetNextLarger(ToOffset(insert));
			insert->next = 0;
		}
	}

	bool CheckAllBlocks(bool dump, bool compact)
	{
		// Traverse all blocks
		CFreeMem *r = ToFreeMem(RootFreeMemOffset());
		CFreeMem *end = ToFreeMem(file_size);
		while (r < end)
		{
			if (r->size == 0)
			{
				if (dump)
					printf("\nAt %4ld: Root block of size 0",
						   ToOffset(r));
				r = ToFreeMem(ToOffset(r) + 16);
			}
			else if ((r->size & 3) == 3)
			{	lword size = ~r->size;
				if (dump)
					printf("\nAt %4ld: Allocated of size %ld", 
				    	   ToOffset(r), size);
				
				r = ToFreeMem(ToOffset(r) + size);
			}
			else if ((r->size & 3) == 0)
			{	CFreeMem *first_free_mem = r;
				CFreeMem *prev_r = NULL;
				lword free_size = 0;
				
				do 
				{	lword size = r->size;
					if (dump)
						printf("\nAt %4ld: Free block of size %ld, back = %ld, next = %ld, next_larger = %ld",
				    		   ToOffset(r), size, r->back, r->next, r->next_larger);

					if (compact && prev_r != NULL)
					{	free_size += prev_r->size;
						Extract(prev_r);
					}
					prev_r = r;
					
					r = ToFreeMem(ToOffset(r) + size);
				}
				while (r < end && (r->size & 3) == 0);
				
				if (compact && free_size > 0)
				{	free_size += prev_r->size;
					Extract(prev_r);
					
					first_free_mem->size = free_size;
					Insert(first_free_mem);
				}
			}
			else
			{	printf("\nAt %4ld: Corrupted.\n", ToOffset(r));
				return FALSE;
			}
		}
		if (r != end)
		{	printf("\nAt %4ld: Past end at %ld.\n",
				   ToOffset(r), ToOffset(end));
			return FALSE;
		}
		
		return TRUE;
	}
	bool CheckFreeMem(bool dump) 	
	{	
		CFreeMem *r = ToFreeMem(RootFreeMemOffset());
		for(;;)
		{	if (   r->next_larger != 0
				&& r->NextLarger()->back != ToOffset(r))
			{	printf("\nError: next larger block %ld, back points to %ld instead of %ld\n",
					   r->next_larger, r->NextLarger()->back, ToOffset(r));
				return FALSE;
			}
		
			lword size = r->size;
			if (dump) 
				printf("\nFor size %ld :", size);
			CFreeMem *n;
			for (n = r; ToOffset(n) != 0; n = n->Next())
			{	if (n->size != size)
				{	if (dump)
						printf("\nError: block at %ld has size %ld\n",
							   ToOffset(n), n->size);
					return FALSE;
				}
				if (   n->next != 0
					&& n->Next()->back != ToOffset(n))
				{	printf("\nError: next block %ld, back points to %ld instead of %ld\n",
						   n->next, n->Next()->back, ToOffset(n));
					return FALSE;
				}
				if (dump)
					printf(" %ld", ToOffset(n));
			}
			if (r->next_larger == 0)
				break;
			r = r->NextLarger();
		}
		if (dump)
			printf("\nReady\n");
		return TRUE;
	}

} mmfile;

// (These need to be placed here, because they make use of mmfile.)
CMMFile::CFreeMem *CMMFile::CFreeMem::ToFreeMem(lword offset) 
{ 	return (CMMFile::CFreeMem*)mmfile.ToPointer(offset);
}
lword CMMFile::CFreeMem::ToOffset(CMMFile::CFreeMem *p)
{	return mmfile.ToOffset(p);
}




// Base class for object inside a MMF
class CMMFObject
{
  public:
	void *operator new(size_t size)
	{	return mmfile.Allocate(size);
	}
	void operator delete(void *p)
	{	mmfile.Free(p);
	}
};



template<class T>
class CMMFPointer
{
  public:
	CMMFPointer() { offset = mmfile.Null; }
	inline bool IsNull() { return mmfile.IsNull(offset); }
	inline T *A() { return (T*)mmfile.ToPointerWithNull(offset); }	
	inline operator T*() { return A(); }
	inline T *operator->() { return A(); }
	inline T *operator=(T *new_val) 
	{	mmfile.SetOffset(offset, new_val);
		return new_val;
	}
	
  private:	
	lword offset;
};

template<class T>
class CMMFArray
{
  public:
	CMMFArray() { offset = mmfile.Null; }
	inline T &operator[](lword i) {	return A()[i]; }
	inline T *A() { return (T*)mmfile.ToPointer(offset); }
	inline operator T*() { return A(); }
	inline bool IsNull() { return mmfile.IsNull(); }
	void Allocate(lword nr)
	{	offset = mmfile.AllocateOffset(nr * sizeof(T));
	}

  private:	
	lword offset;
};


class CMMFByteArray
{
  public:
	CMMFByteArray() { offset = mmfile.Null; }
	inline byte &operator[](lword i) { return A()[i]; }
	inline operator byte*() { return A(); }
	inline byte *A() { return (byte*)mmfile.ToPointer(offset); }
	inline bool IsNull() { return mmfile.IsNull(offset); }
	void Allocate(lword nr)
	{	offset = mmfile.AllocateOffset(nr * sizeof(byte));
	}
	
  private:	
	lword offset;
};


class CMMFWordArray
{
  public:
	CMMFWordArray() { offset = mmfile.Null; }
	inline word &operator[](lword i) { return A()[i]; }
	inline operator word*() { return A(); }
	inline word *A() { return (word*)mmfile.ToPointer(offset); }
	inline bool IsNull() { return mmfile.IsNull(offset); }
	void Allocate(lword nr)
	{	offset = mmfile.AllocateOffset(nr * sizeof(word));
	}
	
  private:	
	lword offset;
};


class CMMFLWordArray
{
  public:
	CMMFLWordArray() { offset = mmfile.Null; }
	inline lword &operator[](lword i) { return A()[i]; }
	inline operator lword*() { return A(); }
	inline lword *A() {	return (lword*)mmfile.ToPointer(offset); }
	inline bool IsNull() { return mmfile.IsNull(offset); }
	void Allocate(lword nr)
	{	offset = mmfile.AllocateOffset(nr * sizeof(lword));
	}
	
  private:	
	lword offset;
};


class CMMFString
{
  public:
	CMMFString() { offset = mmfile.Null; }
	inline char *A() { return (char*)mmfile.ToPointerWithNull(offset); }
	inline char operator[](lword i) { return A()[i]; }
	inline operator char*() { return A(); }
	char *operator=(char *new_val)
	{	mmfile.SetOffset(offset, new_val);
		return new_val;
	}
	void Allocate(lword nr)
	{	offset = mmfile.AllocateOffset(nr);
	}
	void StrCopy(char *new_val)
	{	offset = mmfile.AllocateOffset(strlen(new_val)+1);
		strcpy((char*)mmfile.ToPointer(offset), new_val);
	}
	
  private:	
	lword offset;
};







