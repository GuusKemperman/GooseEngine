export module test_runtime_reflection;

import std;
import logger;
import runtime_reflection;
import test_core;

using namespace ge::test_core;
using namespace ge::test_core::assert;

namespace
{
	struct type_1
	{
		
	};
}

UNIT_TEST( building, basic )
{
	ge::refl::registry reg =
		ge::refl::begin_registry()
			.begin_module()
				.begin_type<type_1>("type_1")
				.end_type()
			.end_module()
		.build();

	std::optional<ge::refl::type> type = reg.try_get_type("type_1");
	is_true(type.has_value());

	is_eq(type->get_name(), "type_1");
	
	const ge::refl::type_info& expectedInfo = ge::refl::type_info::get_type_info<type_1>();
	const ge::refl::type_info& actualInfo = type->get_info();

	is_eq(expectedInfo, actualInfo);
}

//
//UNIT_TST(dependency, circular_impl)
//{
//	refl_data data{};
//	is_eq(data.do_thing(), 5);
//
//	ge::logger l{};
//	l.log_raw(ge::verbose, data.get_name<int>());
//}