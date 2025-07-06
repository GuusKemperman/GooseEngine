export module services;

import std;

namespace ge
{
	class iservice_locator
	{
	public:
		template<typename T>
		std::shared_ptr<T> get_service();

		template<typename T>
		std::shared_ptr<T> make_service();
		
		template<typename From, typename To>
		void remap_service();
	};

	class iservice
	{
		
	};
}

export void MyFunc();