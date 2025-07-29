#pragma once

import test_core;

#define UNIT_TEST(category, name)															\
namespace category																			\
{																							\
	void name(const ge::test_core::context& a_context);										\
}																							\
																							\
extern "C"																					\
{																							\
	API auto get_unit_test_ ## category ## _ ## name()										\
	{																						\
		return * category ## :: ## name;													\
	}																						\
}																							\
void category ## :: ## name ([[maybe_unused]] const ge::test_core::context& a_context)