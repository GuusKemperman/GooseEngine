#include <cassert>
import engine;
import file_io;
import logger;
import dependency_injection;

struct dependency1
{

};

struct dependency2 : public ge::depends_on<dependency1>
{

};

struct dependency3
{

};

class depend_test : public ge::depends_on<dependency2, dependency3>
{
	using depends_on_constructor::depends_on_constructor;
};

int main()
{
	dependency1 d1{};
	dependency2 d2{ d1 };
	dependency3 d3{};

	depend_test t{ d2, d3 };

	//assert(&t.get<dependency1>() == &d1);
	assert(&t.get<dependency2>() == &d2);

	t.get<dependency3>();

	//ge::engine engine{ std::make_shared<ge::file_io>(), std::make_shared<test_logger>() };
	//engine.run();
}

