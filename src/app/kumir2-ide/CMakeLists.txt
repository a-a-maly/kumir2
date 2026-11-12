project(kumir2-ide)
#cmake_minimum_required(VERSION 3.0)
#find_package(Kumir2 REQUIRED)

kumir2_add_launcher(
	NAME            kumir2-ide
	SPLASHSCREEN    "splashscreen-ide.png"
	WINDOW_ICON     "window-icon-ide.png"
	APP_ICON_NAME   "kumir2-ide"
	X_ICONS_DIR     "../../../app_icons/linux/hicolor"
	WIN_ICONS_DIR   "../../../app_icons/win32"
	X_NAME          "Kumir 2 Professional Edition"
	X_NAME_ru       "Кумир-Про 2"
	X_CATEGORIES    "Education,X-KDE-Edu-Misc"
	CONFIGURATION   "CourseManager,Editor,Actor*,KumirAnalizer\(preload=Files\),*CodeGenerator,KumirCodeRun,!CoreGUI"
)

