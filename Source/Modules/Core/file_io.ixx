export module file_io;

import std;

namespace ge
{
	export class ifile_io
	{
	public:
		enum class project { engine, /*game*/ };
		enum class path_name { build_dir, intermediate_dir, assets_dir };

		virtual std::filesystem::path get_path(project proj, path_name path_name) const = 0;
		
		virtual std::unique_ptr<std::istream> read_file(const std::filesystem::path& path) const = 0;
	};

	export class file_io : public ifile_io
	{
		virtual std::filesystem::path get_path(project proj, path_name path_name) const override;

		virtual std::unique_ptr<std::istream> read_file(const std::filesystem::path& path) const override;
	};
}

std::filesystem::path ge::file_io::get_path([[maybe_unused]] project proj, path_name path_name) const
{
	switch (path_name)
	{
	case path_name::build_dir: return "/Build/";
	case path_name::intermediate_dir: return "/Intermediate/";
	case path_name::assets_dir: return "/Assets/";
	default: throw std::invalid_argument{ "Invalid proj/path_name combination" };
	}
}

std::unique_ptr<std::istream> ge::file_io::read_file(const std::filesystem::path& path) const
{
	std::unique_ptr<std::ifstream> stream = std::make_unique<std::ifstream>(path);

	if (stream == nullptr)
	{
		throw std::exception{};
	}

	if (!stream->is_open())
	{
		throw std::invalid_argument{ path.string() + " not found" };
	}

	return stream;
}