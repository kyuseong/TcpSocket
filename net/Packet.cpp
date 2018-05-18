#include "stdafx.h"
#include "packet.h"

//////////////////////////////////////////////////////////////////////////////
// Utility functions

unsigned int CountSetBits(unsigned int nValue)
{
	/*
	unsigned int nResult = 0;
	while (nValue)
	{
		++nResult;
		nValue &= nValue - 1;
	}
	return nResult;
	*/

	// Constant-time operation
	nValue = ((nValue & 0xAAAAAAAA) >> 1) + (nValue & 0x55555555);
	nValue = ((nValue & 0xCCCCCCCC) >> 2) + (nValue & 0x33333333);
	nValue = ((nValue & 0xF0F0F0F0) >> 4) + (nValue & 0x0F0F0F0F);
	nValue = ((nValue & 0xFF00FF00) >> 8) + (nValue & 0x00FF00FF);
	nValue = ((nValue & 0xFFFF0000) >> 16) + (nValue & 0x0000FFFF);

	return nValue;
}

///////////////////////////////////////////////////////////////////////////////
// 시리얼라이즈 데이터 풀
///////////////////////////////////////////////////////////////////////////////

std::recursive_mutex g_cPacket_CS_Trash;
cSerializeData *cSerializeData::s_pTrash_Data = 0;
cSerializeData cSerializeData::s_cTrash_Chunk;


// 정크 하나 할당하기
cSerializeData::SChunk *cSerializeData::Allocate_Chunk(unsigned int nOffset)
{
	g_cPacket_CS_Trash.lock();


	if (s_cTrash_Chunk.m_pFirstChunk)
	{
		SChunk *pResult = s_cTrash_Chunk.m_pFirstChunk;
		s_cTrash_Chunk.m_pFirstChunk = s_cTrash_Chunk.m_pFirstChunk->m_pNext;
		
		g_cPacket_CS_Trash.unlock();

		new(pResult) SChunk(nOffset);
		return pResult;
	}
	else
	{
		g_cPacket_CS_Trash.unlock();
		
		SChunk* pNewChunk;
		pNewChunk = new SChunk(nOffset);
		return pNewChunk;
	}
}

///////////////////////////////////////////////////////////////////////////////
// 정크 하나 해제하기
///////////////////////////////////////////////////////////////////////////////
void cSerializeData::Free_Chunk(SChunk *pChunk)
{
	g_cPacket_CS_Trash.lock();
	
	pChunk->m_pNext = s_cTrash_Chunk.m_pFirstChunk;
	s_cTrash_Chunk.m_pFirstChunk = pChunk;

	g_cPacket_CS_Trash.unlock();	
}

///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
cSerializeData* cSerializeData::Allocate()
{
	// 
	g_cPacket_CS_Trash.lock();
	
	cSerializeData *pResult;
	if (s_pTrash_Data)
	{
		std::lock_guard<std::recursive_mutex> csAuto(s_pTrash_Data->m_CS_Instance);

		pResult = s_pTrash_Data;
		s_pTrash_Data = (cSerializeData*)s_pTrash_Data->m_pFirstChunk;

		// 리셋
		pResult->Reset( );
		pResult->m_bIsOnTrashList = false;
	}
	else
	{
		pResult = new cSerializeData;
	}

	g_cPacket_CS_Trash.unlock();
	
	return pResult;
}

void cSerializeData::AddRef() 
{ 
	std::lock_guard<std::recursive_mutex> csTrashAuto(g_cPacket_CS_Trash);
	std::lock_guard<std::recursive_mutex> csInstanceAuto(m_CS_Instance);
	
	if (m_bIsOnTrashList) 
	{
		ReuseObjectFromTrashList(this); 
	}

	m_nRefCount++; 
}

void cSerializeData::Release() 
{ 
	std::lock_guard<std::recursive_mutex> csTrashAuto(g_cPacket_CS_Trash);
	std::lock_guard<std::recursive_mutex> csInstanceAuto(m_CS_Instance);
	
	m_nRefCount--; 
	
	if (m_nRefCount == 0) 
	{
		const_cast<cSerializeData*>(this)->Free();
	}
}

void cSerializeData::Reset()
{
	std::lock_guard<std::recursive_mutex> autoCS( m_CS_Instance );

	m_pFirstChunk = NULL;
	m_pLastChunk = NULL;
	m_nRefCount = 0;
	m_nSize = 0;
}

void cSerializeData::ReuseObjectFromTrashList(cSerializeData* pPacketData)
{
	std::lock_guard<std::recursive_mutex> csTrash(g_cPacket_CS_Trash);

	cSerializeData* pCurrentPacketData  = s_pTrash_Data;
	cSerializeData* pPreviousPacketData = NULL;

	while (pCurrentPacketData)
	{
		if (pCurrentPacketData == pPacketData)
		{
			// found it
			break;
		}

		pPreviousPacketData = pCurrentPacketData;
		pCurrentPacketData = (cSerializeData*)pCurrentPacketData->m_pFirstChunk;
	}

	if (!pCurrentPacketData)
	{
		// 쓰레기 리스트에서 찾을수 없다.
		return;
	}

	if (pPreviousPacketData)
	{
		pPreviousPacketData->m_pFirstChunk = pPacketData->m_pFirstChunk;
	}
	else
	{
		s_pTrash_Data = (cSerializeData*)pPacketData->m_pFirstChunk;
	}

	pPacketData->m_bIsOnTrashList = false;
	pPacketData->Reset();
}

void cSerializeData::Free()
{
	std::lock_guard<std::recursive_mutex> autoCS( m_CS_Instance );

	while (m_pFirstChunk)
	{
		SChunk *pTemp = m_pFirstChunk;
		m_pFirstChunk = m_pFirstChunk->m_pNext;
		Free_Chunk(pTemp);
	}

	g_cPacket_CS_Trash.lock();
	
	m_pFirstChunk = (SChunk*)s_pTrash_Data;
	s_pTrash_Data = this;

	m_bIsOnTrashList = true;

	g_cPacket_CS_Trash.unlock();
}

//////////////////////////////////////////////////////////////////////////////
// PacketWriter implementation
//////////////////////////////////////////////////////////////////////////////

void PacketWriter::WriteBits(unsigned int nValue, unsigned int nBits)
{
	//assert(nBits <= 32);
	unsigned int nWriteMask = (nBits < 32) ? (1 << nBits) - 1 : -1;
	unsigned int nWriteValue = nValue & nWriteMask;
	m_nBitAccumulator |= nWriteValue << m_nBitsAccumulated;
	m_nBitsAccumulated += nBits;
	if (m_nBitsAccumulated >= 32)
	{
		if (!m_Data)
		{
			m_Data = cSerializeData::Allocate();
		}
		m_Data->Append(m_nBitAccumulator, 32);
		m_nBitsAccumulated -= 32;
		if (m_nBitsAccumulated)
			m_nBitAccumulator = nWriteValue >> (nBits - m_nBitsAccumulated);
		else
			m_nBitAccumulator = 0;
	}
}

void PacketWriter::WriteBits64(unsigned long long nValue, unsigned int nBits)
{
	//ASSERT(nBits <= 64);
	WriteBits((unsigned int)nValue, std::min<unsigned int>(nBits, 32));
	if (nBits > 32)
	{
		nBits -= 32;
		WriteBits((unsigned int)(nValue >> 32), nBits);
	}
}

void PacketWriter::WriteData(const void *pData, unsigned int nBits)
{
	// 32 bit 단위로 잘라서 넣어준다.
	const unsigned int *pData32 = reinterpret_cast<const unsigned int*>(pData);
	while (nBits >= 32)
	{
		WriteBits(*pData32, 32);
		++pData32;
		nBits -= 32;
	}
	// 32 bit 단위로 잘라 넣고 남은 부분은 8bit 단위로 넣어준다.
	if (nBits)
	{
		// 8 bit 단위로 잘라서 넣어준다.

		unsigned int nData8Accumulator = 0;
		unsigned int nWriteMask = (nBits < 32) ? (1 << nBits) - 1 : -1;
		unsigned int nShift = 0;
		const unsigned char *pData8 = reinterpret_cast<const unsigned char*>(pData32);
		while (nWriteMask)
		{
			nData8Accumulator |= (*pData8 & nWriteMask) << nShift;
			nWriteMask >>= 8;
			nShift += 8;
			++pData8;
		}
		WriteBits(nData8Accumulator, nBits);
	}
}

void PacketWriter::WritePacket(const PacketReader &cRead)
{
	unsigned int nBits = cRead.size();
	if (!nBits)
		return;
	
	// 
	if (cRead.m_nStart == 0)
	{
		cSerializeData::iterator iCur = cRead.m_Data->begin();
		for (; nBits >= 32; ++iCur, nBits -= 32)
			WriteBits(*iCur, 32);
		if (nBits)
			WriteBits(*iCur, nBits);
	}
	else
	{
		PacketReader cTempPacket(cRead);
		for (; nBits >= 32; nBits -= 32)
			Writeuint32(cTempPacket.Readuint32());
		if (nBits)
			WriteBits(cTempPacket.ReadBits(nBits), nBits);
	}
}

void PacketWriter::WriteString(const char *pString)
{
	if (!pString)
	{
		Writeint8(0);
		return;
	}
	while (*pString)
	{
		Writeint8(*pString);
		++pString;
	}
	Writeint8(0);
}

void PacketWriter::WriteWString(const wchar_t *pwString)
{
	if (!pwString)
	{
		Writeint16(0);
		return;
	}
	while (*pwString)
	{
		Writeint16(*pwString);
		++pwString;
	}
	Writeint16(0);
}

//////////////////////////////////////////////////////////////////////////////
// PacketReader implementation
//////////////////////////////////////////////////////////////////////////////

unsigned int PacketReader::ReadBits(unsigned int nBits)
{
	//ASSERT(nBits <= 32);
	nBits = std::min(nBits, size() - Tell());
	
	unsigned int nReadMask = (nBits < 32) ? ((1 << nBits) - 1) : -1;
	unsigned int nCurOffset = (m_nOffset + m_nStart) & 31;
	unsigned int nCurLoaded = 32 - nCurOffset;
	unsigned int nResult = (m_nCurData >> nCurOffset) & nReadMask;
	m_nOffset += nBits;
	if (nBits >= nCurLoaded)
	{
		++m_iCurData;
		m_nCurData = *m_iCurData;
		if (nBits > nCurLoaded)
		{
			nReadMask >>= nCurLoaded;
			nResult |= (m_nCurData & nReadMask) << nCurLoaded;
		}
	}
	return nResult;
}

unsigned long long PacketReader::ReadBits64(unsigned int nBits)
{
	//ASSERT(nBits <= 64);
	unsigned long long nResult = (unsigned long long)ReadBits(std::min<unsigned int>(nBits, 32));
	if (nBits > 32)
	{
		nBits -= 32;
		nResult |= (unsigned long long)ReadBits(nBits) << 32;
	}
	return nResult;
}

void PacketReader::ReadData(void *pData, unsigned int nBits)
{
	// 32비트 단위로 카피하기
	unsigned int *pData32 = reinterpret_cast<unsigned int*>(pData);
	while (nBits >= 32)
	{
		*pData32 = ReadBits(32);
		++pData32;
		nBits -= 32;
	}
	
	// 8비트 단이로 카피하기
	if (nBits)
	{
		unsigned char *pData8 = reinterpret_cast<unsigned char*>(pData32);
		while (nBits)
		{
			unsigned int nNumRead = std::min<unsigned int>(8, nBits);
			*pData8 = (unsigned char)ReadBits(nNumRead);
			++pData8;
			nBits -= nNumRead;
		}
	}
}

unsigned int PacketReader::ReadString(char *pDest, unsigned int nMaxLen)
{
	unsigned int nResult = 0;
	char nNextChar;
	char *pEnd = (pDest) ? (pDest + nMaxLen) : 0;
	do
	{
		nNextChar = Readint8();
		++nResult;
		if (pDest != pEnd)
		{
			*pDest = nNextChar;
			++pDest;
		}
	} while (nNextChar != 0);
	return nResult - 1;
}

unsigned int PacketReader::ReadWString(wchar_t *pwDest, unsigned int nMaxLen)
{
	unsigned int nResult = 0;
	wchar_t nNextChar;
	wchar_t *pEnd = (pwDest) ? (pwDest + nMaxLen) : 0;
	do
	{
		nNextChar = Readint16();
		++nResult;
		if (pwDest != pEnd)
		{
			*pwDest = nNextChar;
			++pwDest;
		}
	} while (nNextChar != 0);
	return nResult - 1;
}

class PacketCRC32Table
{
public:
	PacketCRC32Table();
	enum { k_Polynomial = 0xedb88320L };
	enum { k_CRCTableSize = 256 };
	unsigned int m_Table[k_CRCTableSize];
	inline void Calc(unsigned int &nCurCRC, unsigned char nData) { 
		nCurCRC = m_Table[(nCurCRC ^ nData) & 0xFF] ^ (nCurCRC >> 8);
	}
};

PacketCRC32Table::PacketCRC32Table()
{
	for (unsigned int nCurEntry = 0; nCurEntry < k_CRCTableSize; ++nCurEntry)
	{
		unsigned int nAccumulator = nCurEntry;
		for (unsigned int nModEntry = 0; nModEntry < 8; ++nModEntry)
		{
			if (nAccumulator & 1)
				nAccumulator = k_Polynomial ^ (nAccumulator >> 1);
			else
				nAccumulator = nAccumulator >> 1;
		}
		m_Table[nCurEntry] = nAccumulator;
	}
}

static PacketCRC32Table g_CRCTable;

unsigned int PacketReader::CalcChecksum() const
{
	PacketReader cChecksumPacket(*this);
	cChecksumPacket.SeekTo(0);
	unsigned int nResult = 0xFFFFFFFF;
	while (!cChecksumPacket.EOP())
	{
		g_CRCTable.Calc(nResult, cChecksumPacket.Readuint8());
	}
	return nResult ^ 0xFFFFFFFF;

}

