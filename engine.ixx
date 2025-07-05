export module engine;
export import std;

import logger;
import file_io;

namespace ge
{
	export class engine
	{
	public:	
		engine(std::shared_ptr<ifile_io> file_io,
			std::shared_ptr<ilogger> logger);

		void run();

	private:
		std::shared_ptr<ifile_io> m_fileio{};
		std::shared_ptr<ilogger> m_logger{};
	};
}

ge::engine::engine(std::shared_ptr<ifile_io> file_io, std::shared_ptr<ilogger> logger) :
	m_fileio(std::move(file_io)),
	m_logger(std::move(logger))
{
}

void ge::engine::run()
{
	// Construct derived types if overriden by game/plugin
	// 

	// Different plugins should be 
	// new Renderer

	int num{};
	while (1)
	{
		num++;
		m_logger->log("Current frame: {}", num);
	}
}
