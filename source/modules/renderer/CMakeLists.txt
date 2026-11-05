add_module("renderer" 
"renderer.ixx"
"target.ixx"
"utils.ixx"
)

target_link_libraries("renderer" PRIVATE "math")
target_link_libraries("renderer" PRIVATE "core_traits")
