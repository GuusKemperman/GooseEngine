import engine;
import file_io;
import logger;

class test_logger : public ge::ilogger
{
	void println(std::string_view ) override
	{
		std::puts("Haha we are not printing SHIT!\n");
	}
};

int main()
{
	ge::engine engine{ std::make_shared<ge::file_io>(), std::make_shared<test_logger>() };
	engine.run();
}

