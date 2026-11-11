project(kumir2-checkcourse)
#cmake_minimum_required(VERSION 3.0)
#find_package(Kumir2 REQUIRED)

kumir2_add_launcher(
	NAME            kumir2-checkcourse
	CONFIGURATION   "ActorRobot,!CourseManager,KumirCodeGenerator,KumirCodeRun\(console\),KumirAnalizer\(teacher\)"
)

