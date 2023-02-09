class CBuf
{
protected:
  	lword _alloc_length;
	byte *_buf;
	lword _length;
private:
	byte *_pa_buf;
	lword _pa_len;
public:

	CBuf(void)
	{
		_buf = NULL;
		_length = 0;
		_alloc_length = 0;
		_pa_buf = NULL;
		_pa_len = 0L;
	}
	CBuf(byte *pa_buf, lword pa_len)
	{
		_buf = NULL;
		_length = 0;
		_alloc_length = 0;
		_pa_buf = pa_buf;
		_pa_len = pa_len;
	}
	

	~CBuf(void)
	{
		if (_pa_len == 0)
			FREE(_buf);
	}
	
	byte *Buf()
	{
		return _buf;
	}
	lword Length()
	{
		return _length;
	}
	
	byte operator[](lword i)
	{
		ASSERT(i >= 0 && i < _length);
		return _buf[i];
	}

	void Extend(lword len)
	{
		ASSERT(_buf == NULL);
		_alloc_length += len;
	}
		
	void Allocate()
	{
		ASSERT(_buf == NULL);
		if (_pa_len > 0)
		{
			ASSERT(_pa_len >= _alloc_length);
			_buf = _pa_buf;
		}
		else
			_buf = NALLOC(_alloc_length, byte);
  	}
  	
  	void AddBytes(byte *data, lword len)
  	{
  		ASSERT_ALLOCED(_buf);
  		ASSERT(_length + len <= _alloc_length);
  		memcpy(_buf+_length, data, len);
  		_length += len;
  	}
  	
  	bool Full()
  	{
  		return _buf != NULL && _length == _alloc_length;
  	}

	bool ReadFile(char *file_name)
	{
		// Assert that buffer is still empty
		ASSERT(_alloc_length == 0);

		// Open file
		FILE *f = fopen(file_name, "rb");
		if (f == NULL)
			return FALSE;

		// Determine file size (in length)
		int fh = fileno(f);
		lword length = lseek(fh, 0L, SEEK_END);
		lseek(fh, 0L, SEEK_SET);

		// Allocate memory
		Extend(length);
		Allocate();

		// Read the whole file into buffer
		_length = fread(_buf, 1, _alloc_length, f);

		fclose(f);

		return Full();
	}

};

