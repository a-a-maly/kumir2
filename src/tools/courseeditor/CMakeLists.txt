project(kumir2-courseeditor)
#cmake_minimum_required(VERSION 3.0)
#find_package(Kumir2 REQUIRED)
kumir2_use_qt(Core Gui Xml Svg)

set(HEADERS
	kumzadanie.h
	course_model.h
	csInterface.h
	editdialog.h
	interface.h
	#kumfiledialog.h
	mainwindow.h
	newkursdialog.h
	taskControlInterface.h
)

set(MOC_HEADERS
	course_model.h
	editdialog.h
	interface.h
	#kumfiledialog.h
	mainwindow.h
	newkursdialog.h
)

set(SOURCES
	course_model.cpp
	csInterface.cpp
	editdialog.cpp
	interface.cpp
	#kumfiledialog.cpp
	main.cpp
	mainwindow.cpp
	newkursdialog.cpp
)

set(FORMS
	editdialog.ui
	mainwindow.ui
	newkursdialog.ui
)

set(RESOURCES
	marks.qrc
)

kumir2_wrap_ui(UI_SOURCES ${FORMS})
kumir2_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})
kumir2_add_resources(QRC_SOURCES ${RESOURCES})

kumir2_add_tool(
	NAME        kumir2-courseeditor
	SOURCES      ${UI_SOURCES} ${MOC_SOURCES} ${SOURCES} ${RESOURCES} ${QRC_SOURCES}
	LIBRARIES   ${QT_LIBRARIES}
)
