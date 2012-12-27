find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin (earth PLUGINDEPS composite opengl cube LIBRARIES GLEW curl)
