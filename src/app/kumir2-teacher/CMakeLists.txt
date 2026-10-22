project(kumir2-teacher)
#cmake_minimum_required(VERSION 3.0)
#find_package(Kumir2 REQUIRED)

kumir2_add_launcher(
	NAME            kumir2-teacher
	SPLASHSCREEN    "splashscreen-teacher.png"
	WINDOW_ICON     "window-icon-teacher.png"
	APP_ICON_NAME   "kumir2-teacher"
	X_ICONS_DIR     "../../../app_icons/linux/hicolor"
	WIN_ICONS_DIR   "../../../app_icons/win32"
	X_NAME          "Kumir 2 Teacher Edition"
	X_NAME_ru       "Кумир 2 для учителей"
	X_CATEGORIES    "Education,X-KDE-Edu-Misc"
	CONFIGURATION   "CourseManager,Editor\(teacher\),Actor*,KumirAnalizer\(teacher,preload=Files\),KumirCodeGenerator,KumirCodeRun,!CoreGUI"
)
