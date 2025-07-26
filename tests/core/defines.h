#pragma once

#define UNIT_TEST(category, name)										\
namespace category														\
{																		\
	void name();														\
}																		\
																		\
extern "C"																\
{																		\
	API auto get_unit_test_ ## category ## _ ## name() -> void(&)()		\
	{																	\
		return * category ## :: ## name;								\
	}																	\
}																		\
void category ## :: ## name ()