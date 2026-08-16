export module utils;

export import :memory;

namespace ge
{
	// Anchor symbol so MSVC emits an import library for this otherwise
	// template-only module. Consumers link `utils` purely to obtain its
	// module BMI for `import utils;`; without at least one exported symbol
	// no `utils.lib` is produced and linking against it fails (LNK1104).
	export API void utils_link_anchor();
}

void ge::utils_link_anchor()
{
}
