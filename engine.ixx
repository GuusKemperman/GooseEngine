export module engine;

import logger;

namespace ge
{
	export class engine
	{
	public:	
		void run();
	
		logger m_logger{};
	};
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
		m_logger.log("Current frame: {}", num);
	}
}
