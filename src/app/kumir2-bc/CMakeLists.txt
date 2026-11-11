project(kumir2-bc)
#cmake_minimum_required(VERSION 3.0)
#find_package(Kumir2 REQUIRED)

kumir2_add_launcher(
	NAME            kumir2-bc
	CONFIGURATION   "Actor*(tablesOnly),!KumirCompilerTool,KumirCodeGenerator,KumirAnalizer\(teacher\)"
)

