#include "stdafx.h"
#include "Crypt.h"


#include "Logger.h"


#ifdef USE_CRYPTOPP_LIB

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include <aes.h>
#include <hex.h>
#include <filters.h>
#include <ccm.h>
#include <sha.h>
#include <hmac.h>
#include <arc4.h>
#include <base64.h>

#endif

namespace Idea {

#ifdef USE_CRYPTOPP_LIB

bool encrypt_aes_string(const std::string& key, std::string& iv, const std::string& plain, std::string& ciper )
{
	ciper.clear();
	try
	{
		CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
		encryption.SetKeyWithIV((const byte*)key.c_str(), key.length(), (const byte*)iv.c_str());
		CryptoPP::StringSource s(plain, true, new CryptoPP::StreamTransformationFilter(encryption, new CryptoPP::StringSink(ciper)));
	}
	catch (const CryptoPP::Exception& )
	{
		return false;
	}
	return true;
}

bool decrypt_aes_string(const std::string& key, std::string& iv, const std::string& cipher, std::string& plain )
{
	plain.clear();
	try
	{
		CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption;
		decryption.SetKeyWithIV((const byte*)key.c_str(), key.length(), (const byte*)iv.c_str());
		CryptoPP::StringSource s(cipher, true, new CryptoPP::StreamTransformationFilter(decryption, new CryptoPP::StringSink(plain)));
	}
	catch (const CryptoPP::Exception& )
	{
		return false;
	}
	return true;
}


bool encrypt_aes_buffer(const std::string& key, const std::string& iv, const buffer_ptr_t& plain, buffer_ptr_t cipher)
{
	try
	{
		CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption;
		encryption.SetKeyWithIV((const byte*)key.c_str(), key.length(), (const byte*)iv.c_str());
		
		CryptoPP::StreamTransformationFilter encryptor(encryption, NULL);

		for(size_t j = 0; j < plain->size(); j++)
			encryptor.Put((byte)plain->at(j));

		encryptor.MessageEnd();
		size_t ready = encryptor.MaxRetrievable();

		cipher->resize(ready);
		// cipher = boost::make_shared<std::vector<char>>(ready);
		encryptor.Get((byte*) &cipher->front(), cipher->size());
	}
	catch (const CryptoPP::Exception& )
	{
		return false;
	}
	return true;
}

bool decrypt_aes_buffer(const std::string& key, const std::string& iv, const buffer_ptr_t& cipher, buffer_ptr_t plain)
{
	return decrypt_aes_buffer(key, iv, cipher, cipher->size(), plain);
}

bool decrypt_aes_buffer(const std::string& key, const std::string& iv, const buffer_ptr_t& cipher, size_t count, buffer_ptr_t plain)
{
	try
	{
		CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption;
		decryption.SetKeyWithIV((const byte*)key.c_str(), key.length(), (const byte*)iv.c_str());
		
		CryptoPP::StreamTransformationFilter decryptor(decryption, NULL);

		for(size_t j = 0; j < count; j++)
			decryptor.Put((byte)cipher->at(j));

		decryptor.MessageEnd();
		size_t ready = decryptor.MaxRetrievable();

		plain->resize(ready);
		// plain = boost::make_shared<std::vector<char>>(ready);
		decryptor.Get((byte*) &plain->front(), plain->size());
	}
	catch (const CryptoPP::Exception& )
	{
		return false;
	}
	return true;
}

bool encrypt_arc4_string(const std::string& key, const std::string& plain, std::string& ciper )
{
	size_t length = plain.length();
	try
	{
		CryptoPP::Weak1::ARC4 arc4((byte *) key.c_str(), key.length());
		ciper.resize(length);
		arc4.ProcessData( (byte *) ciper.data(), (byte *) plain.c_str(), length);
	}
	catch (const CryptoPP::Exception& )
	{
		return false;
	}
	return true;
}

bool decrypt_arc4_string(const std::string& key, const std::string& cipher, std::string& plain )
{
	return encrypt_arc4_string( key, cipher, plain);
}

bool encrypt_arc4_buffer(const std::string& key, const buffer_ptr_t& plain, size_t count, buffer_ptr_t cipher)
{
	size_t length = count;// plain->size();
	try
	{
		CryptoPP::Weak1::ARC4 arc4((byte *) key.c_str(), key.length());
		cipher->resize(length);
		arc4.ProcessData((byte *) cipher->data(), (byte *) &plain->front(), length);
	}
	catch (const CryptoPP::Exception& )
	{
		return false;
	}
	return true;
}

bool encrypt_arc4_buffer(const std::string& key, const buffer_ptr_t& plain, buffer_ptr_t cipher)
{
	return encrypt_arc4_buffer( key, plain, plain->size(), cipher );
}

bool decrypt_arc4_buffer(const std::string& key, const buffer_ptr_t& cipher, buffer_ptr_t plain)
{
	return encrypt_arc4_buffer( key, cipher, cipher->size(), plain);
}

bool decrypt_arc4_buffer(const std::string& key, const buffer_ptr_t& cipher, size_t count, buffer_ptr_t plain)
{
	return encrypt_arc4_buffer(key, cipher, count, plain);
}

#endif

bool encrypt(const buffer_ptr_t& plain, buffer_ptr_t& cipher)
{
	cipher = std::make_shared<std::vector<char>>();

#ifdef USE_CRYPTOPP_LIB
	if (crypt_type_aes == config.crypt())
	{
		return encrypt_aes_buffer(config.crypt_key(), config.crypt_iv(), plain, cipher);
	}
	else if (crypt_type_arc4 == config.crypt())
	{
		return decrypt_arc4_buffer(config.crypt_key(), cipher, cipher);
	}
#endif

	cipher->assign( plain->begin(), plain->end());	
	return true;
}


bool decrypt( const buffer_ptr_t& cipher, int length, buffer_ptr_t& plain)
{
	plain = std::make_shared<std::vector<char>>();
#ifdef USE_CRYPTOPP_LIB
	if (crypt_type_aes == config.crypt())
	{
		return decrypt_aes_buffer(config.crypt_key(), config.crypt_iv(), cipher, length, plain);
	}
	else if (crypt_type_arc4 == config.crypt())
	{
		return decrypt_arc4_buffer(config.crypt_key(), cipher, length, plain);
	}
#endif
	plain->assign( cipher->begin(), cipher->begin() + length);
	
	return true;
}

}