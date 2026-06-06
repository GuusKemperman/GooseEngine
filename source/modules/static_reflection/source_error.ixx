export module static_reflection:source_error;

import stl;

namespace ge
{
	export struct source_error
	{
		source_error(std::string msg) :
			m_msg(std::move(msg))
		{
		}

		std::string m_msg{};

		// TODO add file / line info.
	};
}