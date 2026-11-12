project(kumir2-highgrade)
#cmake_minimum_required(VERSION 3.0)
#find_package(Kumir2 REQUIRED)

kumir2_add_launcher(
	NAME            kumir2-highgrade
	SPLASHSCREEN    "splashscreen-highgrade.png"
	WINDOW_ICON     "window-icon-highgrade.png"
	APP_ICON_NAME   "kumir2-highgrade"
	X_ICONS_DIR     "../../../app_icons/linux/hicolor"
	WIN_ICONS_DIR   "../../../app_icons/win32"
	X_NAME          "Kumir 2 High Grade School Edition"
	X_NAME_ru       "Кумир 2 для старших классов"
	X_CATEGORIES    "Education,X-KDE-Edu-Misc"
	CONFIGURATION   "CourseManager,Editor,Actor*,KumirAnalizer\(preload=Files,preload=Strings,preload=Keyboard\),*CodeGenerator,KumirCodeRun,!CoreGUI\(notabs,nostartpage,nosessions\)"
)
