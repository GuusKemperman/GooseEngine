add_module("renderer" 
"renderer.ixx"
)

target_link_libraries("renderer" PRIVATE "core_traits")
target_link_libraries("renderer" PRIVATE "Opengl32")
