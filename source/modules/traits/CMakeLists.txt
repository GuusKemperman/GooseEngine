add_module("core_traits" 
"core_traits.ixx" 
"environment.ixx"
"system.ixx"
)

target_link_libraries("core_traits" PRIVATE "runtime_reflection")
