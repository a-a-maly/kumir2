#include "vm.hpp"
#include <fstream>
#include <set>

const int MAX_RECURSION_SIZE = 4000;

namespace VM
{

uint8_t KumirVM::returnCode() const
{
	AnyValue stackResult;
	if (valuesStack_.size() > 0) {
		const Variable &var = valuesStack_.top();
		if (var.dimension() == 0) {
			stackResult = var.value();
		}
	}
	uint8_t result = evaluationResult_;
	if (stackResult.toInt() > result) {
		result = stackResult.toInt();
	}
	return result;
}

static String makeCanonicalName(const String &filename)
{
	Kumir::String result;
	static const Kumir::Char slash = Char('/');
	size_t slashPos = filename.find_last_of(slash);
	if (slashPos == Kumir::String::npos) {
		result = filename;
	} else {
		result = filename.substr(slashPos + 1);
	}
	if (result.length() > 3 &&
		result[0] == Char('l') &&
		result[1] == Char('i') &&
		result[2] == Char('b')) {
		result = result.substr(3);
	}
	if (result.length() > 3 &&
		result[result.length() - 3] == Char('.') &&
		result[result.length() - 2] == Char('s') &&
		result[result.length() - 1] == Char('o')
	) {
		result.resize(result.length() - 3);
	}
	if (result.length() > 4 &&
		result[result.length() - 4] == Char('.') &&
		result[result.length() - 3] == Char('d') &&
		result[result.length() - 2] == Char('l') &&
		result[result.length() - 1] == Char('l')
	) {
		result.resize(result.length() - 4);
	}
	if (result.length() > 6 &&
		result[result.length() - 6] == Char('.') &&
		result[result.length() - 5] == Char('d') &&
		result[result.length() - 4] == Char('y') &&
		result[result.length() - 3] == Char('l') &&
		result[result.length() - 2] == Char('i') &&
		result[result.length() - 1] == Char('b')
	) {
		result.resize(result.length() - 6);
	}
	return result;
}

Variable &KumirVM::findVariable(uint8_t scope, uint16_t id)
{
	if (VariableScope(scope) == Bytecode::CONSTT) {
		return currentConstants_->at(id);
	} else if (VariableScope(scope) == Bytecode::LOCAL) {
		return currentLocals_->at(id);
	} else {
		return currentGlobals_->at(id);
	}
}

void KumirVM::setFunctor(Functor *functor)
{
	if (!functor) {
		return;
	}
	switch (functor->type()) {
	case Functor::ExternalModuleReset:
		externalModuleReset_ =
			dynamic_cast<ExternalModuleResetFunctor *>(functor);
		break;
	case Functor::ExternalModuleLoad:
		externalModuleLoad_ =
			dynamic_cast<ExternalModuleLoadFunctor *>(functor);
		break;
	case Functor::ExternalModuleCall:
		externalModuleCall_ =
			dynamic_cast<ExternalModuleCallFunctor *>(functor);
		break;
	case Functor::Input:
		input_ =
			dynamic_cast<InputFunctor *>(functor);
		break;
	case Functor::Output:
		output_ =
			dynamic_cast<OutputFunctor *>(functor);
		break;
	case Functor::ConvertFromString:
		customTypeFromString_ =
			dynamic_cast<CustomTypeFromStringFunctor *>(functor);
		break;
	case Functor::ConvertToString:
		customTypeToString_ =
			dynamic_cast<CustomTypeToStringFunctor *>(functor);
		break;
	case Functor::GetMainArgument:
		getMainArgument_ =
			dynamic_cast<GetMainArgumentFunctor *>(functor);
		break;
	case Functor::ReturnMainValue:
		returnMainValue_ =
			dynamic_cast<ReturnMainValueFunctor *>(functor);
		break;
	case Functor::Pause:
		pause_ =
			dynamic_cast<PauseFunctor *>(functor);
		break;
	case Functor::Delay:
		delay_ =
			dynamic_cast<DelayFunctor *>(functor);
		break;
	default:
		break;
	}
}

void KumirVM::setProgram(const Bytecode::Data &program, bool isMain, const String &filename, Kumir::String *error)
{
	if (isMain) {
		moduleContexts_.clear();
	}
	moduleContexts_.push_back(ModuleContext());
	moduleContexts_.back().filename = filename;
	int currentModuleContext = moduleContexts_.size() - 1;
	moduleContexts_.back().globals.clear();
	moduleContexts_.back().constants.clear();
	moduleContexts_.back().globals.reserve(16);
	moduleContexts_.back().moduleNames.clear();
	moduleContexts_.back().moduleNames.reserve(16);
	for (int i = 0; i < (int)moduleContexts_.back().globals.size(); i++) {
		moduleContexts_.back().globals.at(i).clear();
		moduleContexts_.back().globals.at(i).reserve(256);
	}
	moduleContexts_.back().constants.reserve(256);
	LocalsMap locals;
	for (int i = 0; i < (int)program.d.size(); i++) {
		const TableElem e = program.d[i];
		if (e.type == EL_GLOBAL) {
			if (moduleContexts_[currentModuleContext].globals.size() <= e.module) {
				moduleContexts_[currentModuleContext].globals.resize(e.module + 1);
				moduleContexts_[currentModuleContext].globals[e.module].reserve(256);
				moduleContexts_[currentModuleContext].moduleNames.resize(e.module + 1);
				moduleContexts_[currentModuleContext].moduleNames[e.module] = e.moduleLocalizedName;
			}
			if (moduleContexts_[currentModuleContext].globals[e.module].size() <= e.id) {
				moduleContexts_[currentModuleContext].globals[e.module].resize(e.id + 1);
			}
			moduleContexts_[currentModuleContext].globals[e.module][e.id] = fromTableElem(e);
		} else if (e.type == EL_CONST) {
			if (moduleContexts_[currentModuleContext].constants.size() <= e.id) {
				moduleContexts_[currentModuleContext].constants.resize(e.id + 1);
			}
			moduleContexts_[currentModuleContext].constants[e.id] = fromTableElem(e);

		} else if (e.type == EL_LOCAL) {
			uint32_t key = 0x00000000;
			uint32_t alg = e.algId;
			uint32_t mod = e.module;
			mod = mod << 16;
			key = mod | alg;
			VariantArray lcs;
			if (locals.count(key)) {
				lcs = locals[key];
			}
			lcs.push_back(fromTableElem(e));
			locals[key] = lcs;
		} else if (e.type == EL_EXTERN &&
			e.fileName.length() > 4 &&
			e.fileName.substr(e.fileName.length() - 4) == Kumir::Core::fromAscii(".kod")) {
#if !defined(WIN32) && !defined(_WIN32)
#define VM_LOCALE Kumir::UTF8
#else
#define VM_LOCALE Kumir::CP866
#endif
			int externModuleContext = -1;
			for (int m = 0; m < (int)moduleContexts_.size(); m++) {
				if (moduleContexts_[m].filename == e.fileName) {
					externModuleContext = m;
					break;
				}
			}
			if (externModuleContext == -1) {
				externModuleContext = currentModuleContext + 1;
				Kumir::String modulePath;
				if (e.fileName[0] == Kumir::Char('/') || programDirectory_.length() == 0) {
					modulePath = e.fileName;
				} else {
					modulePath = programDirectory_;
					modulePath.push_back(Kumir::Char('/'));
					modulePath += e.fileName;
				}
				Kumir::EncodingError encodingError;
				const std::string filename = Kumir::Coder::encode(VM_LOCALE, modulePath, encodingError);
				std::ifstream externalfile(filename.c_str());
				if (
					!Kumir::Files::exist(modulePath)
					|| !externalfile.is_open()
				) {
					int errorCode = errno;
					Kumir::String errorMessage = Kumir::Core::fromUtf8("Не могу загрузить внешний исполнитель: ")
						+ modulePath
						+ Kumir::Core::fromUtf8(" (ошибка ")
						+ Kumir::Converter::sprintfInt(errorCode, 10, 0, 0)
						+ Kumir::Core::fromAscii(") ")
						;
					if (error) {
						error->assign(errorMessage);
					}
					return;
				}
				Bytecode::Data programData;
				Bytecode::bytecodeFromDataStream(externalfile, programData);
				externalfile.close();
				setProgram(programData, false, e.fileName, error);
				if (error && error->length()) {
					return;
				}
			}
			uint32_t key = 0x00000000;
			uint32_t alg = e.algId;
			uint32_t mod = e.module;
			mod = mod << 16;
			key = mod | alg;
			uint32_t extKey = 0x00000000;
			bool found = false;
			const FunctionMap &externMap = moduleContexts_[externModuleContext].functions;
			for (FunctionMap::const_iterator it = externMap.begin();
				it != externMap.end();
				++it) {
				extKey = (*it).first;
				const String fname = (*it).second.name;
				if (fname == e.name) {
					found = true;
					break;
				}
			}
			if (!found) {
				Kumir::String errorMessage = Kumir::Core::fromUtf8("Во внешнем исполнителе \"") +
					e.fileName +
					Kumir::Core::fromUtf8("\" нет алгоритма \"") +
					e.name +
					Kumir::Core::fromAscii("\"");
				if (error) {
					error->assign(errorMessage);
				}
				return;
			}
			ExternReference reference;
			reference.platformDependent = false;
			reference.funcKey = extKey;
			reference.moduleContext = externModuleContext;
			reference.fileName = e.fileName;
			reference.moduleLocalizedName = e.moduleLocalizedName.length() > 0
				? e.name
				: e.fileName;
			moduleContexts_[currentModuleContext].externs[key] = reference;
		} else if (e.type == EL_EXTERN_INIT) {
			ExternReference reference;
			reference.platformDependent = true;
			reference.funcKey = 0xFFFFFFFF;
			reference.moduleAsciiName = e.moduleAsciiName;
			reference.moduleLocalizedName = e.moduleLocalizedName;
			reference.fileName = e.fileName;
			Kumir::EncodingError encodingError;
			reference.platformModuleName = Kumir::Coder::encode(
					VM_LOCALE,
					makeCanonicalName(e.fileName),
					encodingError
				);
			moduleContexts_[currentModuleContext].externInits.push_back(reference);
			if (externalModuleLoad_) {
				(*externalModuleLoad_)(reference.moduleAsciiName, reference.moduleLocalizedName, error);
				if (error && error->length() > 0) {
					return;
				}
			}
		} else if (e.type == EL_EXTERN) {
			ExternReference reference;
			uint32_t key = 0x00000000;
			uint32_t alg = e.algId;
			uint32_t mod = e.module;
			mod = mod << 16;
			key = mod | alg;
			reference.platformDependent = true;
			reference.funcKey = alg;
			reference.moduleAsciiName = e.moduleAsciiName;
			reference.moduleLocalizedName = e.moduleLocalizedName;
			reference.fileName = e.fileName;
			Kumir::EncodingError encodingError;
			reference.platformModuleName = Kumir::Coder::encode(
					VM_LOCALE,
					makeCanonicalName(e.fileName),
					encodingError
				);
			moduleContexts_[currentModuleContext].externs[key] = reference;
			if (externalModuleLoad_) {
				std::deque< std::string > algorithms =
					(*externalModuleLoad_)(
						reference.moduleAsciiName,
						reference.moduleLocalizedName,
						error
					);
				if (error && error->length() > 0) {
					return;
				}
			}
		} else if (e.type == EL_FUNCTION || e.type == EL_MAIN || e.type == EL_BELOWMAIN || e.type == EL_TESTING) {
			uint32_t key = 0x00000000;
			uint32_t alg = e.algId;
			uint32_t mod = e.module;
			mod = mod << 16;
			key = mod | alg;
			moduleContexts_[currentModuleContext].functions[key] = e;
			moduleContexts_[currentModuleContext].exportModuleId = mod;
			breakpointsTable_.registerSourceFileName(filename, e.module);
		} else if (e.type == EL_INIT) {
			if (moduleContexts_[currentModuleContext].globals.size() <= e.module) {
				moduleContexts_[currentModuleContext].globals.resize(e.module + 1);
				moduleContexts_[currentModuleContext].globals[e.module].reserve(256);
				moduleContexts_[currentModuleContext].moduleNames.resize(e.module + 1);
				moduleContexts_[currentModuleContext].moduleNames[e.module] =
					e.moduleLocalizedName;
			}
			moduleContexts_[currentModuleContext].inits.push_back(e);
			breakpointsTable_.registerSourceFileName(filename, e.module);
		}
	}
	for (FunctionMap::iterator it = moduleContexts_[currentModuleContext].functions.begin();
		it != moduleContexts_[currentModuleContext].functions.end();
		++it) {
		uint32_t key = (*it).first;
		const String &algName = (*it).second.name;
		if (locals.count(key)) {
			for (size_t j = 0; j < locals[key].size(); j++) {
				locals[key][j].setAlgorhitmName(algName);
			}
		}
	}
	for (LocalsMap::iterator it = locals.begin(); it != locals.end(); ++it) {
		uint32_t key = (*it).first;
		const VariantArray &arr = (*it).second;
		moduleContexts_[currentModuleContext].cleanLocalTables[key] = arr;
	}
	currentLocals_ = nullptr;
	currentGlobals_ = nullptr;
	currentConstants_ = nullptr;
}

KumirVM::KumirVM()
	: moduleContexts_(std::vector<ModuleContext>())
	, entryPoint_(EP_Main)
	, blindMode_(true)
	, nextCallInto_(false)
	, stacksMutex_()
	, debugHandler_(nullptr)
	, externalModuleLoad_(nullptr)
	, externalModuleReset_(nullptr)
	, externalModuleCall_(nullptr)
	, input_(nullptr)
	, output_(nullptr)
	, customTypeFromString_(nullptr)
	, customTypeToString_(nullptr)
	, getMainArgument_(nullptr)
	, returnMainValue_(nullptr)
	, pause_(nullptr)
	, delay_(nullptr)
	, lastContext_(Context())
	, backtraceSkip_(0)
	, error_(Kumir::String())
	, register0_(AnyValue(0))
	, valuesStack_(Stack<Variable>())
	, contextsStack_(Stack<Context>())
	, programDirectory_(Kumir::String())
	, currentConstants_(nullptr)
	, currentGlobals_(nullptr)
	, currentLocals_(nullptr)
	, consoleInputBuffer_(nullptr)
{

}

Variable KumirVM::fromTableElem(const Bytecode::TableElem &e)
{
	Variable r = e.initialValue;
	r.setDimension(e.dimension);
	r.setBaseType(e.vtype.front());
	r.setName(e.name);
	r.setConstantFlag(e.type == EL_CONST);
	r.setModuleName(e.moduleLocalizedName);
	r.setRecordModuleLocalizedName(e.recordModuleLocalizedName);
	r.setRecordModuleAsciiName(e.recordModuleAsciiName);
	r.setRecordClassAsciiName(e.recordClassAsciiName);
	r.setRecordClassLocalizedName(e.recordClassLocalizedName);
	return r;
}

void KumirVM::checkFunctors()
{
	if (!externalModuleLoad_) {
		static ExternalModuleLoadFunctor dummy;
		externalModuleLoad_ = &dummy;
	}
	if (!externalModuleReset_) {
		static ExternalModuleResetFunctor dummy;
		externalModuleReset_ = &dummy;
	}
	if (!externalModuleCall_) {
		static ExternalModuleCallFunctor dummy;
		externalModuleCall_ = &dummy;
	}
	if (!input_) {
		static InputFunctor dummy;
		input_ = &dummy;
	}
	if (!output_) {
		static OutputFunctor dummy;
		output_ = &dummy;
	}
	if (!customTypeFromString_) {
		static CustomTypeFromStringFunctor dummy;
		customTypeFromString_ = &dummy;
	}
	if (!customTypeToString_) {
		static CustomTypeToStringFunctor dummy;
		customTypeToString_ = &dummy;
	}
	if (!getMainArgument_) {
		static GetMainArgumentFunctor dummy;
		getMainArgument_ = &dummy;
	}
	if (!returnMainValue_) {
		static ReturnMainValueFunctor dummy;
		returnMainValue_ = &dummy;
	}
	if (!pause_) {
		static PauseFunctor dummy;
		pause_ = &dummy;
	}
	if (!delay_) {
		static DelayFunctor dummy;
		delay_ = &dummy;
	}
}

bool KumirVM::hasTestingAlgorithm() const
{
	const ModuleContext &mainContexnt = moduleContexts_.front();
	const FunctionMap &functions = mainContexnt.functions;
	typedef FunctionMap::const_iterator It;
	for (It i = functions.begin(); i != functions.end(); i++) {
		const Bytecode::TableElem &element = i->second;
		if (element.type == Bytecode::EL_TESTING) {
			return true;
		}
	}
	return false;
}

void KumirVM::removeAllBreakpoints()
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	breakpointsTable_.removeAllBreakpoints();
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

void KumirVM::insertOrChangeBreakpoint(const bool enabled, const Kumir::String &fileName, const uint32_t lineNo, const uint32_t ignoreCount, const Kumir::String &)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	breakpointsTable_.insertOrChangeBreakpoint(enabled, fileName, lineNo, ignoreCount, 0);
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

void KumirVM::insertSingleHitBreakpoint(const Kumir::String &fileName, uint32_t lineNo)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	breakpointsTable_.insertSingleHitBreakpoint(fileName, lineNo);
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

void KumirVM::removeBreakpoint(const Kumir::String &fileName, const uint32_t lineNo)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	breakpointsTable_.removeBreakpoint(fileName, lineNo);
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

void KumirVM::reset()
{
	// Clear everything
	if (stacksMutex_) {
		stacksMutex_->reset();
	}
	lastContext_ = Context();
	blindMode_ = false;
	nextCallInto_ = false;
	backtraceSkip_ = 0;
	stepsCounter_ = 0u;
	error_.clear();
	register0_ = AnyValue();
	Variable::ignoreUndefinedError = false;
	valuesStack_.reset();
	cacheStack_.reset();
	contextsStack_.reset();
	previousLineNo_ = -1;
	previousColStart_ = previousColEnd_ = 0u;
	evaluationResult_ = 0u;

	checkFunctors();

	currentLocals_ = nullptr;
	currentGlobals_ = nullptr;
	currentConstants_ = nullptr;

	Bytecode::TableElem *pMainProgram = nullptr;
	Bytecode::TableElem *pTestingProgram = nullptr;

	ModuleContext &mainModuleContext = moduleContexts_.front();

	// Find testing and main algorithms
	for (FunctionMap::iterator it = mainModuleContext.functions.begin();
		it != mainModuleContext.functions.end();
		++it) {
		TableElem &e = (*it).second;
		if (e.type == EL_MAIN || e.type == EL_BELOWMAIN) {
			pMainProgram = &e;
		}
		if (e.type == EL_TESTING) {
			pTestingProgram = &e;
		}
	}

	// Prepare startup context
	Context c;
	if (entryPoint_ == EP_Main && pMainProgram && (pMainProgram->type == EL_MAIN || pMainProgram->type == EL_BELOWMAIN)) {
		uint32_t mod = pMainProgram->module;
		uint32_t alg = pMainProgram->algId;
		uint32_t key = (mod << 16) | alg;
		c.locals = mainModuleContext.cleanLocalTables[key];
		c.program = &(pMainProgram->instructions);
		c.type = pMainProgram->type;
		c.runMode = CRM_ToEnd;
		c.algId = pMainProgram->algId;
		c.moduleId = pMainProgram->module;
		c.name = pMainProgram->name;
	}

	if (entryPoint_ == EP_Testing && pTestingProgram && pTestingProgram->type == EL_TESTING) {
		uint32_t mod = pTestingProgram->module;
		uint32_t alg = pTestingProgram->algId;
		uint32_t key = (mod << 16) | alg;
		c.locals = mainModuleContext.cleanLocalTables[key];
		c.program = &(pTestingProgram->instructions);
		c.type = EL_TESTING;
		c.runMode = CRM_ToEnd;
		c.algId = pTestingProgram->algId;
		c.moduleId = pTestingProgram->module;
		c.name = pTestingProgram->name;
	}
	c.IP = -1;

	if (debugHandler_) {
		debugHandler_->debuggerReset();
		debugHandler_->debuggerNoticeBeforePushContext();
	}

	if (c.program) {
		// Push startup context to stack (if non empty)
		contextsStack_.push(c);
		currentLocals_ = &(contextsStack_.top().locals);
	}

	if (debugHandler_) {
		debugHandler_->debuggerNoticeAfterPushContext();
	}

	currentConstants_ = &(mainModuleContext.constants);
	currentGlobals_ = mainModuleContext.globals.empty()
		? nullptr
		: &(mainModuleContext.globals.back());


	// Each kumir module have 'initialization' section,
	// so push all these sections (if any) into stack
	// to call them BEFORE startup context
	for (int moduleContextNo = moduleContexts_.size() - 1;
		moduleContextNo >= 0;
		moduleContextNo--) {
		const ModuleContext &currentModule = moduleContexts_.at(moduleContextNo);
		const std::deque<Bytecode::TableElem> &inits = currentModule.inits;
		for (int initNo = inits.size() - 1; initNo >= 0; initNo--) {
			const Bytecode::TableElem &e = inits.at(initNo);
			if (e.instructions.size() > 0) {
				Context initContext;
				initContext.program = &(e.instructions);
				initContext.type = EL_INIT;
				initContext.runMode = moduleContextNo == 0 ? CRM_OneStep : CRM_ToEnd;
				initContext.moduleId = e.module;
				initContext.algId = -1;
				initContext.IP = -1;
				initContext.moduleContextNo = moduleContextNo;
				contextsStack_.push(initContext);
				currentConstants_ =
					&(moduleContexts_[moduleContextNo].constants);
				currentGlobals_ =
					&(moduleContexts_[moduleContextNo].globals[e.module]);
			}
		}
	}

	typedef std::pair<std::string, Kumir::String> ModuleRef;

	std::set<ModuleRef> usedExternalModules;

	// Push globals to debugger and make a list of used external modules
	for (size_t i_context = 0; i_context < moduleContexts_.size(); i_context++) {
		const ModuleContext &mc = moduleContexts_[i_context];

		const ExternsMap &contextExterns = mc.externs;
		const std::list<ExternReference> &contextExternInits = mc.externInits;
		for (ExternsMap::const_iterator itExtern = contextExterns.begin();
			itExtern != contextExterns.end(); ++itExtern) {
			const ExternReference &externReference = itExtern->second;
			if (externReference.platformDependent) {
				ModuleRef ref;
				ref.first = externReference.moduleAsciiName;
				ref.second = externReference.moduleLocalizedName;
				if (!usedExternalModules.count(ref)) {
					usedExternalModules.insert(ref);
				}
			}
		}

		for (std::list<ExternReference>::const_iterator itExtern = contextExternInits.begin();
			itExtern != contextExternInits.end(); ++itExtern) {
			const ExternReference &externReference = *itExtern;
			if (externReference.platformDependent) {
				ModuleRef ref;
				ref.first = externReference.moduleAsciiName;
				ref.second = externReference.moduleLocalizedName;
				if (!usedExternalModules.count(ref)) {
					usedExternalModules.insert(ref);
				}
			}
		}
	}

	// Prepare standard library
	Kumir::initStandardLibrary();

	// Reset used external modules
	if (externalModuleReset_) {
		for (std::set<ModuleRef>::const_iterator it = usedExternalModules.begin();
			it != usedExternalModules.end();
			++it
		) {
			const ModuleRef &ref = * it;
			const std::string &moduleAsciiName = ref.first;
			const Kumir::String &moduleLocalizedName = ref.second;
			(*externalModuleReset_)(moduleAsciiName, moduleLocalizedName, &error_);
		}
	}

	if (consoleInputBuffer_) {
		consoleInputBuffer_->clear();
	}
	Kumir::Files::setConsoleInputBuffer(consoleInputBuffer_);
	Kumir::Files::setConsoleOutputBuffer(consoleOutputBuffer_);
	if (! stacksMutex_) {
		stacksMutex_.reset(new CriticalSectionLocker);
	}
}


bool KumirVM::hasMoreInstructions() const
{
	if (contextsStack_.size() > 0) {
		const std::vector<Bytecode::Instruction> *program = contextsStack_.at(0).program;
		int IP = contextsStack_.at(0).IP;
		return program && IP < int(program->size());
	} else {
		return false;
	}
}

void KumirVM::evaluateNextInstruction()
{
	int ip = contextsStack_.top().IP;
	if (ip == -1) {
		ip = 0;
	}
	const std::vector<Instruction> *program = contextsStack_.top().program;
	if (ip >= (int) program->size()) {
		return;
	}
	const Instruction &instr = program->at(ip);
	switch (instr.type) {
	case CALL:
		do_call(instr.module, instr.arg);
		break;
	case INIT:
		do_init(instr.scope, instr.arg);
		break;
	case SETARR:
		do_setarr(instr.scope, instr.arg);
		break;
	case UPDARR:
		do_updarr(instr.scope, instr.arg);
		break;
	case STORE:
		do_store(instr.scope, instr.arg);
		break;
	case STOREARR:
		do_storearr(instr.scope, instr.arg);
		break;
	case LOAD:
		do_load(instr.scope, instr.arg);
		break;
	case LOADARR:
		do_loadarr(instr.scope, instr.arg);
		break;
	case JUMP:
		do_jump(instr.arg);
		break;
	case JNZ:
		do_jnz(instr.registerr, instr.arg);
		break;
	case JZ:
		do_jz(instr.registerr, instr.arg);
		break;
	case POP:
		do_pop(instr.registerr);
		break;
	case PUSH:
		do_push(instr.registerr);
		break;
	case CLOAD:
		do_cload();
		break;
	case CSTORE:
		do_cstore();
		break;
	case CDROPZ:
		do_cdropz();
		break;
	case CACHEBEGIN:
		do_cachebegin();
		break;
	case CACHEEND:
		do_cacheend();
		break;
	case RET:
		do_ret();
		break;
	case ERRORR:
		do_error(instr.scope, instr.arg);
		break;
	case LINE:
		do_line(instr);
		break;
	case REF:
		do_ref(instr.scope, instr.arg);
		break;
	case REFARR:
		do_refarr(instr.scope, instr.arg);
		break;
	case SETREF:
		do_setref(instr.scope, instr.arg);
		break;
	case CTL:
		do_ctl(instr.module, instr.arg);
		break;
	case INRANGE:
		do_inrange();
		break;
	case SUM:
		do_sum();
		break;
	case SUB:
		do_sub();
		break;
	case MUL:
		do_mul();
		break;
	case DIV:
		do_div();
		break;
	case POW:
		do_pow();
		break;
	case NEG:
		do_neg();
		break;
	case AND:
		do_and();
		break;
	case OR:
		do_or();
		break;
	case EQ:
		do_eq();
		break;
	case NEQ:
		do_neq();
		break;
	case LS:
		do_ls();
		break;
	case GT:
		do_gt();
		break;
	case LEQ:
		do_leq();
		break;
	case GEQ:
		do_geq();
		break;
	case SHOWREG:
		do_showreg(instr.registerr);
		break;
	case CLEARMARG:
		do_clearmarg(instr.arg);
		break;
	case PAUSE:
		do_pause(instr.arg);
		break;
	case HALT:
		do_halt(instr.arg);
		break;
	default:
		nextIP();
		break;
	}
	if (error_.length() == 0 && Kumir::Core::getError().length() > 0) {
		error_ = Kumir::Core::getError();
	}
}

/***** BEGIN INSTRUCTIONS IMPLEMENTATION *****/

void KumirVM::do_call(uint8_t mod, uint16_t alg)
{
	uint32_t module = mod << 16;
	uint32_t algorithm = alg;
	uint32_t p = module | algorithm;

	if (mod == 0xF0) { // stdlib
		do_stdcall(alg);
	} else if (mod == 0xF1) {
		do_filescall(alg);
	} else if (mod == 0xF2) {
		do_stringscall(alg);
	} else if (mod == 0xFF) {
		do_specialcall(alg);
	} else if (moduleContexts_[contextsStack_.top().moduleContextNo].functions.count(p)) {

		if (contextsStack_.size() >= MAX_RECURSION_SIZE) {
			error_ = Kumir::Core::fromUtf8("Слишком много вложенных вызовов алгоритмов");
		} else {
			if (stacksMutex_) {
				stacksMutex_->lock();
			}
			Context c;
			c.program = & (moduleContexts_[contextsStack_.top().moduleContextNo].functions[p].instructions);
			c.locals = moduleContexts_[contextsStack_.top().moduleContextNo].cleanLocalTables[p];
			c.type = moduleContexts_[contextsStack_.top().moduleContextNo].functions[p].type;
			if (nextCallInto_) {
				c.runMode = CRM_OneStep;
			} else if (contextsStack_.top().type == EL_BELOWMAIN && c.type == EL_MAIN) {
				c.runMode = contextsStack_.top().runMode;
			} else {
				c.runMode = CRM_ToEnd;
			}
			c.moduleId = moduleContexts_[contextsStack_.top().moduleContextNo].functions[p].module;
			c.algId = moduleContexts_[contextsStack_.top().moduleContextNo].functions[p].algId;
			if (!blindMode_) {
				c.name = moduleContexts_[contextsStack_.top().moduleContextNo].functions[p].name;
			}
			c.moduleContextNo = contextsStack_.top().moduleContextNo;
			if (stacksMutex_) {
				stacksMutex_->unlock();
			}
			if (debugHandler_ && c.runMode == CRM_OneStep) {
				debugHandler_->debuggerNoticeBeforePushContext();
			}
			if (stacksMutex_) {
				stacksMutex_->lock();
			}
			contextsStack_.push(c);
			if (stacksMutex_) {
				stacksMutex_->unlock();
			}
			if (debugHandler_ && c.runMode == CRM_OneStep) {
				debugHandler_->debuggerNoticeAfterPushContext();
			}
			if (stacksMutex_) {
				stacksMutex_->lock();
			}
			nextCallInto_ = false;
			valuesStack_.pop(); // current implementation doesn't requere args count
			currentLocals_ = &(contextsStack_.top().locals);
			currentGlobals_ =
				&(moduleContexts_[c.moduleContextNo].globals[c.moduleId]);
			currentConstants_ =
				&(moduleContexts_[c.moduleContextNo].constants);
			if (stacksMutex_) {
				stacksMutex_->unlock();
			}
		}

	} else if (moduleContexts_[contextsStack_.top().moduleContextNo].externs.count(p)) {
		if (contextsStack_.size() >= MAX_RECURSION_SIZE) {
			error_ = Kumir::Core::fromUtf8("Слишком много вложенных вызовов алгоритмов");
		} else {
			ExternReference reference = moduleContexts_[contextsStack_.top().moduleContextNo].externs[p];
			if (!reference.platformDependent) {
				// External call of algorithm found in another kumir file
				if (stacksMutex_) {
					stacksMutex_->lock();
				}
				uint32_t key = reference.funcKey;
				Context c;
				c.program = & (moduleContexts_[reference.moduleContext].functions[key].instructions);
				c.locals = moduleContexts_[reference.moduleContext].cleanLocalTables[key];
				c.type = moduleContexts_[reference.moduleContext].functions[key].type;
				c.runMode = CRM_ToEnd;
				c.moduleId = moduleContexts_[reference.moduleContext].functions[key].module;
				c.algId = moduleContexts_[reference.moduleContext].functions[key].algId;
				c.moduleContextNo = reference.moduleContext;
				contextsStack_.push(c);
				currentLocals_ = &(contextsStack_.top().locals);
				currentGlobals_ =
					&(moduleContexts_[c.moduleContextNo].globals[c.moduleId]);
				currentConstants_ =
					&(moduleContexts_[reference.moduleContext].constants);
				nextCallInto_ = false;
				valuesStack_.pop(); // current implementation doesn't requere args count
				if (stacksMutex_) {
					stacksMutex_->unlock();
				}
			} else if (externalModuleCall_) {
				uint16_t algKey = reference.funcKey & 0xffff;
				const std::string moduleAsciiName = reference.moduleAsciiName;
				const Kumir::String moduleLocalizedName = reference.moduleLocalizedName;
				if (stacksMutex_) {
					stacksMutex_->lock();
				}
				int argsCount = valuesStack_.pop().toInt();
				std::deque<Variable> args;
				for (int i = 0; i < argsCount; i++) {
					Variable arg = valuesStack_.pop();
					args.push_front(arg);
				}
				if (stacksMutex_) {
					stacksMutex_->unlock();
				}
				AnyValue algResult;
				Kumir::String localError;
				algResult = (*externalModuleCall_)(
						moduleAsciiName, moduleLocalizedName, algKey, args, &localError
					);

				if (stacksMutex_) {
					stacksMutex_->lock();
				}
				if (localError.length() > 0) {
					if (error_.length() == 0) {
						error_ = localError;
					}
				} else {
					if (algResult.isValid()) {
						valuesStack_.push(Variable(algResult));
						register0_ = algResult;
					}
				}
				if (stacksMutex_) {
					stacksMutex_->unlock();
				}
			} else {
				error_ = Kumir::Core::fromUtf8("Вызов алгоритма из недоступного исполнителя");
			}
		}
	} else {
		error_ = Kumir::Core::fromUtf8("Вызов алгоритма из недоступного исполнителя");
	}
	if (Kumir::Core::getError().length() > 0 && error_.length() == 0) {
		error_ = Kumir::Core::getError();
	}
	nextIP();
}

Kumir::FileType KumirVM::fromRecordValue(const Record &record)
{
	Kumir::FileType result;
	result.fullPath = record.fields[0].toString();
	result.mode = record.fields[1].toInt();
	result.type = record.fields[2].toInt();
	result.valid = record.fields[3].toBool();
	return result;
}

Record KumirVM::toRecordValue(const Kumir::FileType &ft)
{
	Record record;
	record.fields.resize(4);
	record.fields[0] = AnyValue(ft.fullPath);
	record.fields[1] = AnyValue(ft.mode);
	record.fields[2] = AnyValue(ft.type);
	record.fields[3] = AnyValue(ft.valid);
	return record;
}

void KumirVM::do_init(uint8_t s, uint16_t id)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	findVariable(s, id).init();
	nextIP();
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

void KumirVM::do_setarr(uint8_t s, uint16_t id)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	Variable &var = findVariable(s, id);
	const int dim = var.dimension();
	int bounds[7];
	if (dim > 0) {
		String name;
		bounds[6] = dim * 2;
		for (int i = 0; i < dim * 2; i++) {
			bounds[i] = valuesStack_.pop().toInt();
		}
		if (debugHandler_ && currentContext().runMode == CRM_OneStep) {
			stacksMutex_->unlock();
			debugHandler_->debuggerNoticeBeforeArrayInitialize(var, bounds);
			stacksMutex_->lock();
		}
		var.setBounds(bounds);
		if (debugHandler_ && currentContext().runMode == CRM_OneStep) {
			stacksMutex_->unlock();
			debugHandler_->debuggerNoticeAfterArrayInitialize(var);
			stacksMutex_->lock();
		}
		if (!blindMode_) {
			name = var.name();
		}
		error_ = Kumir::Core::getError();
		const int lineNo = contextsStack_.top().lineNo;
		if (lineNo != -1 &&
			!blindMode_ &&
			contextsStack_.top().type != EL_BELOWMAIN
		) {
			String boundsText;
			for (int i = 0; i < dim; i++) {
				boundsText += Kumir::Converter::sprintfInt(bounds[i * 2], 10, 0, 0);
				boundsText.push_back(Char(':'));
				boundsText += Kumir::Converter::sprintfInt(bounds[i * 2 + 1], 10, 0, 0);
				if (i < dim - 1) {
					boundsText.push_back(Char(','));
				}
			}
			const String notice = name + Kumir::Core::fromAscii("[") + boundsText + Kumir::Core::fromAscii("]");
			if (debugHandler_ && currentContext().runMode == CRM_OneStep) {
				if (contextsStack_.top().moduleContextNo == 0) {
					debugHandler_->appendTextToMargin(lineNo, notice);
				}
			}
		}
	}
	nextIP();
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

void KumirVM::do_updarr(uint8_t s, uint16_t id)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	Variable &var = findVariable(s, id);
	const int dim = var.dimension();
	int bounds[7];
	if (dim > 0) {
		String name;
		int effectiveBounds[7];
		bounds[6] = effectiveBounds[6] = dim * 2;
		for (int i = 0; i < dim * 2; i++) {
			bounds[i] = valuesStack_.pop().toInt();
		}
		if (debugHandler_ && currentContext().runMode == CRM_OneStep) {
			stacksMutex_->unlock();
			debugHandler_->debuggerNoticeBeforeArrayInitialize(var, bounds);
			stacksMutex_->lock();
		}
		var.updateBounds(bounds);
		if (debugHandler_ && currentContext().runMode == CRM_OneStep) {
			stacksMutex_->unlock();
			debugHandler_->debuggerNoticeAfterArrayInitialize(var);
			stacksMutex_->lock();
		}
		var.getEffectiveBounds(effectiveBounds);
		if (!blindMode_) {
			name = var.myName();
		}
		error_ = Kumir::Core::getError();
		const int lineNo = contextsStack_.top().lineNo;
		if (lineNo != -1 &&
			!blindMode_ &&
			contextsStack_.top().moduleContextNo == 0 &&
			contextsStack_.top().type != EL_BELOWMAIN
		) {
			String boundsText;
			for (int i = 0; i < dim; i++) {
				boundsText += Kumir::Converter::sprintfInt(bounds[i * 2], 10, 0, 0);
				boundsText.push_back(Char(':'));
				boundsText += Kumir::Converter::sprintfInt(bounds[i * 2 + 1], 10, 0, 0);
				if (i < dim - 1) {
					boundsText.push_back(Char(','));
				}
			}
			const String notice = name + Kumir::Core::fromAscii("[") + boundsText + Kumir::Core::fromAscii("]");
			if (debugHandler_)
				if (contextsStack_.top().moduleContextNo == 0) {
					debugHandler_->appendTextToMargin(lineNo, notice);
				}
		}
	}
	nextIP();
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

void KumirVM::do_store(uint8_t s, uint16_t id)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	const Variable &value = valuesStack_.top();
	const int lineNo = contextsStack_.top().lineNo;
	Variable &variable = findVariable(s, id);
	const int dim = variable.dimension();
	ValueType t = variable.baseType();
	int bounds[7];
	if (dim > 0) {
		value.getBounds(bounds);
	}
	if (value.isConstant()) {
		variable.setConstValue(value);
	} else {
		if (dim > 0) {
			variable.setBounds(bounds);
		}
		variable.setValue(value.value());
		variable.setDimension(value.dimension());
	}
	if (lineNo != -1 &&
		!blindMode_ &&
		contextsStack_.top().type != EL_BELOWMAIN &&
		contextsStack_.top().moduleContextNo == 0 &&
		value.dimension() == 0
	) {
		const String &name = variable.myName();
		String svalue;
		if (t == VT_string) {
			const String valueString = value.toString();
			svalue.reserve(valueString.length() + 2);
			svalue.push_back(Char('"'));
			svalue.append(valueString);
			svalue.push_back(Char('"'));
		} else if (t == VT_char) {
			const Char valueChar = value.toChar();
			svalue.reserve(3);
			svalue.push_back(Char('\''));
			svalue.push_back(valueChar);
			svalue.push_back(Char('\''));
		} else if (t == VT_int) {
			svalue = Kumir::Converter::sprintfInt(value.toInt(), 10, 0, 0);
		} else if (t == VT_real) {
			svalue = Kumir::Converter::sprintfReal(value.toReal(), '.', false, 0, -1, 0);
		} else if (t == VT_bool) {
			static const String YES = Kumir::Core::fromUtf8("да");
			static const String NO = Kumir::Core::fromUtf8("нет");
			svalue = value.toBool() ? YES : NO;
		} else if (t == VT_record) {
			Kumir::String localError;
			svalue = (*customTypeToString_)(variable, &localError);
		}
		if (debugHandler_ && svalue.length() > 0) {
			const String message = name + Char('=') + svalue;
			if (contextsStack_.top().moduleContextNo == 0) {
				debugHandler_->appendTextToMargin(lineNo, message);
			}
		}
		if (debugHandler_ && currentContext().runMode == CRM_OneStep) {
			stacksMutex_->unlock();
			debugHandler_->debuggerNoticeOnValueChanged(variable, nullptr);
			stacksMutex_->lock();
		}
	}
	if (contextsStack_.top().type == Bytecode::EL_BELOWMAIN) {
		Variable::unsetError();
	}
	error_ = Kumir::Core::getError();
	nextIP();
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

void KumirVM::do_load(uint8_t s, uint16_t id)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	Variable val;
	Variable &variable = findVariable(s, id);
	const int dim = variable.dimension();
	int bounds[7];
	val.setBaseType(variable.baseType());
	val.setDimension(variable.dimension());
	if (dim > 0) {
		variable.getBounds(bounds);
		val.setBounds(bounds);
	}
	if (VariableScope(s) == CONSTT) {
		bool wasError = Kumir::Core::getError().length() > 0;
		AnyValue v = variable.value();
		if (!wasError) {
			Variable::unsetError();
		}
		val.setValue(v);
		val.setConstantFlag(true);
	} else {
		val.setValue(variable.value());
	}

	bool isRetVal = VariableScope(s) == LOCAL
		&& variable.algorhitmName() == variable.name();
	if (isRetVal && isRunningMain()) {
		Variable::unsetError();
	}
	if (Kumir::Core::getError().length() == 0) {
		valuesStack_.push(val);
		if (val.dimension() == 0) {
			register0_ = val.value();
		}
		if (isRetVal && isRunningMain()) {
			Variable::unsetError();
		}
	}
	error_ = Kumir::Core::getError();
	nextIP();
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}

bool KumirVM::isRunningMain() const
{
	bool mainAtTop = Bytecode::EL_MAIN == contextsStack_.top().type;
	bool noUpStacks =
		1 == contextsStack_.size()
		||
		(
			contextsStack_.size() > 1 &&
			Bytecode::EL_BELOWMAIN == contextsStack_.at(contextsStack_.size() - 2).type
		);
	return mainAtTop && noUpStacks;
}

void KumirVM::do_storearr(uint8_t s, uint16_t id)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	String name;
	String svalue;
	const int lineNo = contextsStack_.top().lineNo;
	String sindeces;
	Variable &variable = findVariable(s, id);
	const int dim = variable.dimension();
	if (!blindMode_) {
		name = variable.name();
	}
	int indeces[4];
	indeces[3] = dim;
	if (dim > 0) {
		for (int i = 0; i < dim; i++) {
			indeces[i] = valuesStack_.pop().toInt();
			if (!sindeces.empty()) {
				sindeces.push_back(',');
			}
			sindeces += Kumir::Converter::sprintfInt(indeces[i], 10, 0, 0);
		}
		const Variable &value = valuesStack_.top();
		ValueType t = VT_void;
		if (!blindMode_) {
			svalue = value.toString();
		}
		variable.setValue(indeces, value.value());
		t = variable.baseType();
		if (t == VT_string)
			if (lineNo != -1 && !blindMode_) {
				svalue.insert(0, 1, Char('"'));
				svalue.push_back(Char('"'));
			}
		if (t == VT_char)
			if (lineNo != -1 && !blindMode_) {
				svalue.insert(0, 1, Char('\''));
				svalue.push_back(Char('\''));
			}
	}
	if (lineNo != -1 &&
		!blindMode_
	) {
		String notice = name;
		notice.push_back(Char('['));
		notice += sindeces;
		notice.push_back(Char(']'));
		notice.push_back(Char('='));
		notice += svalue;
		if (debugHandler_) {
			if (contextsStack_.top().moduleContextNo == 0) {
				debugHandler_->appendTextToMargin(lineNo, notice);
			}
		}
		if (debugHandler_ && currentContext().runMode == CRM_OneStep) {
			if (debugHandler_ && currentContext().runMode == CRM_OneStep) {
				stacksMutex_->unlock();
				debugHandler_->debuggerNoticeOnValueChanged(variable, indeces);
				stacksMutex_->lock();
			}
		}
	}
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
	nextIP();
}

void KumirVM::do_loadarr(uint8_t s, uint16_t id)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	Variable &variable = findVariable(s, id);
	const int dim = variable.dimension();
	const ValueType vt = variable.baseType();

	if (dim > 0 || vt == VT_string) {
		int indeces[4];
		indeces[3] = dim;
		for (int i = 0; i < dim; i++) {
			indeces[i] = valuesStack_.pop().toInt();
		}
		Variable val;
		AnyValue vv;
		val.setBaseType(vt);
		vv = variable.value(indeces);
		if (vv.isValid()) {
			val.setValue(vv);
			valuesStack_.push(val);
			if (val.baseType() == VT_int) {
				register0_ = val.toInt();
			} else if (val.baseType() == VT_real) {
				register0_ = val.toReal();
			} else if (val.baseType() == VT_char) {
				register0_ = val.toChar();
			} else if (val.baseType() == VT_string) {
				register0_ = val.toString();
			} else if (val.baseType() == VT_bool) {
				register0_ = val.toBool();
			}
		}
	}
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
	nextIP();
}

void KumirVM::do_ref(uint8_t s, uint16_t id)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	Variable &variable = findVariable(s, id);
	Variable ref = variable.toReference();
	if (!blindMode_) {
		ref.setReferenceStackContextNo(VariableScope(s) == Bytecode::LOCAL
			? contextsStack_.size() - 1
			: -1
		);
	}
	if (ref.isReference()) {
		valuesStack_.push(ref);
	}
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
	nextIP();
}

void KumirVM::do_setref(uint8_t s, uint16_t id)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	Variable ref = valuesStack_.top();
	int bounds[7];
	ref.getEffectiveBounds(bounds);
	String name;
	if (!ref.isReference()) {
		error_ = Kumir::Core::fromAscii("Internal error");
	} else {
		Variable &variable = findVariable(s, id);
		variable.setReference(ref.reference(), bounds);
		name = variable.myName();
	}
	const int lineNo = contextsStack_.top().lineNo;
	if (lineNo != -1 &&
		!blindMode_ &&
		contextsStack_.top().type != EL_BELOWMAIN
	) {
		const String qn = ref.algorhitmName().empty()
			? ref.name()
			: ref.algorhitmName() + Kumir::Core::fromAscii("::") + ref.name();
		bool startsWithAt = qn.length() > 0 && qn.at(0) == Char('@');
		if (!startsWithAt) {
//            name.push_back('=');
//            name.push_back('&');
//            name += qn;
			if (debugHandler_) {
				if (contextsStack_.top().moduleContextNo == 0) {
					debugHandler_->appendTextToMargin(lineNo, name);
				}
			}
		}
	}
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
	nextIP();
}

void KumirVM::do_refarr(uint8_t s, uint16_t id)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	Variable &variable = findVariable(s, id);
	const int dim = variable.dimension();
	if (dim > 0) {
		int indeces[4];
		indeces[3] = dim;
		for (int i = 0; i < dim; i++) {
			indeces[i] = valuesStack_.pop().toInt();
		}
		Variable ref = variable.toReference(indeces);
		valuesStack_.push(ref);
	}
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
	nextIP();
}

void KumirVM::do_jump(uint16_t ip)
{
	contextsStack_.top().IP = ip;
}

void KumirVM::do_jnz(uint8_t r, uint16_t ip)
{
	const AnyValue &registerValue = r == 0u
		? register0_
		: currentContext().registers[r];

	const bool value = registerValue.toBool();
	if (value) {
		currentContext().IP = ip;
	} else {
		nextIP();
	}
}


void KumirVM::do_jz(uint8_t r, uint16_t ip)
{
	const AnyValue &registerValue = r == 0u
		? register0_
		: currentContext().registers[r];

	const bool value = registerValue.toBool();
	if (! value) {
		currentContext().IP = ip;
	} else {
		nextIP();
	}
}

void KumirVM::do_push(uint8_t r)
{
	AnyValue v;
	if (r == 0) {
		v = register0_;
	} else {
		v = contextsStack_.top().registers[r];
	}
	valuesStack_.push(Variable(v));
	nextIP();
}

void KumirVM::do_pop(uint8_t r)
{
	Variable v = valuesStack_.pop();
	AnyValue &registerToStore = r == 0u
		? register0_
		: currentContext().registers[r];
	if (v.hasValue() && v.dimension() == 0u) {
		registerToStore = v.value();
	}

	nextIP();
}

void KumirVM::do_cload()
{
	const std::pair<bool, Variable> cacheItem = cacheStack_.pop();
	const Variable &value = cacheItem.second;
	valuesStack_.push(value);
	nextIP();
}

void KumirVM::do_cstore()
{
	const Variable &v = valuesStack_.top();
	const std::pair<bool, Variable> cacheItem = std::pair<bool, Variable>(false, v);
	cacheStack_.push(cacheItem);
	nextIP();
}

void KumirVM::do_cdropz()
{
	const bool zFlag = ! register0_.toBool();
	if (zFlag) {
		cacheStack_.pop();
	}
	nextIP();
}

void KumirVM::do_cachebegin()
{
	const std::pair<bool, Variable> marker = std::pair<bool, Variable>(true, Variable());
	cacheStack_.push(marker);
	nextIP();
}

void KumirVM::do_cacheend()
{
	while (cacheStack_.size() > 0) {
		const std::pair<bool, Variable> cacheItem = cacheStack_.pop();
		if (true == cacheItem.first) {
			break;
		}
	}
	nextIP();
}

void KumirVM::do_showreg(uint8_t regNo)
{
	if (!blindMode_) {
		const int lineNo = contextsStack_.top().lineNo;
		if (lineNo != -1 &&
			!blindMode_
		) {
			const AnyValue &val = regNo == 0u
				? register0_
				: currentContext().registers[regNo];
			if (debugHandler_)
				if (contextsStack_.top().moduleContextNo == 0) {
					debugHandler_->appendTextToMargin(lineNo, val.toString());
				}
		}
	}
	nextIP();
}

void KumirVM::do_clearmarg(uint16_t toLine)
{
	const int lineNo = contextsStack_.top().lineNo;
	if (!blindMode_ && lineNo != -1
	) {
		if (debugHandler_) {
			debugHandler_->clearMargin(lineNo, toLine);
		}
	}
	nextIP();
}

void KumirVM::do_ret()
{
	stacksMutex_->lock();
	if (contextsStack_.top().runMode == CRM_UntilReturn) {
		if (debugHandler_) {
			debugHandler_->noticeOnFunctionReturn();
		}
		contextsStack_.top().runMode = CRM_OneStep;
	} else {
		lastContext_ = contextsStack_.top();
//        if (lastContext_.type != Bytecode::EL_MAIN &&
//                lastContext_.type != Bytecode::EL_TESTING)
		// Do not pop last context before program exit
		// to keep values for debugger in analysis mode
		{
			if (debugHandler_ && !blindMode_ && lastContext_.type == Bytecode::EL_FUNCTION) {
				stacksMutex_->unlock();
				debugHandler_->debuggerNoticeBeforePopContext();
				stacksMutex_->lock();
			}
			contextsStack_.pop();
			if (debugHandler_ && !blindMode_ && lastContext_.type == Bytecode::EL_FUNCTION) {
				stacksMutex_->unlock();
				debugHandler_->debuggerNoticeAfterPopContext();
				stacksMutex_->lock();
			}
		}
		if (lastContext_.type == Bytecode::EL_INIT
			&& lastContext_.runMode == CRM_OneStep
		) {
			if (contextsStack_.size() > 0) {
				contextsStack_.top().runMode = CRM_OneStep;
			}
		}
		if (contextsStack_.size() > 0) {
			nextIP();
		}
	}
	if (contextsStack_.size() > 0) {
		currentLocals_ = &(contextsStack_.top().locals);
		currentGlobals_ =
			&(moduleContexts_[contextsStack_.top().moduleContextNo].globals[contextsStack_.top().moduleId]);
		currentConstants_ =
			&(moduleContexts_[contextsStack_.top().moduleContextNo].constants);
	}
#ifndef NDEBUG
	if (cacheStack_.size() > 0) {
		error_ = Kumir::Core::fromUtf8("Cache-стек не пустой");
	}
#endif
	stacksMutex_->unlock();
}

void KumirVM::do_error(uint8_t s, uint16_t id)
{
	const int lineNo = currentContext().lineNo;
	error_ = findVariable(s, id).toString();
	if (debugHandler_ && lineNo != -1) {
		debugHandler_->setTextToMargin(lineNo, error_, true);
	}
}

void KumirVM::do_line(const Bytecode::Instruction &instr)
{
	uint32_t from = 0u, to = 0u;
	if (extractColumnPositionsFromLineInstruction(instr, from, to)) {
		currentContext().columnStart = from;
		currentContext().columnEnd = to;
		int lineNo = currentContext().lineNo;
		if (previousLineNo_ == lineNo && previousColStart_ == from && previousColEnd_ == to) {
			nextIP();
			return;
		} else {
			previousLineNo_ = lineNo;
			previousColStart_ = from;
			previousColEnd_ = to;
		}
		if (!blindMode_ && contextsStack_.top().runMode == CRM_OneStep && contextsStack_.top().moduleContextNo == 0) {
			if (debugHandler_) {
				debugHandler_->noticeOnLineChanged(currentContext().lineNo, from, to);
			}
		}
		if (currentContext().IP != -1) {
			stepsCounter_ ++;
			if (!blindMode_ && debugHandler_) {
				debugHandler_->noticeOnStepsChanged(stepsCounter_);
			} else if (blindMode_ && debugHandler_ && (stepsCounter_ % 1000 == 1)) {
				debugHandler_->noticeOnStepsChanged(stepsCounter_);
			}
		}
	} else {
		int no = instr.arg;
//        if (!blindMode_ && contextsStack_.top().runMode==CRM_OneStep) {
//            if (contextsStack_.top().lineNo!=no) {
//                if (debugHandler_)
//                    debugHandler_->noticeOnLineChanged(no, 0u, 0u);
//            }
//        }
		currentContext().lineNo = no;
		currentContext().columnStart = currentContext().columnEnd = 0u;
		if (!blindMode_ && debugHandler_) {
			const uint8_t modId = currentContext().moduleId;
			const int lineNo = currentContext().lineNo;
			if (breakpointsTable_.processBreakpointHit(modId, lineNo, nullptr)) {
				const String &sourceFileName = breakpointsTable_.registeredSourceFileName(modId);
				debugHandler_->debuggerNoticeOnBreakpointHit(sourceFileName, uint32_t(lineNo));
			}
		}
	}
	nextIP();
}

void KumirVM::do_sum()
{
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (a.baseType() == VT_int && b.baseType() == VT_int) {
		Variable r(a.toInt() + b.toInt());
		valuesStack_.push(r);
		if (!Kumir::Math::checkSumm(a.toInt(), b.toInt())) {
			error_ = Kumir::Core::fromUtf8("Целочисленное переполнение");
		}
	} else if (a.baseType() == VT_real || b.baseType() == VT_real) {
		Variable r(a.toReal() + b.toReal());
		valuesStack_.push(r);
		if (!Kumir::Math::isCorrectReal(r.toReal())) {
			error_ = Kumir::Core::fromUtf8("Вещественное переполнение");
		}
	} else if (a.baseType() == VT_string || a.baseType() == VT_char) {
		Variable r(a.toString() + b.toString());
		valuesStack_.push(r);
	}
	nextIP();
}

void KumirVM::do_sub()
{
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (a.baseType() == VT_int && b.baseType() == VT_int) {
		Variable r(a.toInt() - b.toInt());
		valuesStack_.push(r);
		if (!Kumir::Math::checkDiff(a.toInt(), b.toInt())) {
			error_ = Kumir::Core::fromUtf8("Целочисленное переполнение");
		}
	} else if (a.baseType() == VT_real || b.baseType() == VT_real) {
		Variable r(a.toReal() - b.toReal());
		valuesStack_.push(r);
		if (!Kumir::Math::isCorrectReal(r.toReal())) {
			error_ = Kumir::Core::fromUtf8("Вещественное переполнение");
		}
	}
	nextIP();
}

void KumirVM::do_mul()
{
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (a.baseType() == VT_int && b.baseType() == VT_int) {
		Variable r(a.toInt()*b.toInt());
		valuesStack_.push(r);
		if (!Kumir::Math::checkProd(a.toInt(), b.toInt())) {
			error_ = Kumir::Core::fromUtf8("Целочисленное переполнение");
		}
	} else if (a.baseType() == VT_real || b.baseType() == VT_real) {
		Variable r(a.toReal()*b.toReal());
		valuesStack_.push(r);
		if (!Kumir::Math::isCorrectReal(r.toReal())) {
			error_ = Kumir::Core::fromUtf8("Вещественное переполнение");
		}
	}
	nextIP();
}

void KumirVM::do_div()
{
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (b.baseType() == VT_int && b.toInt() == 0) {
		error_ = Kumir::Core::fromUtf8("Деление на ноль");
	} else if (b.baseType() == VT_real && b.toReal() == 0.0) {
		error_ = Kumir::Core::fromUtf8("Деление на ноль");
	} else {
		Variable r(a.toReal() / b.toReal());
		if (!Kumir::Math::isCorrectReal(r.toReal())) {
			error_ = Kumir::Core::fromUtf8("Вещественное переполнение");
		}
		valuesStack_.push(r);
	}
	nextIP();
}

void KumirVM::do_pow()
{
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	Variable r;
	if (a.baseType() == VT_int && b.baseType() == VT_int) {
		r = Variable(Kumir::Math::ipow(a.toInt(), b.toInt()));
	} else {
		r = Variable(Kumir::Math::pow(a.toReal(), b.toReal()));
	}
	valuesStack_.push(r);
	nextIP();
}

void KumirVM::do_neg()
{
	Variable a = valuesStack_.pop();
	if (a.baseType() == VT_bool) {
		Variable r(!a.toBool());
		valuesStack_.push(r);
		register0_ = AnyValue(!a.toBool());
	} else if (a.baseType() == VT_int) {
		Variable r(-a.toInt());
		valuesStack_.push(r);
	} else if (a.baseType() == VT_real) {
		Variable r(0.0 - a.toReal());
		valuesStack_.push(r);
	}
	nextIP();
}

void KumirVM::do_and()
{
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (a.baseType() == VT_bool && b.baseType() == VT_bool) {
		Variable r(a.toBool() && b.toBool());
		valuesStack_.push(r);
	}
	nextIP();
}

void KumirVM::do_or()
{
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (a.baseType() == VT_bool && b.baseType() == VT_bool) {
		Variable r(a.toBool() || b.toBool());
		valuesStack_.push(r);
	}
	nextIP();
}

void KumirVM::do_eq()
{
	bool result = false;
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (b.baseType() == VT_int && a.baseType() == VT_int) {
		result = a.toInt() == b.toInt();
	}
	if (b.baseType() == VT_real || a.baseType() == VT_real) {
		result = a.toReal() == b.toReal();;
	}
	if (b.baseType() == VT_bool && a.baseType() == VT_bool) {
		result = a.toBool() == b.toBool();
	}
	if (a.baseType() == VT_string || b.baseType() == VT_string) {
		result = a.toString() == b.toString();
	}
	if (a.baseType() == VT_char && b.baseType() == VT_char) {
		result = a.toChar() == b.toChar();
	}

	Variable r(result);
	valuesStack_.push(r);
	register0_ = AnyValue(result);
	nextIP();
}

void KumirVM::do_neq()
{
	bool result = false;
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (b.baseType() == VT_int && a.baseType() == VT_int) {
		result = a.toInt() == b.toInt();
	}
	if (b.baseType() == VT_real || a.baseType() == VT_real) {
		result = a.toReal() == b.toReal();;
	}
	if (b.baseType() == VT_bool && a.baseType() == VT_bool) {
		result = a.toBool() == b.toBool();
	}
	if (a.baseType() == VT_string || b.baseType() == VT_string) {
		result = a.toString() == b.toString();
	}
	if (a.baseType() == VT_char && b.baseType() == VT_char) {
		result = a.toChar() == b.toChar();
	}

	Variable r(!result);
	valuesStack_.push(r);
	register0_ = AnyValue(!result);
	nextIP();
}

void KumirVM::do_ls()
{
	bool result = false;
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (b.baseType() == VT_int && a.baseType() == VT_int) {
		result = a.toInt() < b.toInt();
	}
	if (b.baseType() == VT_real || a.baseType() == VT_real) {
		result = a.toReal() < b.toReal();
	}
	if (b.baseType() == VT_bool && a.baseType() == VT_bool) {
		result = a.toInt() < b.toInt();
	}
	if (a.baseType() == VT_string || b.baseType() == VT_string) {
		result = a.toString() < b.toString();
	}
	if (a.baseType() == VT_char && b.baseType() == VT_char) {
		result = a.toChar() < b.toChar();
	}
	Variable r(result);
	valuesStack_.push(r);
	register0_ = AnyValue(result);
	nextIP();
}

void KumirVM::do_gt()
{
	bool result = false;
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (b.baseType() == VT_int && a.baseType() == VT_int) {
		result = a.toInt() > b.toInt();
	}
	if (b.baseType() == VT_real || a.baseType() == VT_real) {
		result = a.toReal() > b.toReal();
	}
	if (b.baseType() == VT_bool && a.baseType() == VT_bool) {
		result = a.toInt() > b.toInt();
	}
	if (a.baseType() == VT_string || b.baseType() == VT_string) {
		result = a.toString() > b.toString();
	}
	if (a.baseType() == VT_char && b.baseType() == VT_char) {
		result = a.toChar() > b.toChar();
	}
	Variable r(result);
	valuesStack_.push(r);
	register0_ = AnyValue(result);
	nextIP();
}

void KumirVM::do_leq()
{
	bool result = false;
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (b.baseType() == VT_int && a.baseType() == VT_int) {
		result = a.toInt() <= b.toInt();
	}
	if (b.baseType() == VT_real || a.baseType() == VT_real) {
		result = a.toReal() <= b.toReal();
	}
	if (b.baseType() == VT_bool && a.baseType() == VT_bool) {
		result = a.toInt() <= b.toInt();
	}
	if (a.baseType() == VT_string || b.baseType() == VT_string) {
		result = a.toString() <= b.toString();
	}
	if (a.baseType() == VT_char && b.baseType() == VT_char) {
		result = a.toChar() <= b.toChar();
	}
	Variable r(result);
	valuesStack_.push(r);
	register0_ = AnyValue(result);
	nextIP();
}

void KumirVM::do_geq()
{
	bool result = false;
	Variable b = valuesStack_.pop();
	Variable a = valuesStack_.pop();
	if (b.baseType() == VT_int && a.baseType() == VT_int) {
		result = a.toInt() >= b.toInt();
	}
	if (b.baseType() == VT_real || a.baseType() == VT_real) {
		result = a.toReal() >= b.toReal();
	}
	if (b.baseType() == VT_bool && a.baseType() == VT_bool) {
		result = a.toInt() >= b.toInt();
	}
	if (a.baseType() == VT_string || b.baseType() == VT_string) {
		result = a.toString() >= b.toString();
	}
	if (a.baseType() == VT_char && b.baseType() == VT_char) {
		result = a.toChar() >= b.toChar();
	}
	Variable r(result);
	valuesStack_.push(r);
	register0_ = AnyValue(result);
	nextIP();
}

void KumirVM::do_ctl(uint8_t parameter, uint16_t value)
{
	if (parameter == 0x00) {
		Variable::ignoreUndefinedError = value > 0;
	} else if (parameter == 0x01) {
		backtraceSkip_ = value;
	}
	nextIP();
}

void KumirVM::do_inrange()
{
	Variable value = valuesStack_.pop();
	Variable to = valuesStack_.pop();
	Variable from = valuesStack_.pop();
	Variable step = valuesStack_.pop();

	int iValue = value.toInt();
	int iStep = step.toInt();
	int iFrom = from.toInt();
	int iTo = to.toInt();
	bool res = iStep >= 0
		? (iFrom <= iValue) && (iValue <= iTo)
		: (iFrom >= iValue) && (iValue >= iTo);
	register0_ = res;
	nextIP();
}



void KumirVM::do_pause(uint16_t)
{
	if (EP_Main == entryPoint_) {
		if (stacksMutex_) {
			stacksMutex_->lock();
		}
		ContextRunMode prevRunMode = CRM_OneStep;
		if (contextsStack_.size() > 0) {
			prevRunMode = currentContext().runMode;
			currentContext().runMode = CRM_OneStep;
		}
		blindMode_ = false;
		if (prevRunMode != CRM_OneStep) {
			if (debugHandler_) {
				debugHandler_->noticeOnLineChanged(currentContext().lineNo,
					currentContext().columnStart,
					currentContext().columnEnd);
			}
			(*pause_)();
			if (debugHandler_) {
				debugHandler_->noticeOnLineChanged(currentContext().lineNo,
					currentContext().columnStart,
					currentContext().columnEnd);
			}
		}
		if (stacksMutex_) {
			stacksMutex_->unlock();
		}
	}
	nextIP();
}

void KumirVM::do_halt(uint16_t)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	static const String STOP = Kumir::Core::fromUtf8("\nСТОП.");
	std::deque< std::pair<int, int> > formats;
	formats.push_back(std::pair<int, int>(0, 0));
	std::deque<Variable> values;
	values.push_back(Variable(STOP));
	String localError;
	(*output_)(values, formats, &localError);
	contextsStack_.reset();
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
}


void KumirVM::setNextCallInto()
{
	nextCallInto_ = true;
}

void KumirVM::setNextCallToEndOfContext()
{
	if (contextsStack_.size() == 0) {
		return;
	}
	contextsStack_.top().runMode = CRM_UntilReturn;
}

void KumirVM::setNextCallToEnd()
{
	for (int i = 0; i < contextsStack_.size(); i++) {
		contextsStack_.at(i).runMode = CRM_ToEnd;
	}
}

void KumirVM::setNextCallStepOver()
{
	if (contextsStack_.size() == 0) {
		return;
	}
	ContextRunMode prevMode = currentContext().runMode;
	currentContext().runMode = CRM_OneStep;
	if (prevMode != CRM_OneStep) {

	}
}

std::pair<uint32_t, uint32_t> KumirVM::effectiveColumn() const
{
	std::pair<uint32_t, uint32_t> result(0u, 0u);
	if (contextsStack_.size() > 0) {
		result.first = contextsStack_.top().columnStart;
		result.second = contextsStack_.top().columnEnd;
	}
	return result;
}

int KumirVM::effectiveLineNo() const
{
	if (contextsStack_.size() == 0) {
		return -1;
	}
	int index = Kumir::Math::imax(0, int(contextsStack_.size() - 1 - backtraceSkip_));
	return contextsStack_.at(index).lineNo;
}

bool KumirVM::canStepOut() const
{
	if (contextsStack_.size() == 0) {
		return false;
	} else {
		return contextsStack_.top().type == EL_FUNCTION;
	}
}

bool KumirVM::loadProgramFromBinaryBuffer(std::list<char> &stream, bool isMain, const String &filename, String &error)
{
	breakpointsTable_.reset();
	error.clear();
	if (!Bytecode::isValidSignature(stream)) {
		error = Kumir::Core::fromUtf8("Это не исполняемый файл Кумир 2.x");
		return false;
	}
	Bytecode::Data d;
	bool ok;

	Bytecode::bytecodeFromDataStream(stream, d);
	ok = true;

	setProgram(d, isMain, filename, &error);
	ok = error.length() == 0;

	return ok;
}


int KumirVM::contextByIds(int moduleId, int algorhitmId) const
{
	for (int i = contextsStack_.size() - 1; i >= 0; i--) {
		if (contextsStack_.at(i).algId == algorhitmId && contextsStack_.at(i).moduleId == moduleId) {
			return i;
		}
	}
	if (lastContext_.algId == algorhitmId && lastContext_.moduleId == moduleId) {
		return -2;
	}
	return -1;
}

const std::vector<Variable> &KumirVM::getLocals(int frameNo) const
{
	int curFrameNo = -1;
	int frameIndex = -1;
	for (int i = 0; i < contextsStack_.size(); i++) {
		if (contextsStack_.at(i).type == EL_MAIN
			|| contextsStack_.at(i).type == EL_FUNCTION) {
			curFrameNo += 1;
			if (curFrameNo == frameNo) {
				frameIndex = i;
				break;
			}
		}
	}
	const Context &c = contextsStack_.at(frameIndex);
	return c.locals;
}

const std::vector<Variable> *
KumirVM::getMainModuleGlobals() const
{
	if (moduleContexts_.size() == 0) {
		return nullptr;
	}
	const ModuleContext &context = moduleContexts_.front();
	const std::vector<Variable> *result = nullptr;
	const GlobalsMap &allGlobals = context.globals;
	for (size_t i = allGlobals.size(); i-- > 0;) {
		const Kumir::String &currentModuleName = context.moduleNames[i];
		if (currentModuleName.length() == 0) {
			const std::vector<Variable> &moduleGlobals = allGlobals.at(i);
			result = &moduleGlobals;
			break;
		}
	}
	return result;
}

std::pair< Kumir::String, const std::vector<Variable> *>
KumirVM::getLocalsAndName(size_t stackIndex) const
{
	std::pair< Kumir::String, const std::vector<Variable> *> result;
	result.second = nullptr;

	size_t counter = 0;
	for (int i = 0; i < (int)contextsStack_.size(); i++) {
		const Context &context = contextsStack_.at(i);
		if (context.type == EL_MAIN || context.type == EL_TESTING ||
			context.type == EL_FUNCTION) {
			if (counter == stackIndex) {
				result.first = context.name;
				result.second = & context.locals;
				break;
			}
			counter ++;
		}
	}
	return result;
}

size_t KumirVM::functionCallStackSize() const
{
	size_t result = 0;
	for (int i = 0; i < (int)contextsStack_.size(); i++) {
		const Context &context = contextsStack_.at(i);
		if (context.type == EL_MAIN || context.type == EL_TESTING ||
			context.type == EL_FUNCTION) {
			result ++;
		}
	}
	return result;
}

const std::vector<Variable> &
KumirVM::getGlobals(const Kumir::String &moduleName) const
{
	size_t contextNumber = 0;
	bool isFileName = false;
	if (moduleName.length() > 0) {
		// If module name is empty, then it is the main module,
		// which always first in contest. Otherwise, find corresponding
		// context
		for (size_t i_fileNo = 0; i_fileNo < moduleContexts_.size(); i_fileNo++) {
			const ModuleContext &context = moduleContexts_[i_fileNo];
			if (context.filename == moduleName) {
				isFileName = true;
				contextNumber = i_fileNo;
				break;
			}
		}
	}
	const ModuleContext &context = moduleContexts_[contextNumber];
	const GlobalsMap &allGlobals = context.globals;
	int moduleNumber = -1;
	for (int i_modNo = allGlobals.size() - 1; i_modNo >= 0; i_modNo--) {
		// Find a module in a context
		const Kumir::String &currentModuleName = context.moduleNames[i_modNo];
		if (isFileName && currentModuleName.length() == 0) {
			moduleNumber = i_modNo;
			break;
		} else if (moduleName == currentModuleName) {
			moduleNumber = i_modNo;
			break;
		}
	}
	// ASSERT(moduleNumber!=-1);
	return allGlobals.at(moduleNumber);
}


} // namespace VM

