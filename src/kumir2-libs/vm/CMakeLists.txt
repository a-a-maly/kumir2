project(VM)
#cmake_minimum_required(VERSION 3.0)
#find_package(Kumir2 REQUIRED)

set(SOURCES
	vm_instruction.cpp
	vm_tableelem.cpp
	vm_bytecode.cpp
	vm_breakpoints_table.cpp
	vm_console_handlers.cpp
	vm_abstract_handlers.cpp
	variant.cpp
	vm.cpp
	vm_mrtl.cpp
	vm_mfiles.cpp
	vm_mstrings.cpp
	vm_mspecial.cpp
)

kumir2_add_library(
	NAME        VM
	SOURCES     ${SOURCES}
	LIBRARIES   KStdlib
)
