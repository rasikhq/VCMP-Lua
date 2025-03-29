#pragma once
#include "pch.h"

// Fix build errors
#include <cstdint>
#include <array>
#include <string>

#include "modules/crypto/include/digestpp.hpp"

class Hash
{
public:
	static void Init(sol::state*);
private:
};
