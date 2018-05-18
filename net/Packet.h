#pragma once

#include <mutex>
#include <algorithm>

class PacketReader;
class PacketWriter;
class cSerializeData;

// min~max 자름
inline int Clamp(int value, int min, int max)
{
	if (value < min)
	{
		value = min;
	}
	else if (value > max)
	{
		value = max;
	}
	return value;
}

// ref count
template <class T>
class cReference
{
public:
	inline cReference() : m_pPtr(0) {}
	inline cReference(const cReference &cOther) : m_pPtr(cOther.m_pPtr) {
		if (m_pPtr)
			m_pPtr->AddRef();
	}
	inline cReference(T *pPtr) : m_pPtr(pPtr) {
		if (m_pPtr)
			m_pPtr->AddRef();
	}
	inline ~cReference() {
		if (m_pPtr)
			m_pPtr->Release();
	}
	inline cReference &operator=(const cReference &cOther) { return *this = (T*)cOther; }
	inline cReference &operator=(T *pPtr) {
		if (m_pPtr == pPtr)
			return *this;
		if (m_pPtr)
			m_pPtr->Release();
		m_pPtr = pPtr;
		if (m_pPtr)
			m_pPtr->AddRef();
		return *this;
	}
	inline bool operator==(const T *pPtr) const {
		return m_pPtr == pPtr;
	}
	inline bool operator!=(const T *pPtr) const {
		return m_pPtr != pPtr;
	}
	inline T &operator*() const { return *m_pPtr; }
	inline T *operator->() const { return m_pPtr; }
	inline operator T*() const { return m_pPtr; }
private:
	T *m_pPtr;
};

template <unsigned int NUMBER>
struct FNumBitsInclusive
{
	enum { Count = (FNumBitsInclusive<NUMBER / 2>::Count + 1) };
};

template<>
struct FNumBitsInclusive<0>
{
	enum { Count = 0 };
};

template<>
struct FNumBitsInclusive<1>
{
	enum { Count = 1 };
};

template <unsigned int NUMBER>
struct FNumBitsExclusive
{
	enum { Count = (FNumBitsInclusive<NUMBER - 1>::Count) };
};

template<>
struct FNumBitsExclusive<0>
{
	enum { Count = 0 };
};

// 실제 시리얼라이즈 된 데이터
class cSerializeData
{
	struct SChunk
	{
		enum {
			CHUNK_HEAD = sizeof(unsigned int) + sizeof(SChunk*),
			CHUNK_SIZE = 256,
			CHUNK_CAPACITY = (CHUNK_SIZE - CHUNK_HEAD) / sizeof(unsigned int),
			CHUNK_BIT_CAPACITY = CHUNK_CAPACITY * 32
		};

		unsigned int m_aData[CHUNK_CAPACITY];
		unsigned int m_nOffset;
		unsigned int m_nInUse;
		SChunk *m_pNext;

		inline SChunk(unsigned int nOffset = 0) :
			m_nOffset(nOffset),
			m_nInUse(0),
			m_pNext(0)
		{}
		inline SChunk *Append(unsigned int nValue) {
			m_aData[m_nInUse] = nValue;
			++m_nInUse;
			if (m_nInUse == CHUNK_CAPACITY)
			{
				m_pNext = cSerializeData::Allocate_Chunk(m_nOffset + m_nInUse);
				return m_pNext;
			}
			else
				return this;
		}
	};
public:
	class iterator
	{
	public:
		typedef iterator my_t;
		typedef unsigned int ptr_type;
		typedef SChunk value_type;
		typedef SChunk& reference;
		typedef SChunk* pointer;
		typedef std::forward_iterator_tag iterator_category;
		typedef int difference_type;
		iterator() {}
		iterator(SChunk* cont, ptr_type ptr) : cont_(cont), ptr_(ptr) { }
		my_t operator++() { 
			if (!cont_)
				return *this;
			++ptr_;
			if (ptr_ >= cont_->m_nInUse)
			{
				cont_ = cont_->m_pNext;
				ptr_ = 0;
			}
			return *this;
		}
		inline my_t operator+(unsigned int nOffset) {
			my_t sResult(*this);
			sResult.ptr_ += nOffset;
			while (sResult.cont_ && (sResult.ptr_ >= sResult.cont_->m_nInUse))
			{
				sResult.ptr_ -= sResult.cont_->m_nInUse;
				sResult.cont_ = sResult.cont_->m_pNext;
			}
			if (!sResult.cont_)
				sResult.ptr_ = 0;
			return sResult;
		}
		unsigned int operator*() {
			if (!cont_)
				return 0;
			return cont_->m_aData[ptr_];
		}
		inline bool operator==(const my_t &sOther) {
			return (sOther.cont_ == cont_) && (sOther.ptr_ == ptr_);
		}
		inline bool operator!=(const my_t &sOther) {
			return (sOther.cont_ != cont_) || (sOther.ptr_ != ptr_);
		}
	private:
		SChunk* cont_;
		ptr_type ptr_;
	};

	class const_iterator
	{
	public:
		typedef const_iterator my_t;
		typedef unsigned int ptr_type;
		typedef SChunk value_type;
		typedef SChunk& reference;
		typedef SChunk* pointer;
		typedef int difference_type;
		typedef std::forward_iterator_tag iterator_category;
	private:
		SChunk* cont_;
		ptr_type ptr_;

	public:
		const_iterator() {}
		const_iterator(SChunk* cont, ptr_type ptr) : cont_(cont), ptr_(ptr) { }
		my_t operator++() 
		{
			if (!cont_)
				return *this;
			++ptr_;
			if (ptr_ >= cont_->m_nInUse)
			{
				cont_ = cont_->m_pNext;
				ptr_ = 0;
			}
			return *this;
		}
		inline my_t operator+(unsigned int nOffset) {
			my_t sResult(*this);
			sResult.ptr_ += nOffset;
			while (sResult.cont_ && (sResult.ptr_ >= sResult.cont_->m_nInUse))
			{
				sResult.ptr_ -= sResult.cont_->m_nInUse;
				sResult.cont_ = sResult.cont_->m_pNext;
			}
			if (!sResult.cont_)
				sResult.ptr_ = 0;
			return sResult;
		}
		unsigned int operator*() {
			if (!cont_)
				return 0;
			return cont_->m_aData[ptr_];
		}
		inline bool operator==(const my_t &sOther) {
			return (sOther.cont_ == cont_) && (sOther.ptr_ == ptr_);
		}
		inline bool operator!=(const my_t &sOther) {
			return (sOther.cont_ != cont_) || (sOther.ptr_ != ptr_);
		}
	};

private:
	struct SChunk;
	friend struct SChunk;

	SChunk *m_pFirstChunk, *m_pLastChunk;

	mutable unsigned int m_nRefCount;
	unsigned int m_nSize;
	std::recursive_mutex m_CS_Instance;

	bool m_bIsOnTrashList;

	static SChunk *Allocate_Chunk(unsigned int nOffset = 0);
	
	static void Free_Chunk(SChunk *pChunk);
	
	static cSerializeData s_cTrash_Chunk;
	
	static cSerializeData *s_pTrash_Data;
public:
	inline cSerializeData() : 
		m_pFirstChunk(0),
		m_pLastChunk(0),
		m_nRefCount(0),
		m_nSize(0),
		m_bIsOnTrashList(false)
	{}

	static cSerializeData *Allocate();

	void AddRef();
	void Release();
	inline unsigned int GetRefCount() const { return m_nRefCount; }

	inline bool Append(unsigned int nData, unsigned int nBits) 
	{
		std::lock_guard<std::recursive_mutex> csAuto(m_CS_Instance);

		m_nSize += nBits;
		if (!m_pFirstChunk)
		{
			m_pFirstChunk = Allocate_Chunk();
			m_pLastChunk = m_pFirstChunk;
		}
		m_pLastChunk = m_pLastChunk->Append(nData);
		return true;
	}

	inline iterator begin() { return iterator(m_pFirstChunk, 0); }
	inline iterator end() { return iterator(0, 0); }
	inline const_iterator begin() const { return const_iterator(m_pFirstChunk, 0); }
	inline const_iterator end() const { return const_iterator(0, 0); }

	inline unsigned int size() const { return m_nSize; }
	inline bool empty() const { return m_nSize == 0; }
private:
	void Reset();
	void ReuseObjectFromTrashList(cSerializeData* pPacketData);
	void Free();
};
typedef cReference<cSerializeData> cSerializeDataRef;

///////////////////////////////////////////////////////////////////////////////
// 메모리 쓰기
///////////////////////////////////////////////////////////////////////////////
class PacketWriter
{
	friend class PacketReader;

	cSerializeDataRef m_Data;

	// bit로 쓰기위한 버퍼

	unsigned int m_nBitAccumulator;
	unsigned int m_nBitsAccumulated;

public:
	PacketWriter() : m_Data(0)
	{
		m_nBitAccumulator = 0;
		m_nBitsAccumulated = 0;
	}
	~PacketWriter()
	{
	}

	inline void Reset() { m_Data = 0; }

	// 사이즈(얼마나 썼나?)
	inline unsigned int size() const { return ((m_Data) ? (m_Data->size()) : 0) + m_nBitsAccumulated; }
	inline bool empty() const { return size() == 0; }

	// 데이터 쓰기
	void WriteBits(unsigned int nValue, unsigned int nBits);
	void WriteBits64(unsigned long long nValue, unsigned int nBits);
	void WriteData(const void *pData, unsigned int nBits);
	void WritePacket(const PacketReader &cRead);

	template <class T>
	void Write(const T &tValue) { 
		switch (sizeof(T))
		{
			case 1 : Writeuint8(reinterpret_cast<const unsigned char &>(tValue)); break;
			case 2 : Writeuint16(reinterpret_cast<const unsigned short &>(tValue)); break; 
			case 4 : Writeuint32(reinterpret_cast<const unsigned int &>(tValue)); break;
			case 8 : Writeuint64(reinterpret_cast<const unsigned long long &>(tValue)); break;
			default : WriteData(&tValue, sizeof(T) * 8); break;
		}
	}
	void Writebool(bool bValue) { WriteBits(bValue ? 1 : 0, 1); }
	void Writeuint8(unsigned char nValue) { WriteBits(nValue, 8); }
	void Writeuint16(unsigned short nValue) { WriteBits(nValue, 16); }
	void Writeuint32(unsigned int nValue) { WriteBits(nValue, 32); }
	void Writeuint64(unsigned long long nValue) { WriteBits64(nValue, 64); }
	void Writeint8(char nValue) { WriteBits((unsigned int)nValue, 8); }
	void Writeint16(short nValue) { WriteBits((unsigned int)nValue, 16); }
	void Writeint32(int nValue) { WriteBits((unsigned int)nValue, 32); }
	void Writeint64(unsigned long long nValue) { WriteBits64(nValue, 64); }
	void Writefloat(float fValue) { WriteBits(reinterpret_cast<const unsigned int&>(fValue), 32); }
	void Writedouble(double fValue) { WriteBits64(reinterpret_cast<const unsigned long long&>(fValue), 64); }
	void WriteString(const char *pString);
	void WriteWString(const wchar_t *pwString);
private:
	// 데이터로 Flush
	void Flush() 
	{ 
		if (!m_nBitsAccumulated)
			return;
		if (!m_Data)
		{
			m_Data = cSerializeData::Allocate();
		}
		m_Data->Append(m_nBitAccumulator, m_nBitsAccumulated);
		m_nBitsAccumulated = 0;
		m_nBitAccumulator = 0;
	}
};

///////////////////////////////////////////////////////////////////////////////
// 메모리 읽기
///////////////////////////////////////////////////////////////////////////////
class PacketReader
{
	friend class PacketWriter;
	// 실제 데이터
	cSerializeDataRef m_Data;
	// 시작위치
	unsigned int m_nStart;
	// 사이즈
	unsigned int m_nSize;
	// 현재 오프셋
	unsigned int m_nOffset;
	// 
	cSerializeData::iterator m_iCurData;
	// 
	unsigned int m_nCurData;

public:
	// 쓰여진걸 읽기 위해서는 Reader로 
	inline explicit PacketReader(PacketWriter &cOther) :
		m_nStart(0)
	{
		cOther.Flush();
		m_Data = cOther.m_Data;
		if (m_Data)
		{
			m_nSize = m_Data->size();
		}
		else
			m_nSize = 0;
		cOther.Reset();
		SeekTo(0);
	}

	inline PacketReader(const PacketReader &cOther)
	{
		m_Data = cOther.m_Data;
		m_nStart = cOther.m_nStart;
		m_nSize = cOther.m_nSize;
		SeekTo(cOther.m_nOffset);
	}
	inline PacketReader(const PacketReader &cOther, unsigned int nStart) 
	{
		new(this) PacketReader(cOther, nStart, cOther.size());
	}

	inline PacketReader() 
	{
		PacketWriter cPacketWrite;
		new(this) PacketReader(cPacketWrite);
	}
	
	inline PacketReader(const PacketReader &cOther, unsigned int nStart, unsigned int nSize) :
		m_Data(cOther.m_Data),
		m_nStart(cOther.m_nStart + nStart),
		m_nSize(nSize)
	{
		if (m_nStart > (cOther.m_nStart + cOther.m_nSize))
		{
			m_nStart = (cOther.m_nStart + cOther.m_nSize);
			m_nSize = 0;
		}
		m_nSize = std::min<unsigned int>(m_nSize, cOther.m_nSize - (m_nStart - cOther.m_nStart));

		if ((cOther.m_nOffset + cOther.m_nStart) > m_nStart)
			SeekTo((cOther.m_nOffset + cOther.m_nStart) - m_nStart);
		else
			SeekTo(0);
	}
	inline ~PacketReader()
	{
	}
	inline PacketReader &operator=(const PacketReader &cOther) 
	{
		if (this == &cOther)
			return *this;
		m_Data = cOther.m_Data;
		m_nStart = cOther.m_nStart;
		m_nSize = cOther.m_nSize;
		SeekTo(cOther.m_nOffset);
		return *this;
	}

	inline void clear() { *this = PacketReader(); }
	inline unsigned int size() const { return m_nSize; }
	inline unsigned int ByteSize() const { 
		if (m_nSize == 0)
			return 0;
		return m_nSize / 8 + 1;
	}
	inline bool empty() const { return m_nSize == 0; }
	inline void seek(int nOffset) { 
		m_nOffset = Clamp((int)m_nOffset + nOffset, 0, (int)size());
		RefreshIterator();
	}
	inline void SeekTo(unsigned int nPos) { 
		m_nOffset = std::min(size(), nPos);
		RefreshIterator();
	}
	inline unsigned int Tell() const { return m_nOffset; }
	inline unsigned int TellEnd() const { return size() - Tell(); }
	inline bool EOP() const { return m_nOffset >= size(); }
	unsigned int ReadBits(unsigned int nBits);
	unsigned long long ReadBits64(unsigned int nBits);
	void ReadData(void *pData, unsigned int nBits);
	template <class T>
	void ReadType(T *pValue) { 
		switch (sizeof(T))
		{
			case 1 : { unsigned char nTemp = Readuint8(); *pValue = reinterpret_cast<const T &>(nTemp); break; }
			case 2 : { unsigned short nTemp = Readuint16(); *pValue = reinterpret_cast<const T &>(nTemp); break; }
			case 4 : { unsigned int nTemp = Readuint32(); *pValue = reinterpret_cast<const T &>(nTemp); break; }
			case 8 : { unsigned long long nTemp = Readuint64(); *pValue = reinterpret_cast<const T &>(nTemp); break; }
			default : { ReadData(pValue, sizeof(T) * 8); break; }
		}
	}
	inline bool Readbool() { return ReadBits(1) != 0; }
	inline unsigned char Readuint8() { return (unsigned char)ReadBits(8); }
	inline unsigned short Readuint16() { return (unsigned short)ReadBits(16); }
	inline unsigned int Readuint32() { return (unsigned int)ReadBits(32); }
	inline unsigned long long Readuint64() { return (unsigned long long)ReadBits64(64); }
	inline char Readint8() { return (char)ReadBits(8); }
	inline short Readint16() { return (short)ReadBits(16); }
	inline int Readint32() { return (int)ReadBits(32); }
	inline long long Readint64() { return (long long)ReadBits64(64); }
	inline float Readfloat() { unsigned int nTemp = Readuint32(); return reinterpret_cast<const float&>(nTemp); }
	inline double Readdouble() { unsigned long long nTemp = Readuint64(); return reinterpret_cast<const double&>(nTemp); }
	unsigned int ReadString(char *pDest, unsigned int nMaxLen);
	unsigned int ReadWString(wchar_t *pwDest, unsigned int nMaxLen);
	inline unsigned int PeekBits(unsigned int nBits) const { return PacketReader(*this).ReadBits(nBits); }
	inline unsigned long long PeekBits64(unsigned int nBits) const { return PacketReader(*this).ReadBits64(nBits); }
	inline void PeekData(void *pData, unsigned int nBits) const { PacketReader(*this).ReadData(pData, nBits); }
	template <class T>
	void PeekType(T *pValue) const { PacketReader(*this).ReadType(pValue); }
	inline bool Peekbool() const { return PacketReader(*this).Readbool(); }
	inline unsigned char Peekuint8() const { return PacketReader(*this).Readuint8(); }
	inline unsigned short Peekuint16() const { return PacketReader(*this).Readuint16(); }
	inline unsigned int Peekuint32() const { return PacketReader(*this).Readuint32(); }
	inline unsigned long long Peekuint64() const { return PacketReader(*this).Readuint64(); }
	inline char Peekint8() const { return PacketReader(*this).Readint8(); }
	inline short Peekint16() const { return PacketReader(*this).Readint16(); }
	inline int Peekint32() const { return PacketReader(*this).Readint32(); }
	inline long long Peekint64() const { return PacketReader(*this).Readint64(); }
	inline float Peekfloat() const { return PacketReader(*this).Readfloat(); }
	inline double Peekdouble() const { return PacketReader(*this).Readdouble(); }
	inline unsigned int PeekString(char *pDest, unsigned int nMaxLen) const { return PacketReader(*this).ReadString(pDest, nMaxLen); }
	inline unsigned int PeekWString(wchar_t *pwDest, unsigned int nMaxLen) const { return PacketReader(*this).ReadWString(pwDest, nMaxLen); }
	unsigned int CalcChecksum() const;
private:
	inline void RefreshIterator() {
		if (EOP())
		{
			m_nCurData = 0;
			return;
		}
		m_iCurData = m_Data->begin() + ((m_nOffset + m_nStart) / 32); 
		m_nCurData = *m_iCurData;
	}
	
};

