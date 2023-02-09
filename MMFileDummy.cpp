/*	
	MMFileDummy.cpp: a replacement for MMFile.cpp that
	does not use map memory to a file.
*/   


// Singelton object for storing objects in a MMF
class CMMFile
{
  public:
  	
  	CMMFile() 
  	{	}

	//***  Public methodes 
  	
	bool Open(char *fn, lword alloc_size) { return TRUE; }

	lword firstobject[6];
	void *FirstObject() { return 0; }
	
	void Reset() {}
	void Flush() {}
	inline void CompactFreeSpace() {}
	void Close() {}

	void *Allocate(size_t size) 
	{	void *p = malloc(size);
		memset((byte*)p, 0, size);
		return p;
	}

	inline void Free(void *p)
	{	if (p) free(p);
	}

	lword AllocateOffset(size_t size)
	{	void *p = malloc(size);
		memset((byte*)p, 0, size);
		return ToOffset(p);
	}
	
	// Null representation and test
	static const lword Null = 0;
	inline bool IsNull(lword offset)
	{	return offset == Null;
	}
	
	// Converting offset to pointer
	inline void *ToPointer(lword offset)
	{	return (void*)offset;
	}
	inline void *ToPointerWithNull(lword offset)
	{	return (void*)offset;
	}
	
	// Converting pointer to offset
	inline lword ToOffset(void *p)
	{	return (lword)p;
	}
	inline lword ToOffsetWithNull(void *p)
	{	return (lword)p;
	}
	inline void SetOffset(lword &offset, void *p)
	{	offset = (lword)p;
	}

	bool Dump() { return TRUE; }
	
} mmfile;



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

