#pragma once

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

typedef	signed char			int8;
typedef	unsigned char		uint8;
typedef	signed short		int16;
typedef	unsigned short		uint16;
typedef	signed int			int32;
typedef	unsigned int		uint32;
typedef	long long			int64;
typedef	unsigned long long	uint64;

typedef	float				float32;
typedef	double				float64;

typedef std::shared_ptr<std::vector<char>> buffer_ptr_t;
