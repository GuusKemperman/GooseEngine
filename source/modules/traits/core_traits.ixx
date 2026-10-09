export module core_traits;

export import :environment;
export import :system;

namespace ge
{
	// Anchor symbol so MSVC emits an import library for this otherwise
	// template-only module. Consumers link `utils` purely to obtain its
	// module BMI for `import utils;`; without at least one exported symbol
	// no `utils.lib` is produced and linking against it fails (LNK1104).
	export API void core_traits_link_anchor()
	{
	}
} // namespace ge

