#pragma once

#include <map>
#include <iostream>

#include "types.h"
#include "NetServiceDelegate.h"

namespace Idea 
{

#ifdef USE_CRYPTOPP_LIB
/*
std::string / buffer_ptr 타입으로 암호화하기
*/

bool encrypt_aes_string(const std::string& key, std::string& iv, const std::string& plain, std::string& ciper );
bool decrypt_aes_string(const std::string& key, std::string& iv, const std::string& cipher, std::string& plain );
/*
	std::string key = "1234567890123456";
	std::string iv = "1234567890123456";
	
	std::string plain = "그냥txt";
	std::string ciper;
	Idea::encrypt_aes_string( key, iv, plain, ciper);
	std::string new_plain;
	Idea::decrypt_aes_string( key, iv, ciper, new_plain);

*/
bool encrypt_aes_buffer(const std::string& key, const std::string& iv, const buffer_ptr_t& plain, buffer_ptr_t cipher);
bool decrypt_aes_buffer(const std::string& key, const std::string& iv, const buffer_ptr_t& cipher, buffer_ptr_t plain);
bool decrypt_aes_buffer(const std::string& key, const std::string& iv, const buffer_ptr_t& cipher, size_t count, buffer_ptr_t plain);

/*
	std::string key = "1234567890123456";
	std::string iv = "1234567890123456";

	buffer_ptr_t plain = boost::make_shared<std::vector<char>>(10);
	for(int i=0;i<plain->size();++i)
	{
		(*plain)[i] = (char)i;
	}

	buffer_ptr_t ciper = boost::make_shared<std::vector<char>>();
	Idea::encrypt_aes_buffer( key, iv, plain, ciper);
	buffer_ptr_t new_plain = boost::make_shared<std::vector<char>>();
	Idea::decrypt_aes_buffer( key, iv, ciper, new_plain);
 */
bool encrypt_arc4_string(const std::string& key, const std::string& plain, std::string& ciper );
bool decrypt_arc4_string(const std::string& key, const std::string& cipher, std::string& plain );
/*
	std::string key = "1234567890123456";
	std::string iv = "1234567890123456";
	
	std::string plain = "그냥txt";
	std::string ciper;

	Idea::encrypt_arc4_string( key, plain, ciper);
	Idea::encrypt_arc4_string( key, ciper, plain);
*/

bool encrypt_arc4_buffer(const std::string& key, const buffer_ptr_t& plain, buffer_ptr_t cipher);
bool decrypt_arc4_buffer(const std::string& key, const buffer_ptr_t& cipher, buffer_ptr_t plain);
bool decrypt_arc4_buffer(const std::string& key, const buffer_ptr_t& cipher, size_t count, buffer_ptr_t plain);
/*
	std::string key = "1234567890123456";
	buffer_ptr_t plain = boost::make_shared<std::vector<char>>(10);
	for(int i=0;i<plain->size();++i)
	{
		(*plain)[i] = (char)i;
	}

	Idea::encrypt_arc4_buffer( key, plain, ciper);
	Idea::encrypt_arc4_buffer( key, ciper, plain);
*/
#endif

extern bool encrypt(const buffer_ptr_t& plain,buffer_ptr_t& cipher);
extern bool decrypt(const buffer_ptr_t& cipher, int length, buffer_ptr_t& plain);


}