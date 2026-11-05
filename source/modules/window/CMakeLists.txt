add_module("window" 
"window.ixx"
)

target_link_libraries("window" PRIVATE "math")
target_link_libraries("window" PRIVATE "renderer")
target_link_libraries("window" PRIVATE "core_traits")
target_link_libraries("window" PRIVATE "Opengl32")
