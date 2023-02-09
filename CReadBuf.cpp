bool small_endian = FALSE;

#define _ASSERT(C,X,V) if(_error)return V;if(!(C)){if(echo){printf("\nERROR: '%s'.", _name);printf X;}_error=TRUE;return V;} 

class CReadBuf
{

  public:
	CReadBuf(CBuf &buf, const char *name)
	{
		_buffer = buf.Buf();
		_length = buf.Length();
		_name = name;
		_error = FALSE;
		pos = 0L;
	}
	CReadBuf(byte *buffer, lword length, const char *name)
	{
		_buffer = buffer;
		_length = length;
		_name = name;
		_error = FALSE;
		pos = 0L;
	}
	CReadBuf(CReadBuf &read_buf, lword start, lword length, const char *name)
	{
		_buffer = read_buf._buffer + start;
		_length = length;
		_name = name;
		_error = FALSE;
		pos = 0L;
	}

	inline bool Error()
	{	return _error;
	}
	inline void Reset()
	{	_error = FALSE;
	}

	inline lword Length() 
	{	return _length;
	}
	
	// Reading data from a specified position
	bool EOB(lword i)
	{	return i >= _length;
	}
		
	inline byte GetByte(lword i)
	{
		_ASSERT(i < _length, ("GetByte(%ld) past end %ld", i, _length), 0)
		return _buffer[i];
	}
	inline byte *GetBytes(lword pos, lword len)
	{
		ASSERT_ALLOCED(_buffer);
		_ASSERT(pos + len <= _length, ("GetBytes(%ld,%ld) past end %ld", pos, len, _length), 0);
		return _buffer + pos;
	}
	inline byte operator[](lword i)
	{
		_ASSERT(i < _length, ("[%ld] past end %ld", i, _length), 0);
		return _buffer[i];
	}
	inline word GetWord(lword i)
	{
		_ASSERT(i+1 < _length, ("GetWord(%ld) past end %ld", i, _length), 0);
		if (small_endian)
	  		return    ((word)_buffer[i])
					| ((word)_buffer[i+1] << 8);
		else
	  		return    ((word)_buffer[i] << 8) 
					| ((word)_buffer[i+1]);
	}
	inline lword GetLWord(lword i)
	{
		_ASSERT(i+3 < _length, ("GetLWord(%ld) past end %ld", i, _length), 0);
		if (small_endian)
			return    ((lword)_buffer[i])
					| ((lword)_buffer[i+1] << 8)
					| ((lword)_buffer[i+2] << 16) 
					| ((lword)_buffer[i+3] << 24);
		else
			return    ((lword)_buffer[i] << 24) 
					| ((lword)_buffer[i+1] << 16)
					| ((lword)_buffer[i+2] << 8) 
					| ((lword)_buffer[i+3]);
	}

	// Reading using the internal position
	lword pos;

	inline bool EOB() { return pos >= _length; }
	inline byte GetByte() { return _buffer[pos]; }
	inline word GetWord() { return GetWord(pos); }
	inline lword GetLWord() { return GetLWord(pos); }

	inline void SkipBytes(lword len)
	{
		_ASSERT(pos + len <= _length, ("SkipBytes(%ld) past end %ld, %ld", len, pos, _length), /*void*/)
		pos += len;
	}
	inline byte ReadByte() 
	{ 
		_ASSERT(pos < _length, ("ReadByte(%ld) past end %ld", pos, _length), 0);
		return _buffer[pos++]; 
	}
	inline word ReadWord()	 
	{
		_ASSERT(pos + 1 < _length, ("ReadWord(%ld) past end %ld", pos, _length), 0);
		word value = GetWord();
		pos += 2;
		return value;
	}
	inline lword ReadLWord()	 
	{
		_ASSERT(pos + 3 < _length, ("ReadLWord(%ld) past end %ld", pos, _length), 0);
		lword value = GetLWord();
		pos += 4;
		return value;
	}
	char *ReadPascalString()
	{
		_ASSERT(pos < _length, ("ReadPascalString(%ld) past end %ld", pos, _length), "");
		byte len = _buffer[pos++];
		_ASSERT(pos + len <= _length, ("ReadPascalString(%ld) len %d past end %ld", pos, len, _length), "");
		char *str = NALLOC(len+1, char);
		for (word i = 0; i < len; i++)
			str[i] = _buffer[pos++];
		str[len] = '\0';
		return str;
	}
	char *ReadPascal2String()
	{
		_ASSERT(pos < _length, ("ReadPascal2String(%ld) past end %ld", pos, _length), "");
		byte len = _buffer[pos++];
		byte len2 = len + 1-(len % 2);
		_ASSERT(pos + len2 <= _length, ("ReadPascal2String(%ld) len2 %d past end %ld", pos, len2, _length), "");
		char *str = NALLOC(len+1, char);
		for (word i = 0; i < len; i++)
			str[i] = _buffer[pos++];
		str[len] = '\0';
		if (len2 > len)
			pos++;
		return str;
	}
	char *ReadPascalFixedString(lword fix_len)
	{
		_ASSERT(pos + fix_len <= _length, ("ReadPascalFixedString(%ld+%ld) past end %ld", pos, fix_len, _length), "");
		word len = _buffer[pos++];
		_ASSERT((word)(len + 1) <= fix_len, ("ReadPascalFixedString(%ld) len %d longer than fixed len %ld", pos, len, fix_len), "");
		char *str = NALLOC(len+1, char);
		for (word i = 0; i < len; i++)
			str[i] = _buffer[pos++];
		str[len] = '\0';
		pos += fix_len - len - 1;
		return str;
	}
	
	void Done()
	{
		_ASSERT(pos == _length, ("Done() %ld != %ld", pos, _length), /*void*/);
	}

  private:
	byte *_buffer;
	lword _length;
  protected:
	const char *_name;
	bool _error;
};

class CReadBufWithBlocks : public CReadBuf
{
private:
	lword _nr_blocks;
	bool *_blocks_read;
	
public:
	CReadBufWithBlocks(CBuf &buf, const char *name) : CReadBuf(buf, name)
	{
		_nr_blocks = Length() / 256;
		_blocks_read = NALLOC(_nr_blocks, bool);
		for (lword blocknr = 0; blocknr < _nr_blocks; blocknr++)
			_blocks_read[blocknr] = FALSE;
	}

	~CReadBufWithBlocks(void)
	{
		FREE(_blocks_read);		
	}
	
	lword NextFreeBlock() /* 1-based */
	{
		lword blocknr;
		for (blocknr = 0; blocknr < _nr_blocks; blocknr++)
			if (!_blocks_read[blocknr])
				return blocknr + 1;
		return 0;
	}
	
	bool IsFree(lword blocknr /* 1-based */)
	{
		return blocknr > 0 && blocknr <= _nr_blocks && !_blocks_read[blocknr-1];
	}

	bool IsFree(lword blocknr /* 1-based */, lword len)
	{
		ASSERT(len > 0);
		if (blocknr < 1 || blocknr + (len - 1) > _nr_blocks)
			return FALSE;
		
		lword i;
		for (i = 0, blocknr--; i < len; i++, blocknr++)
		{	if (_blocks_read[blocknr])
				return FALSE;
		}
		
		return TRUE;
	}

	bool ClaimBlocks(lword blocknr /* 1-based */, lword len)
	{
		ASSERT(len > 0);
		if (blocknr < 1 || blocknr + (len - 1) > _nr_blocks)
			return FALSE;
		
		lword i;
		for (i = 0, blocknr--; i < len; i++, blocknr++)
		{	if (_blocks_read[blocknr])
				return FALSE;
			if (echo_br) printf("\nClaimBlock %ld ", blocknr+1);
			_blocks_read[blocknr] = TRUE;
		}
		
		return TRUE;
	}
};

#undef _ASSERT
