project(KumirAnalizer)
#cmake_minimum_required(VERSION 3.0)
#find_package(Kumir2 REQUIRED)
#find_package(PythonInterp 3.2.0 REQUIRED)
if (NOT DEFINED PYTHON_EXECUTABLE)
	message(FATAL_ERROR "Variable PYTHON_EXECUTABLE is not set.")
endif()


kumir2_use_qt(Core Gui)

set(FORMS
	quickreferencewidget.ui
)

set(SOURCES
	kumiranalizerplugin.cpp
	analizer.cpp
	lexer.cpp
	statement.cpp
	pdautomata.cpp
	syntaxanalizer.cpp
	kumfilehandler.cpp
	quickreferencewidget.cpp
)

set(MOC_HEADERS
	kumiranalizerplugin.h
	analizer.h
	lexer.h
	pdautomata.h
	syntaxanalizer.h
	kumfilehandler.h
	quickreferencewidget.h
)

kumir2_wrap_ui(UI_SOURCES ${FORMS})
kumir2_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})

kumir2_add_plugin(
	NAME        KumirAnalizer
	SOURCES     ${UI_SOURCES} ${MOC_SOURCES} ${SOURCES}
	LIBRARIES   ${QT_LIBRARIES} VM ExtensionSystem ErrorMessages DataFormats Widgets
)

# Generate messages file
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/kumiranalizer")
add_custom_command(
	TARGET          KumirAnalizer    POST_BUILD
	COMMAND         ${PYTHON_EXECUTABLE}
	"${CMAKE_SOURCE_DIR}/src/kumir2-libs/errormessages/scanmessages.py"
	"--db=${CMAKE_SOURCE_DIR}/share/kumir2/kumiranalizer/messages.csv"
	"--out=${CMAKE_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/kumiranalizer/messages.csv"
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_SOURCE_DIR}/share/kumir2/kumiranalizer"
)

file(
	COPY        "${CMAKE_SOURCE_DIR}/share/kumir2/kumiranalizer/messages.csv"
	DESTINATION "${CMAKE_BINARY_DIR}/${KUMIR2_RESOURCES_DIR}/kumiranalizer/"
)

install(
	FILES       "${CMAKE_SOURCE_DIR}/share/kumir2/kumiranalizer/messages.csv"
	DESTINATION "${KUMIR2_RESOURCES_DIR}/kumiranalizer/"
)

