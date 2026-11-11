#include "syntaxanalizer.h"
#include "lexer.h"
#include <limits>
#include <stack>
#include <iostream>
#include <fstream>

#include <kumir2-libs/dataformats/ast_variable.h>
#include <kumir2-libs/dataformats/ast_type.h>
#include <kumir2-libs/errormessages/errormessages.h>
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <kumir2/actorinterface.h>
#include <kumir2-libs/vm/vm_bytecode.hpp>

#include <QStack>
#include <QCoreApplication>
#include <QProcess>
#include <QDebug>

using namespace Shared;

namespace KumirAnalizer
{


SyntaxAnalizer::SyntaxAnalizer(
	Lexer *lexer,
	const QSet<QString> *alwaysEnabledModules,
	bool teacherMode,
	Analizer *parent
) :
	QObject(parent),
	lexer_(lexer),
	analizer_(parent),
	alwaysEnabledModules_(alwaysEnabledModules),
	currentPosition_(-1),
	teacherMode_(teacherMode)
{
}


void SyntaxAnalizer::init(
	QList<TextStatementPtr> &statements,
	AST::DataPtr ast
) {
	currentPosition_ = statements.size() - 1;
	ast_ = ast;
	statements_.clear();
	for (int i = 0; i < statements.size(); i++) {
		TextStatementPtr st = statements[i];
		Q_CHECK_PTR(st);
		TextStatement sst;
		sst.type = st->type;
		sst.statement = st->statement;
		sst.alg = st->alg;
		sst.mod = st->mod;
		sst.conditionalIndex = st->conditionalIndex;
		for (int j = 0; j < st->data.size(); j++) {
			LexemPtr lx = st->data[j];
			Q_CHECK_PTR(lx);
			if (lx->type != LxTypeComment) {
				sst.data << lx;
			}
		}

		statements_ << sst;
	}

	unresolvedImports_.clear();
}

QString SyntaxAnalizer::suggestFileName() const
{
	if (!ast_) {
		return QString();
	}
	AST::ModulePtr mainMod;
	for (int i = 0; i < ast_->modules.size(); i++) {
		if (ast_->modules[i]->header.type == AST::ModTypeUserMain) {
			mainMod = ast_->modules[i];
			break;
		}
	}
	if (!mainMod) {
		return QString();
	}
	AST::AlgorithmPtr mainAlg = mainMod->impl.algorhitms.size() > 0
		? mainMod->impl.algorhitms[0] : AST::AlgorithmPtr();
	if (!mainAlg || mainAlg->header.name.isEmpty()) {
		return QString();
	}

	const QString src = mainAlg->header.name;
	QString result;

	static const QString Rus = QString::fromUtf8(
			"абвгдеёжзийклмнопрстуфхцчшщьыъэюя"
		);
	static const QStringList Lat = QStringList()
		<< "a" << "b" << "v" << "g" << "d" << "e" << "yo" << "zh" << "z"
		<< "i" << "j" << "k" << "l" << "m" << "n" << "o" << "p" << "r"
		<< "s" << "t" << "u" << "f" << "h" << "ts" << "ch" << "sh"
		<< "sch" << "" << "y" << "" << "ye" << "yu" << "ya";

	Q_ASSERT(Rus.length() == Lat.size());

	for (int i = 0; i < src.size(); i++) {
		const QChar ch = src[i];
		if (ch == ' ') {
			result += "_";
		} else if (ch.toLatin1() != '\0') {
			if (ch.isLetterOrNumber()) {
				result += ch;
			}
		} else {
			int index = Rus.indexOf(ch, 0, Qt::CaseInsensitive);
			QString repl = Lat.at(index);
			if (ch.isUpper() && repl.length() > 0) {
				repl[0] = repl[0].toUpper();
			}
			result += repl;
		}
	}
	return result;
}


LexemPtr SyntaxAnalizer::findLexemByType(const QList<LexemPtr> lxs, LexemType type)
{
	foreach (LexemPtr lx, lxs) {
		if (lx->type == type) {
			return lx;
		}
	}
	return LexemPtr();
}

AST::Type typeFromSignature(QString s)
{
	AST::Type result;
	if (s.startsWith("void")) {
		result.kind = AST::TypeNone;
	} else if (s.startsWith("int")) {
		result.kind = AST::TypeInteger;
	} else if (s.startsWith("real")) {
		result.kind = AST::TypeReal;
	} else if (s.startsWith("bool")) {
		result.kind = AST::TypeBoolean;
	} else if (s.startsWith("char")) {
		result.kind = AST::TypeCharect;
	} else if (s.startsWith("string")) {
		result.kind = AST::TypeString;
	} else if (s.startsWith("record ")) {
		result.kind = AST::TypeUser;
		s.remove(0, 7);
		int br = s.indexOf("{");
		result.name = s.left(br);
		s.remove(0, br + 1);
		int lbr = s.lastIndexOf("}");
		s = s.left(lbr);
		QStringList fields = s.split(";");
		for (int i = 0; i < fields.size(); i++) {
			AST::Type fieldType = typeFromSignature(fields[i]);
			AST::Field field(fields[i], fieldType);
			result.userTypeFields.append(field);
		}
	}
	return result;
}

QList<Shared::Analizer::Suggestion>
SyntaxAnalizer::suggestImportAutoComplete(
	int lineNo,
	const TextStatementPtr,
	const QList<LexemPtr>
) const
{
	AST::ModulePtr currentModule = analizer_->findModuleByLine(lineNo);
	QList<Shared::Analizer::Suggestion> result;
	foreach (const AST::ModulePtr mod, ast_->modules) {
		if (!mod->isEnabledFor(currentModule) && mod->header.name.length() > 0) {
			Shared::Analizer::Suggestion suggestion;
			suggestion.value = mod->header.name;
			suggestion.description = tr("Built-it module %1").arg(mod->header.name);
			suggestion.kind = Shared::Analizer::Suggestion::BuiltinModule;
			result.push_back(suggestion);
		}
	}
	QDir programDir(sourceDirName_);
	QSet<QString> kumFiles;
	foreach (const QString fileName, programDir.entryList(QStringList() << "*.kum")) {
		kumFiles.insert(fileName);
		Shared::Analizer::Suggestion suggestion;
		suggestion.value = "\"" + fileName + "\"";
		suggestion.description = tr("Use file \"%1\" as module").arg(fileName);
		suggestion.kind = Shared::Analizer::Suggestion::Module;
		result.push_back(suggestion);
	}
	foreach (const QString fileName, programDir.entryList(QStringList() << "*.kod")) {
		const QString kumFileName = fileName.left(fileName.length() - 4) + ".kum";
		if (!kumFiles.contains(kumFileName)) {
			Shared::Analizer::Suggestion suggestion;
			suggestion.value = "\"" + fileName + "\"";
			suggestion.description = tr("Use precompiled file \"%1\" as module").arg(fileName);
			suggestion.kind = Shared::Analizer::Suggestion::Module;
			result.push_back(suggestion);
		}
	}
	return result;
}

QList<Shared::Analizer::Suggestion>
SyntaxAnalizer::suggestInputOutputAutoComplete(
	int lineNo,
	const TextStatementPtr statementBefore,
	const QList<LexemPtr> lexemsAfter,
	const AST::ModulePtr contextModule,
	const AST::AlgorithmPtr contextAlgorithm) const
{
	AST::ModulePtr currentModule = analizer_->findModuleByLine(lineNo);
	QList<Shared::Analizer::Suggestion> result;
	const QList<LexemPtr> lexemsBefore = statementBefore->data;
	int curBlockStartPos = 1;
	int curBlockIndex = 0;
	int sqBrDeep = 0;
	int brDeep = 0;
	for (int i = curBlockStartPos; i < lexemsBefore.size(); i++) {
		LexemPtr lx = lexemsBefore[i];
		if (lx->type == LxOperComa && sqBrDeep == 0 && brDeep == 0) {
			curBlockStartPos = i + 1;
			curBlockIndex += 1;
		} else if (lx->type == LxOperRightBr) {
			brDeep -= 1;
		} else if (lx->type == LxOperRightSqBr) {
			sqBrDeep -= 1;
		} else if (lx->type == LxOperLeftBr && brDeep > 0) {
			brDeep += 1;
		} else if (lx->type == LxOperLeftSqBr && sqBrDeep > 0) {
			sqBrDeep += 1;
		}

	}
	const QList<LexemPtr> curBlock = curBlockStartPos < lexemsBefore.size()
		? lexemsBefore.mid(curBlockStartPos)
		: QList<LexemPtr>();

	if (curBlockIndex == 0) {
		// Check for passing file variable
		bool filesModuleAvailable = false;
		AST::ModulePtr filesMod;
		foreach (const AST::ModulePtr mod, ast_->modules) {
			if (mod->isEnabledFor(currentModule) && mod->header.name == QString::fromUtf8("Файлы")) {
				filesModuleAvailable = true;
				filesMod = mod;
				break;
			}
		}
		if (filesModuleAvailable) {
			static const AST::Type FileType = AST::Type(QString::fromUtf8("файл"), QByteArray("file"));
			QList<Shared::Analizer::Suggestion> fileSuggestions =
				suggestExpressionAutoComplete(
					lineNo,
					/* lexemsBefore    = */ curBlock,
					/* lexemsAfter     = */ lexemsAfter,
					/* contextModule   = */ contextModule,
					/* contextAlgorithm= */ contextAlgorithm,
					/* typeIsKnown     = */ true,
					/* baseType        = */ FileType,
					/* dimension       = */ 0,
					/* accessType      = */ AST::AccessArgumentIn,
					/* expressionKind  = */ AST::ExprNone
				);
			foreach (const Shared::Analizer::Suggestion &s, fileSuggestions) {
				// Filter values not to use built-in functions retvals
				// to avoid unclosed files
				const QString &name = s.value;
				if (s.kind == Shared::Analizer::Suggestion::Function) {
					bool deny = false;
					foreach (const AST::AlgorithmPtr alg, filesMod->header.algorhitms) {
						if (alg->header.name == name) {
							deny = true;
							break;
						}
					}
					if (!deny) {
						result.push_back(s);
					}
				} else {
					result.push_back(s);
				}
			} // end file filtering
		} // end if (filesModuleAvailable)
	} // end if (curBlockIndex==0)

	const AST::VariableAccessType accessType = statementBefore->type == LxPriInput
		? AST::AccessArgumentOut : AST::AccessArgumentIn;
	QList<Shared::Analizer::Suggestion> valueSuggestions =
		suggestExpressionAutoComplete(
			lineNo,
			/* lexemsBefore    = */ curBlock,
			/* lexemsAfter     = */ lexemsAfter,
			/* contextModule   = */ contextModule,
			/* contextAlgorithm= */ contextAlgorithm,
			/* typeIsKnown     = */ false,
			/* baseType        = */ AST::TypeNone,
			/* dimension       = */ 0,
			/* accessType      = */ accessType,
			/* expressionKind  = */ AST::ExprNone
		);
	foreach (const Shared::Analizer::Suggestion &s, valueSuggestions) {
		// Filter values to match conditions:
		//   a) can't input algorithms
		//   b) can't input/output values of custom type
		if (s.kind == Shared::Analizer::Suggestion::LocalVariable || s.kind == Shared::Analizer::Suggestion::GlobalVariable
			|| accessType == AST::AccessArgumentIn
		) {
			const QString &name = s.value;
			AST::VariablePtr var;
			AST::AlgorithmPtr alg;
			AST::ModulePtr mmod;
			AST::Type type;
			QVariantList templateParameters;
			if (findVariable(name, contextModule, contextAlgorithm, var)) {
				type = var->baseType;
			} else if (findAlgorithm(name, contextModule, contextAlgorithm, mmod, alg, templateParameters)) {
				type = alg->header.returnType;
			}
			if (type.kind != AST::TypeUser && type.kind != AST::TypeNone) {
				result.push_back(s);
			}

		}
	}

	return result;
}

QList<Shared::Analizer::Suggestion>
SyntaxAnalizer::suggestConditionAutoComplete(
	int lineNo,
	const TextStatementPtr statementBefore,
	const QList<LexemPtr> lexemsAfter,
	const AST::ModulePtr contextModule,
	const AST::AlgorithmPtr contextAlgorithm) const
{
	QList<Shared::Analizer::Suggestion> result;
	int start = (statementBefore->type == LxPriEndLoop || statementBefore->type == LxPriLoop)
		? 2 : 1;
	const QList<LexemPtr> block = statementBefore->data.size() > start
		? statementBefore->data.mid(start) : QList<LexemPtr>();
	result = suggestExpressionAutoComplete(
			lineNo,
			/* lexemsBefore    = */ block,
			/* lexemsAfter     = */ lexemsAfter,
			/* contextModule   = */ contextModule,
			/* contextAlgorithm= */ contextAlgorithm,
			/* typeIsKnown     = */ true,
			/* baseType        = */ AST::TypeBoolean,
			/* dimension       = */ 0,
			/* accessType      = */ AST::AccessArgumentIn,
			/* expressionKind  = */ AST::ExprNone
		);
	return result;
}

QList<Shared::Analizer::Suggestion>
SyntaxAnalizer::suggestLoopBeginAutoComplete(
	int lineNo,
	const TextStatementPtr statementBefore,
	const QList<LexemPtr> lexemsAfter,
	const AST::ModulePtr contextModule,
	const AST::AlgorithmPtr contextAlgorithm) const
{
	QList<Shared::Analizer::Suggestion> result;
	bool secondLexemIncomplete = statementBefore->data.size() == 1;
	if (statementBefore->data.size() == 2) {
		LexemPtr lx = statementBefore->data.at(1);
		if (lx->type != LxSecFor && lx->type != LxSecWhile) {
			AST::ExpressionPtr  ex = parseExpression(QList<LexemPtr>() << lx,
					contextModule,
					contextAlgorithm);
			// try to parse integer ... times
			if (!ex || ex->baseType.kind != AST::TypeInteger) {
				secondLexemIncomplete = true;
			}
		}
	}
	if (secondLexemIncomplete) {
		// 1. Suggest additional keywords for loop types
		Shared::Analizer::Suggestion s;
		s.value = QString::fromUtf8(" для ");
		s.description = tr("Loop for variable");
		s.kind = Shared::Analizer::Suggestion::SecondaryKeyword;
		result.push_back(s);

		s.value = QString::fromUtf8(" пока ");
		s.description = tr("Repeat while condition is true");
		result.push_back(s);

		const QList<LexemPtr> block = statementBefore->data.size() > 1
			? statementBefore->data.mid(2)
			: QList<LexemPtr>();

		// 2. Suggest integer values followed by 'times' keyword
		QList<Shared::Analizer::Suggestion> intvals = suggestExpressionAutoComplete(
				lineNo,
				/* lexemsBefore    = */ block,
				/* lexemsAfter     = */ lexemsAfter,
				/* contextModule   = */ contextModule,
				/* contextAlgorithm= */ contextAlgorithm,
				/* typeIsKnown     = */ true,
				/* baseType        = */ AST::TypeInteger,
				/* dimension       = */ 0,
				/* accessType      = */ AST::AccessArgumentIn,
				/* expressionKind  = */ AST::ExprNone
			);
		for (int i_intvals = 0; i_intvals < intvals.size(); i_intvals++) {
			Shared::Analizer::Suggestion ss = intvals.at(i_intvals);
			ss.value += QString::fromUtf8(" раз");
			result.push_back(ss);
		}
	} else if (statementBefore->data.size() >= 2 && statementBefore->data[1]->type == LxSecFor) {
		// Suggest values for 'from' or 'to'.
		// If no such keywords, then suggest keywords
		LexemPtr lastKeywordLexem;
		for (int i = statementBefore->data.size() - 1; i > 0; i--) {
			LexemPtr lx = statementBefore->data[i];
			if (lx->type == LxSecFrom) {
				if (!lastKeywordLexem) {
					lastKeywordLexem = lx;
				}
			} else if (lx->type == LxSecTo) {
				if (!lastKeywordLexem) {
					lastKeywordLexem = lx;
				}
			} else if (lx->type == LxSecStep) {
				if (!lastKeywordLexem) {
					lastKeywordLexem = lx;
				}
			} else if (lx->type == LxSecFor) {
				if (!lastKeywordLexem) {
					lastKeywordLexem = lx;
				}
			}
		}
		QList<LexemPtr> block;
		Q_ASSERT(lastKeywordLexem != nullptr);
		if (lastKeywordLexem->type == LxSecFor) {
			// Suggest variables applicable to 'for' statement
			if (statementBefore->data.size() > 2) {
				block = statementBefore->data.mid(2);
			}
			bool hasCompleteForVariable = false;
			if (block.size() > 0) {
				// First check if there is complete for variable
				QString varName;
				foreach (LexemPtr lx, block) {
					varName += " " + lx->data;
				}
				varName = varName.simplified();
				AST::VariablePtr  var;
				if (findVariable(varName, contextModule, contextAlgorithm, var)) {
					if (var->baseType.kind == AST::TypeInteger
						&&
						(var->accessType == AST::AccessRegular || var->accessType == AST::AccessArgumentInOut)
					) {
						hasCompleteForVariable = true;
					}
				}
			}
			if (hasCompleteForVariable) {
				// Suggest only 'from' keyword
				Shared::Analizer::Suggestion s;
				s.kind = Shared::Analizer::Suggestion::SecondaryKeyword;
				s.value = QString::fromUtf8(" от ");
				result.push_back(s);
			} else {
				QList<Shared::Analizer::Suggestion> forvalues = suggestExpressionAutoComplete(
						lineNo,
						/* lexemsBefore    = */ QList<LexemPtr>(),
						/* lexemsAfter     = */ lexemsAfter,
						/* contextModule   = */ contextModule,
						/* contextAlgorithm= */ contextAlgorithm,
						/* typeIsKnown     = */ true,
						/* baseType        = */ AST::TypeInteger,
						/* dimension       = */ 0,
						/* accessType      = */ AST::AccessRegular,
						/* expressionKind  = */ AST::ExprNone
					) + suggestExpressionAutoComplete(
						lineNo,
						/* lexemsBefore    = */ QList<LexemPtr>(),
						/* lexemsAfter     = */ lexemsAfter,
						/* contextModule   = */ contextModule,
						/* contextAlgorithm= */ contextAlgorithm,
						/* typeIsKnown     = */ true,
						/* baseType        = */ AST::TypeInteger,
						/* dimension       = */ 0,
						/* accessType      = */ AST::AccessArgumentInOut,
						/* expressionKind  = */ AST::ExprNone
					);
				foreach (Shared::Analizer::Suggestion ss, forvalues) {
					// Filter values to match one of conditions:
					//   a) local/global variable; or
					//   b) in/out parameter
					if (ss.kind == Shared::Analizer::Suggestion::GlobalVariable) {
						ss.value += QString::fromUtf8(" от ");
						result.push_back(ss);
					} else if (ss.kind == Shared::Analizer::Suggestion::LocalVariable) {
						AST::VariablePtr  var;
						if (findVariable(ss.value, contextModule, contextAlgorithm, var)) {
							if (var->accessType == AST::AccessRegular || var->accessType == AST::AccessArgumentInOut) {
								ss.value += QString::fromUtf8(" от ");
								result.push_back(ss);
							}
						}
					}
				} // end for (Shared::Analizer::Suggestion & ss : forvalues)
			} // end if (hasCompleteForVariable) { ... } else { ... }
		} else {
			// Suggest any integer value for 'from', 'to' or 'step'
			int lastKeywordPos = statementBefore->data.lastIndexOf(lastKeywordLexem);
			if (lastKeywordPos < statementBefore->data.size() - 1) {
				block = statementBefore->data.mid(lastKeywordPos + 1);
			}
			bool hasCompleteIntegerExpression = false;
			if (block.size() > 0) {
				// First check if there is complete integer value
				AST::ExpressionPtr  expr = parseExpression(block, contextModule, contextAlgorithm);
				if (expr) {
					if (expr->baseType.kind == AST::TypeInteger) {
						hasCompleteIntegerExpression = true;
					}
				}
			}
			if (hasCompleteIntegerExpression) {
				// Append next for-statement keyword
				Shared::Analizer::Suggestion ss;
				ss.kind = Shared::Analizer::Suggestion::SecondaryKeyword;
				if (lastKeywordLexem->type == LxSecFrom) {
					ss.value = QString::fromUtf8(" до ");
					result.push_back(ss);
				} else if (lastKeywordLexem->type == LxSecTo) {
					ss.value = QString::fromUtf8(" шаг ");
					result.push_back(ss);
				}
			} else {
				// Suggest integer values
				QList<Shared::Analizer::Suggestion> intvalues = suggestExpressionAutoComplete(
						lineNo,
						/* lexemsBefore    = */ QList<LexemPtr>(),
						/* lexemsAfter     = */ lexemsAfter,
						/* contextModule   = */ contextModule,
						/* contextAlgorithm= */ contextAlgorithm,
						/* typeIsKnown     = */ true,
						/* baseType        = */ AST::TypeInteger,
						/* dimension       = */ 0,
						/* accessType      = */ AST::AccessArgumentIn,
						/* expressionKind  = */ AST::ExprNone
					);
				foreach (Shared::Analizer::Suggestion ss, intvalues) {
					if (lastKeywordLexem->type == LxSecFrom) {
						ss.value += QString::fromUtf8(" до ");
					}
					result.push_back(ss);
				}
			}
		}
	}
	return result;
}

QList<Shared::Analizer::Suggestion>
SyntaxAnalizer::suggestAssignmentAutoComplete(
	int lineNo,
	const TextStatementPtr statementBefore,
	const QList<LexemPtr> lexemsAfter,
	const AST::ModulePtr contextModule,
	const AST::AlgorithmPtr contextAlgorithm) const
{
	QList<Shared::Analizer::Suggestion> result;
	QList<LexemPtr> lvalue, rvalue;
	LexemPtr assignOperator;
	if (statementBefore) {
		for (QList<LexemPtr>::Iterator it = statementBefore->data.begin(); it != statementBefore->data.end(); ++it) {
			LexemPtr lx = *it;
			if (lx->type == LxPriAssign) {
				assignOperator = lx;
			} else if (assignOperator) {
				rvalue.push_back(lx);
			} else {
				lvalue.push_back(lx);
			}
		}
	}
	AST::ExpressionPtr  leftExpr;
	if (assignOperator) {
		// Make a tree for a left value and deduce it's type
		leftExpr = parseExpression(lvalue, contextModule, contextAlgorithm);
		if (leftExpr) {
			// Make suggestion only if left if correct
			result = suggestExpressionAutoComplete(
					lineNo,
					/* lexemsBefore     = */ rvalue,
					/* lexemsAfter      = */ lexemsAfter,
					/* contextModule    = */ contextModule,
					/* contextAlgorithm = */ contextAlgorithm,
					/* typeIsKnown      = */ true,
					/* baseType         = */ leftExpr->baseType,
					/* dimension        = */ leftExpr->dimension,
					/* accessType       = */ leftExpr->variable->accessType,
					/* expressionKind   = */ AST::ExprSubexpression
				);
		}
	} else {
		// Make suggestion for void algorithm call
		result = suggestExpressionAutoComplete(
				lineNo,
				/* lexemsBefore    = */ lvalue,
				/* lexemsAfter     = */ lexemsAfter,
				/* contextModule   = */ contextModule,
				/* contextAlgorithm= */ contextAlgorithm,
				/* typeIsKnown     = */ true,
				/* baseType        = */ AST::TypeNone,
				/* dimension       = */ 0,
				/* accessType      = */ AST::AccessRegular,
				/* expressionKind  = */ AST::ExprNone
			);
		// Make suggestion for variable assignment
		result += suggestExpressionAutoComplete(
				lineNo,
				/* lexemsBefore    = */ lvalue,
				/* lexemsAfter     = */ lexemsAfter,
				/* contextModule   = */ contextModule,
				/* contextAlgorithm= */ contextAlgorithm,
				/* typeIsKnown     = */ false,
				/* baseType        = */ AST::TypeNone,
				/* dimension       = */ 0,
				/* accessType      = */ AST::AccessArgumentOut,
				/* expressionKind  = */ AST::ExprNone
			);
	}
	return result;
}

#define IS_OPERATOR(x) (x & LxTypeOperator || x==LxOperGreaterOrEqual || x==LxSecOr || x==LxSecAnd || x==LxPriAssign/* || x==LxSecNot */)

QList<Shared::Analizer::Suggestion> SyntaxAnalizer::suggestExpressionAutoComplete(
	int lineNo,
	const QList<LexemPtr> lexemsBefore,
	const QList<LexemPtr> lexemsAfter,
	const AST::ModulePtr contextModule,
	const AST::AlgorithmPtr contextAlgorithm,
	bool typeIsKnown,
	AST::Type baseType,
	unsigned int dimension,
	AST::VariableAccessType accessType,
	AST::ExpressionType expressionKind
) const
{
	QList<Shared::Analizer::Suggestion> result;
	AST::ModulePtr currentModule = analizer_->findModuleByLine(lineNo);

	LexemPtr prevOper;

	QList<LexemPtr> prevBlock;
	int prevPos = lexemsBefore.size() - 1;

	while (prevPos >= 0) {
		if (IS_OPERATOR(lexemsBefore[prevPos]->type)) {
			prevOper = lexemsBefore[prevPos];
			break;
		}
		prevBlock.push_front(lexemsBefore[prevPos]);
		prevPos -= 1;
	}

	if (prevOper
		&& (prevOper->type == LxOperComa
			|| prevOper->type == LxOperColon
			|| prevOper->type == LxOperLeftBr
			|| prevOper->type == LxOperLeftSqBr)
	) {
		// An array index or algorithm argument.
		// Find unpaired [ or ( before to have more information
		int comasBefore = 0;
		int unpairedBracketPos = -1;
		int sqBrDeep = 0;
		int brDeep = 0;
		for (int i = prevPos; i >= 0; i--) {
			LexemPtr lx = lexemsBefore[i];
			if (lx->type == LxOperComa && sqBrDeep == 0 && brDeep == 0) {
				comasBefore += 1;
			} else if (lx->type == LxOperRightBr) {
				brDeep += 1;
			} else if (lx->type == LxOperRightSqBr) {
				sqBrDeep += 1;
			} else if (lx->type == LxOperLeftBr && brDeep > 0) {
				brDeep -= 1;
			} else if (lx->type == LxOperLeftSqBr && sqBrDeep > 0) {
				sqBrDeep -= 1;
			} else if (lx->type == LxOperLeftBr && brDeep == 0) {
				unpairedBracketPos = i;
				break;
			} else if (lx->type == LxOperLeftSqBr && sqBrDeep == 0) {
				unpairedBracketPos = i;
				break;
			}
		}
		if (unpairedBracketPos != -1) {
			if (lexemsBefore[unpairedBracketPos]->type == LxOperLeftSqBr) {
				// Suggest an integer value as of array index
				result = suggestValueAutoComplete(
						lineNo,
						/* lexemsBefore    = */ prevBlock,
						/* lexemsAfter     = */ lexemsAfter,
						/* contextModule   = */ contextModule,
						/* contextAlgorithm= */ contextAlgorithm,
						/* typeIsKnown     = */ true,
						/* baseType        = */ AST::Type(AST::TypeInteger),
						/* minimumDimension= */ 0,
						/* accessType      = */ AST::AccessArgumentIn
					);
			} else if (lexemsBefore[unpairedBracketPos]->type == LxOperLeftBr) {
				// Find an algorithm to rely on it's argument type
				QString algorithmName;
				for (int i = unpairedBracketPos - 1; i >= 0; i--) {
					LexemPtr lx = lexemsBefore[i];
					if (IS_OPERATOR(lx->type) && lx->type != LxSecNot) {
						break;
					}
					if (lx->type != LxSecNot) {
						algorithmName.prepend(lx->data + " ");
					}
				}
				algorithmName = algorithmName.simplified();
				AST::AlgorithmPtr alg;
				AST::ModulePtr mmod;
				QVariantList templateParameters;
				if (findAlgorithm(algorithmName, contextModule, contextAlgorithm, mmod, alg, templateParameters)) {
					int argumentIndex = comasBefore;
					if (argumentIndex < alg->header.arguments.size()) {
						// Suggest a corresponding argument type value
						AST::VariablePtr argument = alg->header.arguments[argumentIndex];
						result = suggestValueAutoComplete(
								lineNo,
								/* lexemsBefore    = */ prevBlock,
								/* lexemsAfter     = */ lexemsAfter,
								/* contextModule   = */ contextModule,
								/* contextAlgorithm= */ contextAlgorithm,
								/* typeIsKnown     = */ true,
								/* baseType        = */ argument->baseType,
								/* minimumDimension= */ argument->dimension,
								/* accessType      = */ argument->accessType
							);
					}
				}
			}
		}
	} // end if { coma, left bracket, colon }
	else if (prevOper && (
			prevOper->type == LxOperAsterisk
			|| prevOper->type == LxOperPower
			|| prevOper->type == LxOperPlus
			|| prevOper->type == LxOperMinus
			|| prevOper->type == LxOperEqual
			|| prevOper->type == LxOperNotEqual
			|| prevOper->type == LxOperLess
			|| prevOper->type == LxOperLessOrEqual
			|| prevOper->type == LxOperGreater
			|| prevOper->type == LxOperGreaterOrEqual
			|| prevOper->type == LxSecAnd
			|| prevOper->type == LxSecOr
			|| prevOper->type == LxSecNot
		)
	) {
		// Sugest a correct type operand to match operator
		QList<LexemPtr> previousSubExpressionLexems;
		if (prevPos > 0) {
			previousSubExpressionLexems = lexemsBefore.mid(0, prevPos);
		}
		AST::ExpressionPtr  previousSubExpression;
		if (previousSubExpressionLexems.size() > 0)
			previousSubExpression = parseExpression(
					/* lexems = */ previousSubExpressionLexems,
					/* mod    = */ contextModule,
					/* alg    = */ contextAlgorithm
				);
		if (previousSubExpression || previousSubExpressionLexems.size() == 0) {
			result = suggestOperandAutoComplete(
					lineNo,
					/* lexemsBefore    = */ prevBlock,
					/* lexemsAfter     = */ lexemsAfter,
					/* contextModule   = */ contextModule,
					/* contextAlgorithm= */ contextAlgorithm,
					/* leftExpression  = */ previousSubExpression,
					/* operatorr       = */ prevOper->type,
					/* targetBaseType  = */ typeIsKnown ? baseType : AST::TypeNone
				);
		}
	} // end if (expression operator)
	else if (prevPos < 0) {
		// Make a suggestion from clear space.
		// Suggest everything capable
		result = suggestValueAutoComplete(
				lineNo,
				/* lexemsBefore    = */ prevBlock,
				/* lexemsAfter     = */ lexemsAfter,
				/* contextModule   = */ contextModule,
				/* contextAlgorithm= */ contextAlgorithm,
				/* typeIsKnown     = */ typeIsKnown,
				/* baseType        = */ typeIsKnown ? baseType : AST::TypeNone,
				/* minimumDimension= */ typeIsKnown ? dimension : 0,
				/* accessType      = */ accessType
			);
		if (typeIsKnown && baseType.kind == AST::TypeBoolean) {
			// Suggest something to bool-calculable
			result += suggestValueAutoComplete(
					lineNo,
					/* lexemsBefore    = */ prevBlock,
					/* lexemsAfter     = */ lexemsAfter,
					/* contextModule   = */ contextModule,
					/* contextAlgorithm= */ contextAlgorithm,
					/* typeIsKnown     = */ false,
					/* baseType        = */ AST::TypeNone,
					/* minimumDimension= */ typeIsKnown ? dimension : 0,
					/* accessType      = */ accessType
				);
		}
	}
	return result;
}

static bool isSuggestionValueApplicable(
	const AST::VariablePtr  var,
	AST::Type baseType,
	unsigned int minimumDimension,
	AST::VariableAccessType accessType
)
{
	bool typeMatch = var->baseType == baseType;
	if (accessType == AST::AccessArgumentIn || accessType == AST::AccessRegular) {
		// It is possible to cast int->real and char->string
		if (baseType.kind == AST::TypeReal && var->baseType.kind == AST::TypeInteger) {
			typeMatch = true;
		}
		if (baseType.kind == AST::TypeString && var->baseType.kind == AST::TypeString) {
			typeMatch = true;
		}
	}
	bool dimensionMatch = minimumDimension == (unsigned int)(var->dimension);
	if (accessType == AST::AccessArgumentIn || accessType == AST::AccessRegular) {
		dimensionMatch = minimumDimension <= (unsigned int)(var->dimension);
	}
	if (baseType.kind == AST::TypeNone) {
		// It is possible to suggest value of any type and dimension
		typeMatch = true;
		dimensionMatch = true;
	}
	bool accessMatch = (
			(accessType == AST::AccessArgumentIn || accessType == AST::AccessRegular)
			&& var->accessType != AST::AccessArgumentOut
		)
		||
		(accessType == AST::AccessArgumentOut && var->accessType != AST::AccessArgumentIn)
		||
		(accessType == var->accessType);
	return typeMatch && dimensionMatch && accessMatch;
}

QList<Shared::Analizer::Suggestion>
SyntaxAnalizer::suggestValueAutoComplete(
	int lineNo,
	const QList<LexemPtr> lexemsBefore,
	const QList<LexemPtr> lexemsAfter,
	const AST::ModulePtr contextModule,
	const AST::AlgorithmPtr contextAlgorithm,
	bool typeIsKnown,
	AST::Type baseType,
	unsigned int minimumDimension,
	AST::VariableAccessType accessType
) const
{
	QList<Shared::Analizer::Suggestion> result;
	AST::ModulePtr currentModule = analizer_->findModuleByLine(lineNo);

	// 2. Suggest algorithms if any applicable
	if (minimumDimension == 0
		&& (
			baseType.kind == AST::TypeNone
			|| accessType == AST::AccessArgumentIn
			|| accessType == AST::AccessRegular
		)
	) {
		QList<AST::AlgorithmPtr> algs;
		if (contextModule) {
			algs += contextModule->impl.algorhitms;
		}
		QList<AST::ModulePtr> enabledModules;
		std::copy_if(ast_->modules.begin(),
			ast_->modules.end(),
			std::back_inserter(enabledModules),
		[this, currentModule](AST::ModulePtr p) {
			return p->isEnabledFor(currentModule) ||
				alwaysEnabledModules_->contains(p->header.name);
		});
		std::for_each(enabledModules.begin(), enabledModules.end(),
		[&algs](AST::ModulePtr p) {
			std::list<AST::AlgorithmPtr> modulePublicAlgorithms =
				p->header.algorhitms.toStdList();
			std::copy(modulePublicAlgorithms.begin(), modulePublicAlgorithms.end(),
				std::back_inserter(algs));
		});

		// Filter algs list by return type matching
		for (QList<AST::AlgorithmPtr>::iterator it = algs.begin(); it != algs.end();) {
			AST::AlgorithmPtr alg = *it;
			bool typeMatch = alg->header.returnType == baseType;
			if (accessType == AST::AccessRegular || accessType == AST::AccessArgumentIn) {
				// It is possible to cast int->real and char->string
				if (alg->header.returnType.kind == AST::TypeInteger && baseType.kind == AST::TypeReal) {
					typeMatch = true;
				}
				if (alg->header.returnType.kind == AST::TypeCharect && baseType.kind == AST::TypeString) {
					typeMatch = true;
				}
			}
			if (!typeIsKnown) {
				typeMatch = alg->header.returnType.kind != AST::TypeNone;
			}

			if (!typeMatch) {
				it = algs.erase(it);
			} else {
				++it;
			}
		}

		// Filter algs list by access type
		for (QList<AST::AlgorithmPtr>::iterator it = algs.begin(); it != algs.end();) {
			bool accessMatch = false;
			// Can pass an algorithm here
			if (typeIsKnown && baseType.kind == AST::TypeNone) {
				// It is always possible to call an algorithm from start of line
				accessMatch = true;
			} else {
				// Algorithms can be passed only by value, but not reference
				accessMatch = accessType == AST::AccessRegular || accessType == AST::AccessArgumentIn;
			}

			if (!accessMatch) {
				it = algs.erase(it);
			} else {
				++it;
			}
		}

		// Filter algs list by name
		const bool allowTeacherAlgorithms =
			AST::ModTypeTeacher == currentModule->header.type ||
			AST::ModTypeTeacherMain == currentModule->header.type;

		for (QList<AST::AlgorithmPtr>::iterator it = algs.begin(); it != algs.end();) {
			AST::AlgorithmPtr alg = *it;
			const QString &name = alg->header.name;
			if (name.isEmpty() || (name.startsWith("@") && !allowTeacherAlgorithms)) {
				it = algs.erase(it);
			} else {
				++it;
			}
		}

		// Create suggestions list
		std::transform(algs.begin(), algs.end(), std::back_inserter(result),
		[this](AST::AlgorithmPtr alg)->Shared::Analizer::Suggestion {
			Shared::Analizer::Suggestion suggestion;
			suggestion.value = alg->header.name;
			suggestion.description = QString::fromUtf8("алг ");
			suggestion.description += alg->header.returnType.kind == AST::TypeUser
			? alg->header.returnType.name
			: lexer_->classNameByBaseType(alg->header.returnType.kind);
			if (!suggestion.description.endsWith(' '))
				suggestion.description += " ";
			suggestion.description += alg->header.name;
			if (alg->header.arguments.size() > 0)
			{
				suggestion.description += "(";
				for (int i = 0; i < alg->header.arguments.size(); i++) {
					const AST::VariablePtr  arg = alg->header.arguments[i];
					suggestion.description += arg->name;
					if (i > 0) {
						suggestion.description += ",";
					}
				}
				suggestion.description += ")";
			}
			suggestion.kind = Shared::Analizer::Suggestion::Function;
			return suggestion;
		});
	}

	// 1. Suggest locals and globals if any applicable
	if (!typeIsKnown || baseType.kind != AST::TypeNone) {
		QList<AST::VariablePtr> vars;
		if (contextAlgorithm) {
			vars += contextAlgorithm->impl.locals;
		}
		if (contextModule) {
			vars += contextModule->impl.globals;
		}

		foreach (AST::VariablePtr  var, vars) {
			if (isSuggestionValueApplicable(var, baseType, minimumDimension, accessType)) {
				Shared::Analizer::Suggestion suggestion;
				if (contextAlgorithm
					&& contextAlgorithm->impl.locals.contains(var)
					&& contextAlgorithm->header.name == var->name
				) {
					// return-value
					suggestion.value = QString::fromUtf8("знач");
					suggestion.kind = Shared::Analizer::Suggestion::LocalVariable;
					suggestion.description = var->baseType.kind == AST::TypeUser
						? var->baseType.name
						: lexer_->classNameByBaseType(var->baseType.kind);
					suggestion.description += " " + suggestion.value;
				} else {
					// local or global variable
					suggestion.value = var->name;
					suggestion.description = var->baseType.kind == AST::TypeUser
						? var->baseType.name
						: lexer_->classNameByBaseType(var->baseType.kind);
					suggestion.description += " " + var->name;
					suggestion.kind = contextModule->impl.globals.contains(var)
						? Shared::Analizer::Suggestion::GlobalVariable
						: Shared::Analizer::Suggestion::LocalVariable;
				}
				result.push_back(suggestion);
			}
		}
	}

	return result;
}

QList<Shared::Analizer::Suggestion>
SyntaxAnalizer::suggestOperandAutoComplete(
	int lineNo,
	const QList<LexemPtr> lexemsBefore,
	const QList<LexemPtr> lexemsAfter,
	const AST::ModulePtr contextModule,
	const AST::AlgorithmPtr contextAlgorithm,
	const AST::ExpressionPtr leftExpression,
	const LexemType operatorr,
	const AST::Type &targetBaseType
) const
{
	QList<Shared::Analizer::Suggestion> result;
	AST::Type baseType = AST::TypeNone;
	if (leftExpression) {
		baseType = leftExpression->baseType;
	}

	if ((operatorr == LxOperPlus || operatorr == LxOperMinus) && leftExpression == nullptr) {
		// Unary + or -
		if (targetBaseType.kind == AST::TypeReal || targetBaseType.kind == AST::TypeInteger) {
			result  = suggestValueAutoComplete(
					lineNo,
					/* lexemsBefore    = */ lexemsBefore,
					/* lexemsAfter     = */ lexemsAfter,
					/* contextModule   = */ contextModule,
					/* contextAlgorithm= */ contextAlgorithm,
					/* typeIsKnown     = */ true,
					/* baseType        = */ targetBaseType,
					/* minimumDimension= */ 0,
					/* accessType      = */ AST::AccessArgumentIn
				);
		}
	} else if (operatorr == LxSecNot && leftExpression == nullptr) {
		// Unary 'not'
		result  = suggestValueAutoComplete(
				lineNo,
				/* lexemsBefore    = */ lexemsBefore,
				/* lexemsAfter     = */ lexemsAfter,
				/* contextModule   = */ contextModule,
				/* contextAlgorithm= */ contextAlgorithm,
				/* typeIsKnown     = */ true,
				/* baseType        = */ AST::TypeBoolean,
				/* minimumDimension= */ 0,
				/* accessType      = */ AST::AccessArgumentIn
			);
	} else if (operatorr == LxOperPlus) {
		// Binary +
		if (targetBaseType.kind == AST::TypeString
			|| targetBaseType.kind == AST::TypeReal
			|| targetBaseType.kind == AST::TypeInteger
		) {
			result  = suggestValueAutoComplete(
					lineNo,
					/* lexemsBefore    = */ lexemsBefore,
					/* lexemsAfter     = */ lexemsAfter,
					/* contextModule   = */ contextModule,
					/* contextAlgorithm= */ contextAlgorithm,
					/* typeIsKnown     = */ true,
					/* baseType        = */ targetBaseType,
					/* minimumDimension= */ 0,
					/* accessType      = */ AST::AccessArgumentIn
				);
		}
	} else if ((operatorr == LxOperMinus || operatorr == LxOperAsterisk || operatorr == LxOperPower)
		&& leftExpression != nullptr) {
		// Binary -, * or **
		// Applicable to integers and reals
		if (targetBaseType.kind == AST::TypeInteger || targetBaseType.kind == AST::TypeReal) {
			result  = suggestValueAutoComplete(
					lineNo,
					/* lexemsBefore    = */ lexemsBefore,
					/* lexemsAfter     = */ lexemsAfter,
					/* contextModule   = */ contextModule,
					/* contextAlgorithm= */ contextAlgorithm,
					/* typeIsKnown     = */ true,
					/* baseType        = */ targetBaseType,
					/* minimumDimension= */ 0,
					/* accessType      = */ AST::AccessArgumentIn
				);
		}
	} else if (operatorr == LxOperSlash && leftExpression != nullptr) {
		// Binary /
		// Applicable to reals only
		if (targetBaseType.kind == AST::TypeReal) {
			result  = suggestValueAutoComplete(
					lineNo,
					/* lexemsBefore    = */ lexemsBefore,
					/* lexemsAfter     = */ lexemsAfter,
					/* contextModule   = */ contextModule,
					/* contextAlgorithm= */ contextAlgorithm,
					/* typeIsKnown     = */ true,
					/* baseType        = */ AST::TypeReal,
					/* minimumDimension= */ 0,
					/* accessType      = */ AST::AccessArgumentIn
				);
		}
	} else if ((operatorr == LxOperEqual ||
			operatorr == LxOperNotEqual ||
			operatorr == LxOperLess ||
			operatorr == LxOperLessOrEqual ||
			operatorr == LxOperGreater ||
			operatorr == LxOperGreaterOrEqual
		) && leftExpression != nullptr) {
		// Comparision operators
		// Applicable in result of bool, type must be the same as left (or cast-applicable)
		if (targetBaseType.kind == AST::TypeBoolean) {
			result  = suggestValueAutoComplete(
					lineNo,
					/* lexemsBefore    = */ lexemsBefore,
					/* lexemsAfter     = */ lexemsAfter,
					/* contextModule   = */ contextModule,
					/* contextAlgorithm= */ contextAlgorithm,
					/* typeIsKnown     = */ true,
					/* baseType        = */ baseType,
					/* minimumDimension= */ 0,
					/* accessType      = */ AST::AccessArgumentIn
				);
		}
	} else if ((operatorr == LxSecAnd ||
			operatorr == LxSecOr
		) && leftExpression != nullptr) {
		// Logical operators
		// Applicable to bools only
		if (targetBaseType.kind == AST::TypeBoolean) {
			result  = suggestValueAutoComplete(
					lineNo,
					/* lexemsBefore    = */ lexemsBefore,
					/* lexemsAfter     = */ lexemsAfter,
					/* contextModule   = */ contextModule,
					/* contextAlgorithm= */ contextAlgorithm,
					/* typeIsKnown     = */ true,
					/* baseType        = */ AST::TypeBoolean,
					/* minimumDimension= */ 0,
					/* accessType      = */ AST::AccessArgumentIn
				);
		}
	}

	return result;
}

void SyntaxAnalizer::buildTables(bool isInternalBuild)
{

	for (int i = 0; i < statements_.size(); i++) {
		const TextStatement &st = statements_[i];
		bool wasError = st.hasError();
		if (st.type == LxPriModule) {
			parseModuleHeader(i);
		}
		if (st.type == LxPriImport) {
			parseImport(i);
		}
		if (!wasError && statements_[i].hasError()) {
			foreach (LexemPtr lx, statements_[i].data) {
				if (!lx->error.isEmpty()) {
					lx->errorStage = Lexem::Tables;
				}
			}
		}
	}


	// Find and load unresolved imports
	for (int i = 0; i < unresolvedImports_.size(); i++) {
		const QString name = unresolvedImports_.toList()[i];
		QString error;
		if (name.endsWith(".kod")) {
			QString canonicalName = name;
			if (canonicalName.startsWith(QDir::currentPath())) {
				canonicalName.remove(0, QDir::currentPath().length());
				if (canonicalName.startsWith("/")) {
					canonicalName.remove(0, 1);
				}
			}
			QFileInfo kodFile(name);
			QString kodFilePath = QDir::toNativeSeparators(kodFile.absoluteFilePath());
			char programName[1024];
			strcpy(programName, kodFilePath.toLocal8Bit().constData());
			std::ifstream programFile(programName,  std::ios::in | std::ios::binary);
			Bytecode::Data programData;
			if (!programFile.is_open()) {
				error = _("Can't open module file");
			} else {
				Bytecode::bytecodeFromDataStream(programFile, programData);
			}
			programFile.close();
			if (error.length() == 0) {
				AST::ModulePtr  module = AST::ModulePtr(new AST::Module);
				module->header.type = AST::ModTypeCached;
				module->header.name = canonicalName;
				ast_->modules.push_back(module);
				for (size_t e = 0; e < programData.d.size(); e++) {
					const Bytecode::TableElem &elem = programData.d.at(e);
					if (elem.type == Bytecode::EL_FUNCTION || elem.type == Bytecode::EL_MAIN) {
						const QString algName = QString::fromStdWString(elem.name);
						if (algName.length() > 0 && !algName.startsWith("_")) {
							AST::AlgorithmPtr  alg = AST::AlgorithmPtr(new AST::Algorithm);
							alg->header.name = algName;
							alg->header.implType = AST::AlgorhitmExternal;
							alg->header.external.moduleName = canonicalName;
							alg->header.external.id = elem.id;
							const QString signature = QString::fromStdWString(elem.signature);
							QStringList algSig = signature.split(":");
							alg->header.returnType = typeFromSignature(algSig[0]);
							if (algSig.size() > 1) {
								QStringList argSignatures = algSig[1].split(",");
								for (int argNo = 0; argNo < argSignatures.size(); argNo++) {
									if (argSignatures[argNo].length() == 0) {
										break;
									}
									AST::VariablePtr  var = AST::VariablePtr(new AST::Variable);
									QStringList sigPair = argSignatures[argNo].split(" ");
									if (sigPair[0] == "in") {
										var->accessType = AST::AccessArgumentIn;
									} else if (sigPair[0] == "out") {
										var->accessType = AST::AccessArgumentOut;
									} else if (sigPair[0] == "inout") {
										var->accessType = AST::AccessArgumentInOut;
									}
									var->baseType = typeFromSignature(sigPair[1]);
									var->dimension = sigPair[1].count("[]");
									alg->header.arguments.push_back(var);
								}
							}
							module->header.algorhitms.push_back(alg);
						}
					}
				}
			}

		}
	}

	// Set errors for imports that could not be resolved
	for (int i = 0; i < unresolvedImports_.size(); i++) {
		const QString name = unresolvedImports_.toList()[i];

		for (int j = 0; j < statements_.size(); j++) {
			if (statements_[j].type == Shared::LxPriImport && !statements_[j].hasError()) {
				QString nn;
				for (int k = 1; k < statements_[j].data.size(); k++) {
					LexemPtr lx = statements_[j].data[k];
					if (!nn.isEmpty()) {
						nn += " ";
					}
					nn += lx->data;
				}
				if (nn == name) {
					const QString error = _("No such module");
					for (int k = 1; k < statements_[j].data.size(); k++) {
						LexemPtr lx = statements_[j].data[k];
						lx->error = error;
					}
					if (statements_[j].mod) {
						AST::ModulePtr  mod = statements_[j].mod;
						AST::StatementPtr errStatement = AST::StatementPtr(new AST::Statement);
						errStatement->type = AST::StError;
						errStatement->error = error;
						errStatement->lexems = statements_[j].data;
						errStatement->skipErrorEvaluation = false;
						mod->impl.initializerBody.prepend(errStatement);
					}
				}
			}
		}
	}

	for (int i = 0; i < statements_.size(); i++) {
		const TextStatement &st = statements_[i];
		bool wasError = st.hasError();
		if (st.type == LxPriAlgHeader) {
			parseAlgHeader(i, true, isInternalBuild);
		}
		if (!wasError && statements_[i].hasError()) {
			foreach (LexemPtr lx, statements_[i].data) {
				if (!lx->error.isEmpty()) {
					lx->errorStage = Lexem::Tables;
				}
			}
		}
	}

	for (int i = 0; i < statements_.size(); i++) {
		const TextStatement &st = statements_[i];
		bool wasError = st.hasError();
		if (st.type == LxNameClass && st.alg == 0) {
			parseVarDecl(i);
		}
		if (st.type == LxPriAlgHeader) {
			parseAlgHeader(i, false, isInternalBuild);
		}
		if (!wasError && statements_[i].hasError()) {
			foreach (LexemPtr lx, statements_[i].data) {
				if (!lx->error.isEmpty()) {
					lx->errorStage = Lexem::Tables;
				}
			}
		}
	}

	checkForEmitImportsSignal();
}

void SyntaxAnalizer::checkForEmitImportsSignal()
{
	QStringList imports;
	AST::ModulePtr mainModule;
	for (int i = 0; i < ast_->modules.size(); i++) {
		const AST::ModulePtr module = ast_->modules[i];
		if (AST::ModTypeUserMain == module->header.type ||
			AST::ModTypeTeacherMain == module->header.type) {
			mainModule = module;
			break;
		}
	}
	if (mainModule) {
		for (int i = 0; i < ast_->modules.size(); i++) {
			const AST::ModulePtr module = ast_->modules[i];
			if (module->isEnabledFor(mainModule) && module->header.name.length() > 0) {
				imports << Shared::actorCanonicalName(module->header.name);
			}
		}
	}
	emit importsChanged(imports);
}

void SyntaxAnalizer::processAnalisys()
{
	for (int i = 0; i < statements_.size(); i++) {
		if (statements_[i].hasError()) {
			foreach (LexemPtr lx, statements_[i].data) {
				if (lx->errorStage == AST::Lexem::Semantics) {
					lx->error.clear();
					lx->errorStage = AST::Lexem::NoError;
				}
			}
		}
	}
	for (int i = 0; i < statements_.size(); i++) {
		currentPosition_ = i;
		TextStatement &st = statements_[i];
		// Fix unmatched modules first
		if (!st.mod) {
			for (int j = 0; j < ast_->modules.size(); j++) {
				if (ast_->modules[j]->header.type == AST::ModTypeUserMain || ast_->modules[j]->header.type == AST::ModTypeUser) {
					st.mod = ast_->modules[j];
					break;
				}
			}
		}
		if (!st.mod) {
			// Add dummy module for this statement
			// (it is possible to reuse dummy module later)
			AST::ModulePtr  dummyModule = AST::ModulePtr(new AST::Module);
			dummyModule->header.type = AST::ModTypeUser;
			ast_->modules.push_back(dummyModule);
			st.mod  = dummyModule;
		}
		bool wasError = st.hasError();
		if (st.statement) {
			statements_[i].statement->expressions.clear();
		}
		if (st.statement) {
			if (st.type == LxPriAssign) {
				parseAssignment(i);
			} else if (st.type == LxNameClass && st.alg) {
				parseVarDecl(i);
			} else if (
				st.type == LxPriAssert
				|| st.type == LxPriPre
				|| st.type == LxPriPost
			) {
				parseAssertPrePost(i);
			} else if (st.type == LxPriInput) {
				parseInput(i);
			} else if (
				st.type == LxPriOutput
			) {
				parseOutput(i);
			}

			else if (st.type == LxPriEndModule
				|| st.type == LxPriAlgBegin
				|| st.type == LxPriAlgEnd
				|| st.type == LxPriThen
				|| st.type == LxPriElse
				|| st.type == LxPriFi
				|| st.type == LxPriSwitch
				|| st.type == LxPriExit
				|| st.type == LxPriPause
				|| st.type == LxPriHalt
			) {
				parseOneLexemInstruction(i);
			} else if (st.type == LxPriEndLoop) {
				parseEndLoop(i);
			} else if (st.type == LxPriIf || st.type == LxPriCase) {
				parseIfCase(i);
			} else if (st.type == LxPriLoop) {
				parseLoopBegin(i);
			}
		}
		if (st.type == LxPriEndModule || st.type == LxPriAlgEnd) {
			parseEndNamedBlock(st);
		}
		if (st.statement && st.statement->expressions.size() > 0) {
			// Check for wrong "ds" keyword usage
			bool hasDSError = false;
			for (int j = 0; j < st.statement->expressions.size(); j++) {
				hasDSError = hasDSError || checkWrongDSUsage(st.statement->expressions.at(j));
			}
			if (hasDSError) {
				st.statement->expressions.clear();
			}
		}
		for (int j = 0; j < st.data.size(); j++) {
			if (!st.data[j]->error.isEmpty()) {
				if (st.statement) {
					if (st.type == LxPriLoop || st.type == LxPriSwitch) {
						st.statement->beginBlockError = st.data[j]->error;
					} else if (st.type == LxPriEndLoop || st.type == LxPriFi) {
						st.statement->endBlockError = st.data[j]->error;
					} else if (st.type == LxPriCase || st.type == LxPriElse) {
						if (st.conditionalIndex < st.statement->conditionals.size()) {
							st.statement->conditionals[st.conditionalIndex].conditionError = st.data[j]->error;
						}
					} else {
						st.statement->type = AST::StError;
						st.statement->error = st.data[j]->error;
					}
					break;
				}
			}
		}
		if (!wasError && statements_[i].hasError()) {
			foreach (LexemPtr lx, statements_[i].data) {
				if (lx->errorStage == AST::Lexem::NoError && !lx->error.isEmpty())  {
					lx->errorStage = AST::Lexem::Semantics;
				}

			}
		}
	}
}



QList<Shared::Analizer::Suggestion> SyntaxAnalizer::suggestAutoComplete(
	int lineNo,
	const TextStatementPtr statementBefore,
	const QList<LexemPtr> lexemsAfter,
	const AST::ModulePtr contextModule,
	const AST::AlgorithmPtr contextAlgorithm) const
{

	QList<Shared::Analizer::Suggestion> result;
	if (statementBefore == nullptr || statementBefore->type == LxPriAssign) {
		result = suggestAssignmentAutoComplete(lineNo, statementBefore, lexemsAfter, contextModule, contextAlgorithm);
	} else if (statementBefore->type == LxPriImport) {
		result = suggestImportAutoComplete(lineNo, statementBefore, lexemsAfter);
	} else if (statementBefore->type == LxPriInput || statementBefore->type == LxPriOutput) {
		result = suggestInputOutputAutoComplete(lineNo, statementBefore, lexemsAfter, contextModule, contextAlgorithm);
	} else if (statementBefore->type == LxPriAssert
		|| statementBefore->type == LxPriPre
		|| statementBefore->type == LxPriPost
		|| statementBefore->type == LxPriIf
		|| statementBefore->type == LxPriCase
		|| (statementBefore->type == LxPriEndLoop && statementBefore->data.size() > 1 && statementBefore->data[1]->type == LxSecIf)
		// endloop_if ...
		|| (statementBefore->type == LxPriLoop && statementBefore->data.size() > 1 && statementBefore->data[1]->type == LxSecWhile)
		// loop while ...
	) {
		result = suggestConditionAutoComplete(lineNo, statementBefore, lexemsAfter, contextModule, contextAlgorithm);
	} else if (statementBefore->type == LxPriLoop) {
		result = suggestLoopBeginAutoComplete(lineNo, statementBefore, lexemsAfter, contextModule, contextAlgorithm);
	}
	QList<Shared::Analizer::Suggestion> filteredResult;
	foreach (Shared::Analizer::Suggestion s, result) {
		// Filter suggestions to match last lexem text
		if (statementBefore && statementBefore->data.size() > 0 && statementBefore->data.last()->type == LxTypeName) {
			LexemPtr last = statementBefore->data.back();
			const QString lastText = last->data;
			const QString match = s.kind == Shared::Analizer::Suggestion::SecondaryKeyword ?
				s.value.trimmed() : s.value;
			if (match.startsWith(lastText)) {
				filteredResult.push_back(s);
			}
		} else {
			if (statementBefore && statementBefore->data.size() > 0) {
				LexemPtr last = statementBefore->data.back();
				if (last->type & LxTypePrimaryKwd || last->type & LxTypeSecondaryKwd)
					if (!s.value.startsWith(' ')) {
						s.value.prepend(' ');
					}
			}
			filteredResult.push_back(s);
		}
	}

	QList<Shared::Analizer::Suggestion> extraFilteredResult;
	foreach (Shared::Analizer::Suggestion s, filteredResult) {
		bool remove = false;
		if (!teacherMode_ && s.value.startsWith("@")) {
			remove = true;
		}
		if (s.value.trimmed().isEmpty()) {
			remove = true;
		}
		if (!remove) {
			extraFilteredResult.append(s);
		}
	}

	return extraFilteredResult;
}



void SyntaxAnalizer::setSourceDirName(const QString &dirName)
{
	sourceDirName_ = dirName;
}

static AST::ModulePtr findModuleByActor(AST::DataPtr ast,
	Shared::ActorInterface *actor)
{
	foreach (AST::ModulePtr mod, ast->modules) {
		if (mod->impl.actor == actor) {
			return mod;
		}
	}
	return AST::ModulePtr();
}

void SyntaxAnalizer::parseImport(int str)
{
	if (statements_[str].hasError()) {
		return;
	}
	TextStatement &st = statements_[str];
	AST::ModulePtr  mod = st.mod;
	AST::ModulePtr moduleToImport;
	Q_CHECK_PTR(mod);
	if (st.data.size() < 2) {
		Q_ASSERT(st.data.size() == 1);
		st.data[0]->error = _("No module name");
		return;
	}
	if (st.data[1]->data.isEmpty()) {
		st.data[1]->error = _("No module name");
		return;
	}
	QString name;
	if (st.data[1]->type == LxConstLiteral) {
		if (st.data.size() > 2) {
			for (int i = 2; i < st.data.size(); i++) {
				st.data[i]->error = _("Garbage afrer module name");
			}
			return;
		}
		name = st.data[1]->data.trimmed();
		if (name.isEmpty()) {
			st.data[1]->error = _("Must be Kumir program file name");
			return;
		}
		if (!name.endsWith(".kum") && !name.endsWith(".kod")) {
			name += ".kum";
		}
		QString kumName, binName;
		QFileInfo binFile, kumFile;
		if (name.endsWith(".kum")) {
			kumName = name;
			name = binName = name.left(name.length() - 4) + ".kod";
			binFile = QFileInfo(QDir(sourceDirName_).absoluteFilePath(binName));
			kumFile = QFileInfo(QDir(sourceDirName_).absoluteFilePath(kumName));
			QString kumir2bc = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("kumir2-bc");
#ifdef Q_OS_WIN32
			kumir2bc += ".exe";
#endif
			if (kumFile.exists()) {
				if (!binFile.exists() || binFile.lastModified() < kumFile.lastModified()) {
					// Run kumir2-bc to create (or update) binary module
					int status = QProcess::execute(kumir2bc, QStringList()
							<< "--debuglevel=2" << kumFile.absoluteFilePath());
					if (status == -2) {
						st.data[1]->error = _("Can't run kumir2-bc to compile this module");
						return;
					} else if (status == -1) {
						st.data[1]->error = _("kumir2-bc crashed while compiling this module");
						return;
					}
					binFile = QFileInfo(QDir(sourceDirName_).absoluteFilePath(binName));
					if (!binFile.exists()) {
						st.data[1]->error = _("Error compiling this module");
						return;
					}
				}
			}
		} else {
			binName = name;
			binFile = QFileInfo(QDir::current().absoluteFilePath(binName));
		}
		if (!kumFile.exists() && !binFile.exists()) {
			st.data[1]->error = _("No such file");
			return;
		}
		name = binFile.absoluteFilePath();
		moduleToImport = loadKodFile(name, st.data[1]->error);
		if (!moduleToImport) {
			return;
		}
	} else {
//        bool isTemplateName = false;
		foreach (const LexemPtr lexem, st.data.mid(1)) {
			name += " " + lexem->data;
		}
		QString localError = lexer_->testName(name);
		if (localError.size() > 0) {
			foreach (LexemPtr lexem, st.data.mid(1)) {
				lexem->error = localError;
			}
			return;
		}
		name = name.simplified();
		foreach (AST::ModulePtr module, ast_->modules) {
//            if (module->header.nameTemplate.length() == 0) {
			// Regular module name
			if (module->header.name == name && !module->header.name.startsWith("_")) {
				moduleToImport = module;
				break;
			}
//            }
//            else {
//                QString namePattern = module->header.nameTemplate;
//                namePattern.replace("%d", "(\\d+)");
//                namePattern.replace("%s", "(\\w+)");
//                QRegExp moduleRx(namePattern);
//                if (-1 != moduleRx.indexIn(name)) {
//                    const QStringList caps = moduleRx.capturedTexts().mid(1);
//                    if (caps.size() == module->header.templateTypes.size()) {
//                        for (int k=0; k<caps.size(); k++) {
//                            QVariant param = QVariant(caps[k]);
//                            param.convert(module->header.templateTypes[k]);
//                            module->header.templateParameters[k] = param;
//                        }
//                        moduleToImport = module;
//                        break;
//                    }
//                }
//            }
		}
//        if (!isTemplateName) {
		if (st.data.size() > 2) {
			for (int i = 2; i < st.data.size(); i++) {
				st.data[i]->error = _("Garbage afrer module name");
			}
			return;
		}
//        }
	}
	if (moduleToImport) {
		st.data[1]->type = LxNameModule;
		if (moduleToImport->isEnabledFor(st.mod)) {
			st.data[1]->error = _("Module already imported");
			return;
		} else {
			moduleToImport->header.usedBy.append(st.mod.toWeakRef());
			if (moduleToImport->impl.actor) {
				QList<Shared::ActorInterface *> deps =
					moduleToImport->impl.actor->usesList();
				foreach (Shared::ActorInterface *actor, deps) {
					AST::ModulePtr actorMod = findModuleByActor(ast_, actor);
					actorMod->header.usedBy.append(st.mod.toWeakRef());
				}
			}
		}
	} else {
		// Check if valid but uncomplete name
//        bool incompleteName = false;
//        foreach (AST::ModulePtr module, ast_->modules) {
//            if (module->header.nameTemplate.length() > 0) {
//                if (name == module->header.name) {
//                    moduleToImport = module;
//                    incompleteName = true;
//                    break;
//                }
//            }
//        }
//        if (incompleteName) {
//            // Try to use default template parameters
//            if (moduleToImport->impl.actor) {
//                const ActorInterface * actor = moduleToImport->impl.actor;
//                if (actor->defaultTemplateParameters().size() > 0) {
//                    incompleteName = false;
//                    moduleToImport->header.templateParameters = actor->defaultTemplateParameters();
//                }
//            }
//            if (incompleteName) {
//                for (int i=1; i<st.data.size(); i++) {
//                    st.data[i]->error = _("Incomplete module name");
//                }
//                return;
//            }
//            else {
//                moduleToImport->header.usedBy.append(st.mod.toWeakRef());
//                if (moduleToImport->impl.actor) {
//                    QList<Shared::ActorInterface*> deps =
//                            moduleToImport->impl.actor->usesList();
//                    Q_FOREACH (Shared::ActorInterface * actor, deps) {
//                        AST::ModulePtr actorMod = findModuleByActor(ast_, actor);
//                        actorMod->header.usedBy.append(st.mod.toWeakRef());
//                    }
//                }
//                for (int i=1; i<st.data.size(); i++) {
//                    st.data[i]->type = LxNameModule;
//                }
//            }
//        }
//        else {
		for (int i = 1; i < st.data.size(); i++) {
			st.data[i]->error = _("No such module");
		}
		return;
//        }
	}
	const QStringList conflictNames = checkForConflictingNames(moduleToImport, st.mod);
	if (conflictNames.size() > 0) {
		st.data[1]->error = _("Name conflict with modules: %1",
				conflictNames.join(", ")
			);
		moduleToImport->header.usedBy.removeAll(st.mod.toWeakRef());
		return;
	}
}

QStringList SyntaxAnalizer::checkForConflictingNames(const ModulePtr &moduleToCheck, const ModulePtr &parentModule) const
{
	QSet<QString> result;

	QStringList thisModuleNames;
	Q_FOREACH (const AST::AlgorithmPtr &alg, moduleToCheck->header.algorhitms) {
		thisModuleNames << alg->header.name;
	}
	Q_FOREACH (const AST::Type &type, moduleToCheck->header.types) {
		thisModuleNames << type.name;
	}

	Q_FOREACH (const QString &nameToCheck, thisModuleNames) {
		Q_FOREACH (const AST::ModulePtr &module, ast_->modules) {
			bool moduleAvailable =
				module->builtInID == 0xF0 ||
				module->isEnabledFor(parentModule) ||
				alwaysEnabledModules_->contains(module->header.name)
				;
			if (moduleAvailable && module != moduleToCheck) {
				Q_FOREACH (const AST::AlgorithmPtr &alg, module->header.algorhitms) {
					const QString &algName = alg->header.name;
					if (algName == nameToCheck) {
						result.insert(module->header.name);
						break;
					}
				}
				Q_FOREACH (const AST::Type &type, module->header.types) {
					const QString &typeName = type.name;
					if (typeName == nameToCheck) {
						result.insert(module->header.name);
						break;
					}
				}
			}
		}
	}
	return result.toList();
}

AST::ModulePtr SyntaxAnalizer::loadKodFile(const QString &name, QString &error)
{
	QString canonicalName = name;
	if (canonicalName.startsWith(QDir::currentPath())) {
		canonicalName.remove(0, QDir::currentPath().length());
		if (canonicalName.startsWith("/")) {
			canonicalName.remove(0, 1);
		}
	}
	QFileInfo kodFile(name);
	QString kodFilePath = QDir::toNativeSeparators(kodFile.absoluteFilePath());
	char programName[1024];
	strcpy(programName, kodFilePath.toLocal8Bit().constData());
	std::ifstream programFile(programName,  std::ios::in | std::ios::binary);
	Bytecode::Data programData;
	if (!programFile.is_open()) {
		error = _("Can't open module file");
	} else {
		Bytecode::bytecodeFromDataStream(programFile, programData);
	}
	programFile.close();
	AST::ModulePtr result;
	if (error.length() == 0) {
		AST::Module *module = new AST::Module;
		module->header.type = AST::ModTypeCached;
		module->header.name = canonicalName;
		for (size_t e = 0; e < programData.d.size(); e++) {
			const Bytecode::TableElem &elem = programData.d.at(e);
			if (elem.type == Bytecode::EL_FUNCTION || elem.type == Bytecode::EL_MAIN) {
				const QString algName = QString::fromStdWString(elem.name);
				if (algName.length() > 0 && !algName.startsWith("_")) {
					AST::Algorithm *alg = new AST::Algorithm;
					alg->header.name = algName;
					alg->header.implType = AST::AlgorhitmExternal;
					alg->header.external.moduleName = canonicalName;
					alg->header.external.id = elem.id;
					const QString signature = QString::fromStdWString(elem.signature);
					QStringList algSig = signature.split(":");
					alg->header.returnType = typeFromSignature(algSig[0]);
					if (algSig.size() > 1) {
						QStringList argSignatures = algSig[1].split(",");
						for (int argNo = 0; argNo < argSignatures.size(); argNo++) {
							if (argSignatures[argNo].length() == 0) {
								break;
							}
							AST::Variable *var = new AST::Variable;
							QStringList sigPair = argSignatures[argNo].split(" ");
							if (sigPair[0] == "in") {
								var->accessType = AST::AccessArgumentIn;
							} else if (sigPair[0] == "out") {
								var->accessType = AST::AccessArgumentOut;
							} else if (sigPair[0] == "inout") {
								var->accessType = AST::AccessArgumentInOut;
							}
							var->baseType = typeFromSignature(sigPair[1]);
							var->dimension = sigPair[1].count("[]");
							alg->header.arguments.push_back(AST::VariablePtr(var));
						}
					}
					module->header.algorhitms.push_back(AST::AlgorithmPtr(alg));
				}
			}
		}
		result = AST::ModulePtr(module);
		ast_->modules.push_back(result);
	}
	return result;
}

void SyntaxAnalizer::parseModuleHeader(int str)
{
	if (statements_[str].hasError()) {
		return;
	}
	const TextStatement &st = statements_[str];
	if (st.data.size() < 2) {
		Q_ASSERT(st.data.size() == 1);
		st.data[0]->error = _("No module name");
		return;
	}
	if (st.data.size() > 2) {
		for (int i = 2; i < st.data.size(); i++) {
			st.data[i]->error = _("Garbage afrer module name");
		}
		return;
	}
	if (st.data[1]->data.isEmpty()) {
		st.data[1]->error = _("No module name");
		return;
	}
	QString localError = lexer_->testName(st.data[1]->data);
	if (localError.size() > 0) {
		st.data[1]->error = localError;
		return;
	}
	AST::ModulePtr  mod = st.mod;
	Q_CHECK_PTR(mod);
	st.data[1]->type = LxNameModule;
	mod->header.name = st.data[1]->data.simplified();
}


void SyntaxAnalizer::parseVarDecl(int str)
{
	TextStatement &st = statements_[str];
	if (st.hasError()) {
		return;
	}

	// Check for in-block declaration
	AST::StatementWPtr p = st.statement.toWeakRef();
	while (p) {
		QString error;
		if (p.data()->type == AST::StLoop) {
			error = _("Can't declare variable in loop");
		} else if (p.data()->type == AST::StIfThenElse) {
			error = _("Can't declare variable in condidion");
		} else if (p.data()->type == AST::StSwitchCaseElse) {
			error = _("Can't declare variable in switch");
		}
		if (error.length() > 0) {
			st.setError(error,
				Lexem::Tables,
				Lexem::AsIs);
			return;
		}
		p = p.data()->parent;
	}

	AST::AlgorithmPtr  alg = st.alg;
	AST::ModulePtr  mod = st.mod;
	VariablesGroup group;
	group.access = AST::AccessRegular;
	for (int i = 0; i < st.data.size(); i++) {
		if (st.data[i]->type & LxTypeComment) {
			break;
		}
		group.lexems << st.data[i];
	}
	QList<AST::VariablePtr> vars = parseVariables(str, group, mod, alg, false);
	QString error;
	for (int i = 0; i < group.lexems.size() - 1; i++) {
		if (group.lexems[i]->error.size() > 0) {
			error = group.lexems[i]->error;
			break;
		}
	}
	if (!error.isEmpty()) {
		st.statement->type = AST::StError;
		st.statement->error = error;
	} else {
		if (!st.statement) {
			st.statement = AST::StatementPtr(new Statement);
		}
		st.statement->type = AST::StVarInitialize;
		st.statement->variables = vars;
	}
}

void SyntaxAnalizer::parseOutput(int str)
{
	const TextStatement &st = statements_[str];
	if (st.hasError()) {
		return;
	}
	if (st.data.size() == 1) {
		QString err = _("What to output?");
		st.data[0]->error = err;
		return;
	}
	if (st.data.last()->type == LxOperComa) {
		st.data.last()->error = _("Statement ends with coma");
		return;
	}
	QList< QList<LexemPtr> > groups;
	QList<LexemPtr> comas;
	splitLexemsByOperator(st.data.mid(1), LxOperComa, groups, comas);

	AST::ExpressionPtr  fileHandle;

	for (int i = 0 ; i < groups.size(); i++) {
		QString err;
		if (groups[i].size() == 0) {
			if (i < comas.size()) {
				comas[i]->error = _("Extra coma");
			} else {
				comas[i - 1]->error = _("Extra coma");
			}
			return;
		}
		QList< QList<LexemPtr> > subgroups;
		QList< LexemPtr > colons;
		splitLexemsByOperator(groups[i], LxOperColon, subgroups, colons);
		AST::ExpressionPtr  expr = parseExpression(subgroups[0], st.mod, st.alg);
		if (!expr) {
			return;
		}
		int maxSubgroups;
		if (expr->baseType.kind == AST::TypeReal) {
			maxSubgroups = 3;
		} else if (expr->baseType.kind == AST::TypeInteger) {
			maxSubgroups = 2;
		} else {
			maxSubgroups = 1;
		}
		if (expr->baseType.kind == AST::TypeNone && expr->kind == AST::ExprFunctionCall) {
			err = _("This algorithm has no return value");
			foreach (LexemPtr lx, groups[i]) {
				lx->error = err;
			}
			return;
		}
		Q_ASSERT(colons.size() == subgroups.size() - 1);
		if (subgroups.size() > maxSubgroups) {
			err = _("Extra format parameter");
			for (int j = maxSubgroups - 1 ; j < colons.size(); j++) {
				colons[j]->error = err;
			}
			for (int j = maxSubgroups; j < subgroups.size(); j++) {
				foreach (LexemPtr lx, subgroups[j]) {
					lx->error = err;
				}
			}
			return;
		}
		if (!fileHandle && expr->baseType.kind == AST::TypeUser && expr->baseType.name == QString::fromUtf8("файл")) {
			if (i > 0) {
				st.statement->expressions.clear();
				foreach (LexemPtr lx, groups[i]) {
					lx->error = _("File must be specified at first");
				}
				return;
			}
			fileHandle = expr;
			continue;
		}
		AST::ExpressionPtr  expr2, expr3;
		if (subgroups.size() > 1) {
			expr2 = parseExpression(subgroups[1], st.mod, st.alg);
			if (!expr2) {
				return;
			}
			if (expr2->baseType.kind != AST::TypeInteger) {
				err = _("Format parameter not integer");
				foreach (LexemPtr lx, subgroups[1]) {
					lx->error = err;
				}
				return;
			}
		} else {
			expr2 = AST::ExpressionPtr(new AST::Expression);
			expr2->baseType.kind = AST::TypeInteger;
			expr2->dimension = 0;
			expr2->kind = AST::ExprConst;
			expr2->constant = QVariant(0);
		}
		if (subgroups.size() > 2) {
			expr3 = parseExpression(subgroups[2], st.mod, st.alg);
			if (!expr3) {
				return;
			}
			if (expr3->baseType.kind != AST::TypeInteger) {
				err = _("Format parameter not integer");
				foreach (LexemPtr lx, subgroups[2]) {
					lx->error = err;
				}
				return;
			}
		} else {
			expr3 = AST::ExpressionPtr(new AST::Expression);
			expr3->baseType.kind = AST::TypeInteger;
			expr3->dimension = 0;
			expr3->kind = AST::ExprConst;
			expr3->constant = QVariant(-1);
		}
		if (expr->baseType.kind == AST::TypeUser) {
			bool canConvert = makeCustomUnaryOperation<bool>(lexer_->outputLexemName(), expr, st.mod);
			if (!canConvert) {
				err = _("Can't output value of type %1", expr->baseType.name);
				foreach (LexemPtr lx, subgroups[0]) {
					lx->error = err;
				}
				return;
			}
			expr = makeCustomUnaryOperation<AST::ExpressionPtr>(lexer_->outputLexemName(), expr, st.mod);
		}
		st.statement->expressions << expr << expr2 << expr3;
	}
	if (fileHandle) {
		if (st.statement->expressions.size() == 0) {
			QString err = _("What to output to file?");
			st.data.at(0)->error = err;
			return;
		}
		st.statement->expressions << fileHandle;
	}
}




void SyntaxAnalizer::parseInput(int str)
{
	const TextStatement &st = statements_[str];
	if (st.hasError()) {
		return;
	}
	if (st.data.size() == 1) {
		QString err = _("What to input?");
		st.data[0]->error = err;
		return;
	}
	if (st.data.last()->type == LxOperComa) {
		st.data.last()->error = _("Statement ends with coma");
		return;
	}
	QList< QList<LexemPtr> > groups;
	QList<LexemPtr> comas;
	splitLexemsByOperator(st.data.mid(1), LxOperComa, groups, comas);

	AST::ExpressionPtr  fileHandle;

	for (int i = 0 ; i < groups.size(); i++) {
		QString err;

		AST::ExpressionPtr  expr = parseExpression(groups[i], st.mod, st.alg);
		if (!expr) {
			st.statement->expressions.clear();
			return;
		}

		if (!fileHandle && expr->baseType.kind == AST::TypeUser && expr->baseType.name == QString::fromUtf8("файл")) {
			if (i > 0) {

				st.statement->expressions.clear();
				foreach (LexemPtr lx, groups[i]) {
					lx->error = _("File must be specified at first");
				}
				return;
			}
			fileHandle = expr;
			continue;
		}

		if (expr->baseType.kind == AST::TypeUser) {
			bool canConvert = makeCustomUnaryOperation<bool>(lexer_->inputLexemName(), expr, st.mod);
			if (!canConvert) {
				err = _("Can't input value of type %1", expr->baseType.name);
			}
		}

		if (expr->kind == AST::ExprConst &&
			(expr->baseType != AST::TypeCharect || expr->constant.toChar() != '\n')) {
			err = _("Can't input constant");
		}
		if (expr->kind == AST::ExprFunctionCall) {
			err = _("Can't input function call");
		}
		if (expr->kind == AST::ExprSubexpression) {
			err = _("Can't input subexpression");
		}
		if (expr->dimension > 0) {
			err = _("Can't input an array");
		}
		if (expr->isStringPart) {
			err = _("Can't input part of string");
		}
		if (expr->kind == AST::ExprVariable && expr->variable->accessType == AST::AccessArgumentIn) {
			err = _("Can't input in-argument");
		}

		if (err.length() > 0) {
			foreach (LexemPtr lx, groups[i]) {
				lx->error = err;
			}
			st.statement->expressions.clear();
			return;
		}

		st.statement->expressions << expr;
	}
	if (fileHandle) {
		if (st.statement->expressions.size() == 0) {
			QString err = _("What to input from file?");
			st.data.at(0)->error = err;
			return;
		}
		st.statement->expressions << fileHandle;
	}
}

void SyntaxAnalizer::parseAssertPrePost(int str)
{
	const TextStatement &st = statements_[str];
	if (st.hasError()) {
		return;
	}
//    if (st.data.size()==1) {
//        QString err = _("No expressions after '%1'", st.data[0]->data);
//        if (st.data[0]->type!=LxPriPre && st.data[0]->type!=LxPriPost)
//            st.data[0]->error = err;
//        return;
//    }
	if (st.data.last()->type == LxOperComa) {
		st.data.last()->error = _("Statement ends with coma");
		return;
	}
	QList< QList<LexemPtr> > groups;
	QList<LexemPtr> comas;
	splitLexemsByOperator(st.data.mid(1), LxOperComa, groups, comas);

	for (int i = 0 ; i < groups.size(); i++) {

		AST::ExpressionPtr  expr = parseExpression(groups[i], st.mod, st.alg);
		if (!expr) {
			return;
		}
		QString err;
		if (expr->baseType.kind != AST::TypeBoolean) {
			err = _("Condition is not boolean");
		}

		if (!err.isEmpty()) {
			for (int a = 0; a < groups[i].size(); a++) {
				groups[i][a]->error = err;
			}
			return;
		}
		st.statement->expressions << expr;
	}
}


void SyntaxAnalizer::parseEndNamedBlock(TextStatement &st)
{
	if (st.hasError()) {
		return;
	}
	AST::AlgorithmPtr alg = st.alg;
	AST::ModulePtr mod = st.mod;
	QList<AST::LexemPtr> contentLexems = st.data.mid(1);
	QList<AST::LexemPtr> nameLexems;
	QString name;
	static const QString GARBAGE = _("Garbage at end of line");
	static const QString NAME_MISMATCH = _("Name does not match header");
	QString currentError;
	bool forceGarbage = false;
	for (int i = 0; i < contentLexems.size(); i++) {
		AST::LexemPtr lx = contentLexems.at(i);
		if (lx->type == LxTypeName && !forceGarbage) {
			if (name.length() > 0) {
				name += " ";
			}
			name += lx->data;
			nameLexems << lx;
		} else {
			forceGarbage = true;
			lx->error = GARBAGE;
			currentError = GARBAGE;
		}
	}
	const QString &headerName = alg ? alg->header.name : mod->header.name;
	if (name.length() > 0 && headerName != name) {
		foreach (LexemPtr lx, nameLexems) {
			lx->error = forceGarbage || headerName.isEmpty() ? GARBAGE : NAME_MISMATCH;
			currentError = lx->error;
		}
	}
	if (st.hasError() && !st.statement) {
		AST::Statement *err = new AST::Statement;
		err->type = AST::StError;
		err->error = currentError;
		err->lexems = st.data;
		st.statement = AST::StatementPtr(err);
	}

	foreach (LexemPtr lx, nameLexems) {
		if (lx->error.isEmpty()) {
			lx->type = alg ? Shared::LxNameAlg : Shared::LxNameModule;
		}
	}

}

void SyntaxAnalizer::parseOneLexemInstruction(int str)
{
	const TextStatement &st = statements_[str];
	if (st.hasError()) {
		return;
	}
	for (int i = 1; i < st.data.size(); i++) {
		st.data[i]->error = _("Garbage at end of line");
	}
}

void SyntaxAnalizer::parseEndLoop(int str)
{
	const TextStatement &st = statements_[str];
	if (st.hasError()) {
		return;
	}
	if (st.data.size() > 1 && st.data[1]->type != LxSecIf) {
		for (int i = 1; i < st.data.size(); i++) {
			st.data[i]->error = _("Garbage at end of line");
		}
		return;
	}
	if (st.data.size() == 2 && st.data[1]->type == LxSecIf) {

		st.data[1]->error = _("No condition after 'end if'");
		if (st.data[1]->data.startsWith("_")) {
			// Mark "endloop_if" first lexem if it's written close to second
			st.data[0]->error = st.data[1]->error;
		}
		return;
	}
	if (st.data.size() > 2) {
		QList<LexemPtr> condLexems = st.data.mid(2);
		AST::ExpressionPtr  expr = parseExpression(condLexems, st.mod, st.alg);
		if (expr) {
			if (expr->baseType.kind != AST::TypeBoolean) {
				for (int i = 2; i < st.data.size(); i++) {
					st.data[i]->error = _("Condition is not boolean");
				}
			} else {
				st.statement->loop.endCondition = expr;
			}
		}
	}

}

void SyntaxAnalizer::parseIfCase(int str)
{
	const TextStatement &st = statements_[str];
	if (st.hasError()) {
		return;
	}
	if (st.type == LxPriIf && st.data.size() == 1) {
		st.data[0]->error = _("No condition after 'if'");
		return;
	}
	if (st.type == LxPriCase && st.data.size() == 1) {
		st.data[0]->error = _("No condition after 'case'");
		return;
	}
	if (st.type == LxPriCase && st.data.size() == 2 && st.data[1]->type == LxOperColon) {
		st.data[0]->error = _("No condition between 'case' and colon");
		st.data[1]->error = _("No condition between 'case' and colon");
		return;
	}
	if (st.type == LxPriCase && st.data.last()->type != LxOperColon) {
		for (int i = 0; i < st.data.size(); i++) {
			st.data[i]->error = _("No colon after condition");
		}
		return;
	}
	QList<LexemPtr> cond;
	if (st.type == LxPriCase) {
		cond = st.data.mid(1, st.data.length() - 2);
	} else {
		cond = st.data.mid(1);
	}
	AST::ExpressionPtr  expr = parseExpression(cond, st.mod, st.alg);
	if (expr) {
		if (expr->baseType.kind != AST::TypeBoolean) {
			for (int i = 0; i < cond.size(); i++) {
				cond[i]->error = _("Condition after '%1' not boolean", st.data[0]->data);
			}
		} else {
			if (st.statement && st.conditionalIndex < st.statement->conditionals.size()) {
				st.statement->conditionals[st.conditionalIndex].condition = expr;
			} else {
			}
		}
	}
}

void SyntaxAnalizer::parseLoopBegin(int str)
{
	const TextStatement &st = statements_[str];
	if (st.hasError() || !st.statement) {
		return;
	}
	AST::LoopType type = AST::LoopWhile;
	if (st.data.size() == 1) {
		type = AST::LoopForever;
	} else if (st.data.size() > 1 && st.data[1]->type == LxSecFor) {
		type = AST::LoopFor;
	} else if (st.data.size() > 1 && st.data[1]->type == LxSecWhile) {
		type = AST::LoopWhile;
	} else if (findLexemByType(st.data, LxSecTimes)) {
		type = AST::LoopTimes;
	} else {
		st.data[0]->error = _("Loop type not specified");
		return;
	}
	st.statement->loop.type = type;
	if (type == AST::LoopFor) {
		QList<LexemPtr> forr;
		QList<LexemPtr> from;
		QList<LexemPtr> to;
		QList<LexemPtr> step;
		LexemPtr forLexem = st.data[1];
		LexemPtr fromLexem = findLexemByType(st.data, LxSecFrom);
		LexemPtr toLexem = findLexemByType(st.data, LxSecTo);
		LexemPtr stepLexem = findLexemByType(st.data, LxSecStep);


		int forIndex = st.data.indexOf(forLexem);
		int fromIndex = st.data.indexOf(fromLexem);
		int toIndex = st.data.indexOf(toLexem);
		int stepIndex = st.data.indexOf(stepLexem);

		QString err =  "";

		if (err.length() == 0 && !fromLexem) {
			err = _("No loop 'from'");
		}
		if (err.length() == 0 && !toLexem) {
			err = _("No loop 'to'");
		}
		if (err.length() == 0 && fromIndex != -1 && forIndex != -1 && fromIndex - forIndex == 1) {
			err =  _("No loop variable");
		}

		if (err.length() == 0 && toIndex != -1 && fromIndex > toIndex) {
			err = _("'to' earler then 'from'");
		}

		if (err.length() == 0 && fromIndex == -1 && toIndex != -1) {
			err = _("No 'from' before 'to'");
		}

		if (err.length() == 0 && fromIndex == -1 && toIndex == -1) {
			err = _("No 'from'..'to'.. after variable");
		}

		if (err.length()) {
			for (int i = 1; i < st.data.size(); i++) {
				st.data[i]->error = err;
			}
			return;
		}

		if (stepIndex == -1) {
			stepIndex = st.data.size();
		}
		for (int i = forIndex + 1; i < fromIndex; i++) {
			forr << st.data[i];
		}
		for (int i = fromIndex + 1; i < toIndex; i++) {
			from << st.data[i];
		}
		for (int i = toIndex + 1; i < stepIndex; i++) {
			to << st.data[i];
		}
		for (int i = stepIndex + 1; i < st.data.size(); i++) {
			step << st.data[i];
		}
		if (forr.isEmpty()) {
			forLexem->error = _("No for-loop variable");
			return;
		}
		if (from.isEmpty()) {
			fromLexem->error = _("From-value not specified");
			return;
		}
		if (to.isEmpty()) {
			toLexem->error = _("Then-value not specified");
			return;
		}
		if (step.isEmpty() && stepLexem) {
			stepLexem->error = _("Step-value is empty");
		}
		QString name;
		AST::AlgorithmPtr  dummyAlg;
		AST::ModulePtr dummyMod;
		QVariantList templateParameters;
		for (int i = 0; i < forr.size(); i++) {
			if (i > 0) {
				name += " ";
			}
			name += forr[i]->data;
		}
		if (findLocalVariable(name, st.alg, st.statement->loop.forVariable)) {

		} else if (findGlobalVariable(name, st.mod, st.statement->loop.forVariable)) {

		} else if (findAlgorithm(name, st.mod, st.alg, dummyMod, dummyAlg, templateParameters)) {
			foreach (LexemPtr l, forr) l->error = _("Algorithm can't be a loop variable");
			return;
		} else if (forr.size() == 1 && forr[0]->type & LxTypeConstant) {
			foreach (LexemPtr l, forr) l->error = _("Constant for-loop variable");
			return;
		} else {
			foreach (LexemPtr l, forr) l->error = _("Variable not found");
			return;
		}
		if (st.statement->loop.forVariable->accessType == AST::AccessArgumentIn) {
			foreach (LexemPtr l, forr) l->error = _("Can't use input parameter as for-variable");
			return;
		}
		if (st.statement->loop.forVariable->baseType.kind != AST::TypeInteger
			|| st.statement->loop.forVariable->dimension > 0) {
			foreach (LexemPtr l, forr) l->error = _("Not integer for-loop variable");
			return;
		}
		AST::ExpressionPtr  fromExpr = parseExpression(from, st.mod, st.alg);
		if (!fromExpr) {
			return;
		} else if (fromExpr->baseType.kind != AST::TypeInteger) {
			foreach (LexemPtr l, from) l->error = _("Not integer from-value");
			return;
		}
		AST::ExpressionPtr  toExpr = parseExpression(to, st.mod, st.alg);
		if (!toExpr) {
			return;
		} else if (toExpr->baseType.kind != AST::TypeInteger) {
			foreach (LexemPtr l, to) l->error = _("Not integer to-value");
			return;
		}
		if (stepLexem && step.isEmpty()) {
			stepLexem->error = _("No step-value");
			return;
		}
		AST::ExpressionPtr  stepExpr = stepLexem ? parseExpression(step, st.mod, st.alg) : AST::ExpressionPtr();
		if (stepLexem && !stepExpr) {
			return;
		} else if (stepExpr && stepExpr->baseType.kind != AST::TypeInteger) {
			foreach (LexemPtr l, step) l->error = _("Not integer step-value");
			return;
		}
		st.statement->loop.fromValue = fromExpr;
		st.statement->loop.toValue = toExpr;
		st.statement->loop.stepValue = stepExpr;

	} // end if (type==AST::LoopFor)
	else if (type == AST::LoopTimes) {
		QList<LexemPtr> times;
		bool timesFound = false;
		bool err = false;
		for (int i = 1; i < st.data.size(); i++) {
			if (st.data[i]->type == LxSecTimes) {
				timesFound = true;
				if (times.isEmpty()) {
					err = true;
					st.data[i]->error = _("Times value not specified");
					break;
				}
			} else {
				if (timesFound) {
					err = true;
					st.data[i]->error = _("Garbage at the end of statement");
				} else {
					times << st.data[i];
				}
			}
		}
		if (err) {
			return;
		}
		AST::ExpressionPtr  timesExpr = parseExpression(times, st.mod, st.alg);
		if (!timesExpr) {
			return;
		} else if (timesExpr->baseType.kind != AST::TypeInteger) {
			foreach (LexemPtr l, times) l->error = _("Not integer times-value");
			return;
		}
		st.statement->loop.timesValue = timesExpr;
	} // end if (type==AST::LoopTimes)
	else if (type == AST::LoopWhile || type == AST::LoopForever) {
		if (st.data.size() == 1) {
			// Forever loop
			AST::ExpressionPtr  foreverr = AST::ExpressionPtr(new AST::Expression);
			foreverr->baseType.kind = AST::TypeBoolean;
			foreverr->kind = AST::ExprConst;
			foreverr->constant = QVariant(true);
			st.statement->loop.whileCondition = foreverr;
		} else {
			// While condition loop
			QList<LexemPtr> cond = st.data.mid(2);
			if (cond.isEmpty()) {
				st.data[1]->error = _("No condition after 'loop while'");
				return;
			}
			AST::ExpressionPtr  condExpr = parseExpression(cond, st.mod, st.alg);
			if (!condExpr) {
				return;
			} else if (condExpr->baseType.kind != AST::TypeBoolean) {
				foreach (LexemPtr l, cond) l->error = _("Condition is not boolean");
				return;
			}
			st.statement->loop.whileCondition = condExpr;
		}
	} // end if (type==AST::LoopWhile)
}

void SyntaxAnalizer::parseAssignment(int str)
{
	const TextStatement &st = statements_[str];
	if (st.hasError()) {
		return;
	}
	if (st.data.first()->type == LxSecFor) {
		st.data[0]->error = _("No 'loop' before 'for'");
		return;
	}
	if (st.data.first()->type == LxSecWhile) {
		st.data[0]->error = _("No 'loop' before 'while'");
		return;
	}
	if (st.data.first()->type == LxSecTimes) {
		st.data[0]->error = _("No 'loop' before 'times'");
		return;
	}
	AST::AlgorithmPtr  alg = st.alg;
	AST::ModulePtr  mod = st.mod;
	Q_CHECK_PTR(mod);
	QList<LexemPtr> left;
	QList<LexemPtr> right;
	LexemPtr assignOp;
	int assignCount = 0;
	for (int i = 0; i < st.data.size(); i++) {
		if (st.data[i]->type == LxPriAssign) {
			if (assignCount == 1) {
				st.data[i]->error = _("Too many ':=' operators");
				return;
			}
			assignCount++;
			assignOp = st.data[i];
		} else {
			if (assignCount == 0) {
				left << st.data[i];
			} else {
				right << st.data[i];
			}
		}
	}
	if (assignCount == 1) {
		if (left.isEmpty()) {
			assignOp->error = _("No left value of assignment");
			return;
		}
		if (right.isEmpty()) {
			assignOp->error = _("No right value of assignment");
			return;
		}
	} else {
		right = left;
		left.clear();
	}
	AST::ExpressionPtr  rightExpr = parseExpression(right, mod, alg);
	if (!rightExpr) {
		return;
	}
	if (left.isEmpty() && rightExpr->kind == AST::ExprFunctionCall &&
		rightExpr->baseType != AST::TypeNone) {
		// TODO in 2.1:
		// allow this!
		QString err = _("Expression not assigned to anything");
		if (rightExpr->operatorr == AST::OpEqual) {
			// Possible = instead of :=
			err = _("'=' instead of ':='");
		}
		for (int i = 0; i < right.size(); i++) {
			st.data[i]->error = err;
		}
		return;
	}
	if (left.isEmpty() && rightExpr->kind != AST::ExprFunctionCall) {
		if (rightExpr->kind == AST::ExprSubexpression && rightExpr->operatorr == AST::OpEqual) {
			LexemPtr lx;
			for (int i = 0; i < st.data.size(); i++) {
				if (st.data[i]->type == LxOperEqual) {
					lx = st.data[i];
					break;
				}
			}
			if (lx) {
				lx->error = _("'=' instead of ':='");
				return;
			}
		}
		const QString err = _("Expression not assigned to anything");
		for (int i = 0; i < right.size(); i++) {
			st.data[i]->error = err;
		}
		return;
	}
	AST::ExpressionPtr  leftExpr;
	if (!left.isEmpty()) {
		if (rightExpr->baseType.kind == AST::TypeNone) {
			if (rightExpr->kind != AST::ExprFunctionCall) {
				assignOp->error = _("Assignment of void");
			} else {
				const QString err = _("Assignment of non-returning algorithm");
				foreach (LexemPtr lx, right) {
					if (lx->type == LxOperLeftBr) {
						break;
					}
					lx->error = err;
				}
			}
			return;
		}
		leftExpr = parseExpression(left, mod, alg);
		if (!leftExpr) {
			return;
		}
		if (leftExpr->kind == AST::ExprFunctionCall) {
			assignOp->error = _("Assignment to function");
			return;
		}
		if (leftExpr->kind == AST::ExprSubexpression) {
			assignOp->error = _("Assignment to complex expression");
			return;
		}
		if (leftExpr->kind == AST::ExprConst) {
			assignOp->error = _("Assignment to constant value");
			return;
		}
		if ((leftExpr->kind == AST::ExprVariable) && (leftExpr->variable->dimension > 0)) {
			assignOp->error = _("Assignment to array");
			return;
		}
		if ((rightExpr->kind == AST::ExprVariable) && (rightExpr->variable->dimension > 0)) {
			assignOp->error = _("Assignment of array");
			return;
		}
		if ((leftExpr->kind == AST::ExprVariable || AST::ExprArrayElement == leftExpr->kind) && (leftExpr->variable->accessType == AST::AccessArgumentIn)) {
			assignOp->error = _("Assignment to in- argument");
			return;
		}
		QString err;
		const AST::Type &aa = leftExpr->baseType;
		const AST::Type &bb = rightExpr->baseType;
		const AST::VariableBaseType a = aa.kind;
		const AST::VariableBaseType b = bb.kind;
		if (aa != bb && (AST::TypeUser == a || AST::TypeUser == b)) {
			AST::ModulePtr  convMod;
			AST::AlgorithmPtr  convAlg;
			if (findConversionAlgorithm(bb, aa, convMod, convAlg, st.mod)) {
				AST::ExpressionPtr  convExpr = AST::ExpressionPtr(new AST::Expression);
				convExpr->kind = AST::ExprFunctionCall;
				convExpr->function = convAlg;
				convExpr->operands.push_back(rightExpr);
				convExpr->baseType = a;
				convExpr->dimension = leftExpr->dimension;
				rightExpr = convExpr;
			} else {
				const QString aName = AST::TypeUser == a
					? aa.name : lexer_->classNameByBaseType(a);
				const QString bName = AST::TypeUser == b
					? bb.name : lexer_->classNameByBaseType(b);

				err = _("Can't %1:=%2", aName, bName);
			}
		} else if (aa != bb) {
			if (a == AST::TypeInteger) {
				if (b == AST::TypeReal) {
					bool isRealConstant = false;
					static const QString realSpecificSymbols =
						QString::fromUtf8("еЕeE.");
					if (rightExpr->kind == AST::ExprConst) {
						QString strValue;
						foreach (const AST::LexemPtr lx, right) {
							strValue += lx->data;
						}
						for (int s = 0; s < realSpecificSymbols.length(); s++) {
							const QChar ss = realSpecificSymbols[s];
							if (strValue.contains(ss)) {
								isRealConstant = true;
								break;
							}
						}
					}
					bool isBigIntegerConstant = !isRealConstant &&
						rightExpr->kind == AST::ExprConst &&
						rightExpr->baseType.kind == AST::TypeReal;
					if (isBigIntegerConstant) {
						// Constant became real because of big integer representation
						err = _("Integer constant too big");
						foreach (LexemPtr lx, right) {
							lx->error = err;
						}
						return;
					} else {
						err = _("Can't integer:=real");
					}
				} else if (b == AST::TypeCharect) {
					err = _("Can't integer:=charect");
				} else if (b == AST::TypeString) {
					err = _("Can't integer:=string");
				} else if (b == AST::TypeBoolean) {
					err = _("Can't integer:=boolean");
				} else if (b == AST::TypeUser) {
					err = _("Can't %1:=%2", lexer_->classNameByBaseType(leftExpr->baseType.kind), rightExpr->baseType.name);
				}
			} else if (a == AST::TypeReal) {
				if (b == AST::TypeCharect) {
					err = _("Can't real:=charect");
				} else if (b == AST::TypeString) {
					err = _("Can't real:=string");
				} else if (b == AST::TypeBoolean) {
					err = _("Can't real:=boolean");
				} else if (b == AST::TypeUser) {
					err = _("Can't %1:=%2", lexer_->classNameByBaseType(leftExpr->baseType.kind), rightExpr->baseType.name);
				}
			} else if (a == AST::TypeCharect) {
				if (b == AST::TypeBoolean) {
					err = _("Can't charect:=boolean");
				} else if (b == AST::TypeInteger) {
					err = _("Can't charect:=integer");
				} else if (b == AST::TypeReal) {
					err = _("Can't charect:=real");
				} else if (b == AST::TypeString) {
					err = _("Can't charect:=string");
				} else if (b == AST::TypeUser) {
					err = _("Can't %1:=%2", lexer_->classNameByBaseType(leftExpr->baseType.kind), rightExpr->baseType.name);
				}
			} else if (a == AST::TypeBoolean) {
				if (b == AST::TypeInteger) {
					err = _("Can't boolean:=integer");
				} else if (b == AST::TypeReal) {
					err = _("Can't boolean:=real");
				} else if (b == AST::TypeCharect) {
					err = _("Can't boolean:=charect");
				} else if (b == AST::TypeString) {
					err = _("Can't boolean:=string");
				} else if (b == AST::TypeUser) {
					err = _("Can't %1:=%2", lexer_->classNameByBaseType(leftExpr->baseType.kind), rightExpr->baseType.name);
				}
			} else if (a == AST::TypeString) {
				if (b == AST::TypeBoolean) {
					err = _("Can't string:=boolean");
				} else if (b == AST::TypeInteger) {
					err = _("Can't string:=integer");
				} else if (b == AST::TypeReal) {
					err = _("Can't string:=real");
				} else if (b == AST::TypeUser) {
					err = _("Can't %1:=%2", lexer_->classNameByBaseType(leftExpr->baseType.kind), rightExpr->baseType.name);
				}
			}
		} // if (leftExpr->type!=rightExpr->type)
		if (!err.isEmpty()) {
			assignOp->error = err;
			return;
		}
	}
	st.statement->type = AST::StAssign;
	st.statement->expressions << rightExpr;
	if (leftExpr) {
		st.statement->expressions << leftExpr;
	}
}

void SyntaxAnalizer::parseAlgHeader(int str, bool onlyName, bool internalBuildFlag)
{
	const TextStatement &st = statements_[str];
	if (st.hasError() || !st.mod || !st.alg) {
		return;
	}
	AST::AlgorithmPtr  alg = st.alg;
	AST::ModulePtr  mod = st.mod;
	Q_CHECK_PTR(alg);
	Q_CHECK_PTR(mod);
	QString name;
	bool isOperator = false;
	bool isFirst =
		mod->header.name.isEmpty() &&
		mod->header.type == AST::ModTypeUserMain &&
		mod->impl.algorhitms.indexOf(alg) == 0;
	int argsStartLexem = -1;
	int nameStartLexem = 1;
	if (st.data.size() > 1 && st.data[1]->type == LxNameClass) {
		nameStartLexem = 2;
		AST::VariableBaseType bt = lexer_->baseTypeByClassName(st.data[1]->data);
		if (bt == AST::TypeNone) {
			AST::Type tp;
			AST::ModulePtr  typeMod;
			if (findUserType(st.data[1]->data, tp, typeMod, st.mod)) {
				alg->header.returnType = tp;
			} else {
				alg->header.returnType.kind = AST::TypeNone;
			}
		} else {
			alg->header.returnType.kind = bt;
		}
		if (lexer_->isArrayClassName(st.data[1]->data)) {
			alg->header.returnType.kind = AST::TypeNone;
			st.data[1]->error = _("Algorhitms can't return array");
			return;
		}

	}
	for (int i = nameStartLexem; i < st.data.size(); i++) {
		if (st.data[i]->type == LxNameClass || st.data[i]->type == LxConstBoolFalse || st.data[i]->type == LxConstBoolTrue) {
			st.data[i]->error = _("Name contains keyword");
			return;
		} else if (st.data[i]->type & LxTypeName) {
			if (i > nameStartLexem) {
				name += " ";
			}
			name += st.data[i]->data;
			st.data[i]->type = LxNameAlg;
		} else if (LxConstInteger == st.data[i]->type || LxConstReal == st.data[i]->type) {
			if (i == nameStartLexem) {
				st.data[i]->error = _("Name starts with digit");
				return;
			}
			if (st.data[i]->data.contains('.') ||
				st.data[i]->data.contains('+') ||
				st.data[i]->data.contains('-')
			) {
				st.data[i]->error = _("Bad symbol in name");
				return;
			}
			if (i > nameStartLexem) {
				name += " ";
				name += st.data[i]->data;
				st.data[i]->type = LxNameAlg;
			}
		} else if (st.data[i]->type == LxOperLeftBr) {
			argsStartLexem = i + 1;
			break;
		} else if (internalBuildFlag) {
			if (i > nameStartLexem) {
				name += " ";
			}
			name += st.data[i]->data;
			isOperator = true;
		} else if (st.data[i]->type == LxNameClass
			|| st.data[i]->type & LxTypePrimaryKwd
			|| st.data[i]->type & LxTypeSecondaryKwd) {
			st.data[i]->error = _("Keyword in name");
			return;
		} else if (st.data[i]->type & LxTypeOperator) {
			st.data[i]->error = _("Operator in name");
			return;
		}
	}

	if (isFirst) {
		if (name.isEmpty()) {
			if (argsStartLexem != -1) {
				for (int i = argsStartLexem - 1; i < st.data.size(); i++) {
					st.data[i]->error = _("Extra arguments");
				}
				return;
			}
			if (alg->header.returnType.kind != AST::TypeNone) {
				st.data[1]->error = _("First unnamed algorithm should not return anything");
				alg->header.returnType.kind = AST::TypeNone;
				return;
			}
		}
	} else {
		// если алгоритм не первый, обязательно должно быть имя, иначе -- ошибка
		if (name.isEmpty()) {
			for (int i = 0; i < st.data.size(); i++) {
				st.data[i]->error = _("No algorithm name");
			}
			return ;
		}
	}

	if (name.length() > 0) {
		// process generic name test
		const QString error = lexer_->testName(name);
		if (error.length() > 0) {
			const int start = nameStartLexem;
			const int end = argsStartLexem == -1
				? st.data.length() : argsStartLexem - 1;
			for (int i = start; i < end; i++) {
				st.data[i]->error = error;
			}
			return;
		}
	}

	// Заносим алгоритм в таблицу функций
	alg->header.name = name;
	bool nameHasError = false;

	// Проверяем на повторное описание алгоритма
	AST::AlgorithmPtr  aa;
	AST::ModulePtr mm;
	QVariantList aaTemplateParameters;
	if (!isOperator && findAlgorithm(name, st.mod, AST::AlgorithmPtr(), mm, aa, aaTemplateParameters) && aa != alg) {
		for (int i = 1; i < st.data.size(); i++) {
			if (st.data[i]->type == LxNameAlg) {
				st.data[i]->error = _("The name is used by other algorithm");
			}
		}
		nameHasError = true;
	}

	{
		AST::Type typee;
		QVariant cvalue;
		if (tryInputOperatorAlgorithm(name, typee, cvalue, st.mod)) {
			const QString error = _("Name is used by module %1", typee.actor->localizedModuleName(QLocale::Russian));
			for (int i = 1; i < st.data.size(); i++) {
				if (st.data[i]->type == LxNameAlg) {
					st.data[i]->error = error;
				}
			}
			nameHasError = true;
		}
	}

	// Проверяем на наличие переменной с таким же именем

	AST::VariablePtr  vv;
	if (!nameHasError && !isOperator && findGlobalVariable(name, st.mod, vv)) {
		for (int i = 1; i < st.data.size(); i++) {
			if (st.data[i]->type == LxNameAlg) {
				st.data[i]->error = _("The name is used by global variable");
			}
		}
		nameHasError = true;
	}

	if (alg->header.returnType.kind != AST::TypeNone  && onlyName) {
		AST::VariablePtr  retVar = AST::VariablePtr(new AST::Variable);
		retVar->accessType = AST::AccessRegular;
		retVar->baseType = alg->header.returnType;
		retVar->name = name;
		alg->impl.locals << retVar;
	}


	if (onlyName) {
		// Make this algorhitm public available (if not private name)
		if (!name.isEmpty() && !name.startsWith("_")) {
			if (isOperator) {
				mod->header.operators << alg;
			} else {
				mod->header.algorhitms << alg;
			}
		}
	}



	for (int i = nameStartLexem; i < argsStartLexem - 1; i++) {
		st.data[i]->type = LxNameAlg;
	}

	if (/*alg->header.specialType==AST::AlgorhitmTypeTeacher && */alg->header.name == lexer_->testingAlgorhitmName()) {
		alg->header.specialType = AST::AlgorithmTypeTesting;
	}

	const bool teacherModule = AST::ModTypeTeacher == mod->header.type || AST::ModTypeTeacherMain == mod->header.type;
	const bool allowTeacherModeAlgorithms = teacherModule || teacherMode_ || internalBuildFlag;

	if (allowTeacherModeAlgorithms && alg->header.name == lexer_->testingAlgorhitmName()) {
		alg->header.specialType = AST::AlgorithmTypeTesting;
	} else if (allowTeacherModeAlgorithms && alg->header.name.startsWith("@")) {
		alg->header.specialType = AST::AlgorithmTypeTeacher;
	}

	if (!allowTeacherModeAlgorithms && alg->header.name.startsWith("@")) {
		for (int i = 1; i < st.data.size(); i++) {
			if (st.data[i]->type == LxNameAlg) {
				st.data[i]->error = _("Incorrect algorithm name");
			}
		}
		nameHasError = true;
	}

	if (onlyName || nameHasError) {
		alg->header.broken = nameHasError;
		return;
	}


	// Если нет аргументов, то всё

	if (argsStartLexem == -1) {
		return;
	}

	// =============== Argument list parsing

	alg->header.broken = true;

	QList<VariablesGroup> groups;
	if (st.data.last()->type != LxOperRightBr) {
		alg->header.error = st.data[argsStartLexem - 1]->error = _("Unpaired '('");
		return;
	}
	VariablesGroup currentGroup;
	currentGroup.access = AST::AccessArgumentIn;
	currentGroup.accessDefined = false;
	for (int i = argsStartLexem; i < st.data.size() - 1; i++) {
		if (st.data[i]->type == LxTypeComment) {
			break;
		}
		LexemType type = st.data[i]->type;
		if (type == LxSecIn || type == LxSecOut || type == LxSecInout) {
			if (!currentGroup.lexems.isEmpty() && currentGroup.lexems.last()->type == LxOperComa) {
				currentGroup.lexems.pop_back();
			} else if (!currentGroup.lexems.isEmpty()) {
				alg->header.error = st.data[i]->error = _("No coma before %1", st.data[i]->data);
				return;
			}
			if (!currentGroup.lexems.isEmpty()) {
				groups << currentGroup;
				currentGroup.lexems.clear();
				currentGroup.accessDefined = false;
			}
			if (type == LxSecIn) {
				if (currentGroup.accessDefined) {
					alg->header.error = st.data[i]->error = _("Extra variable group in-specifier");
				} else {
					currentGroup.accessDefined = true;
					currentGroup.access = AST::AccessArgumentIn;
				}
			} else if (type == LxSecOut) {
				if (currentGroup.accessDefined) {
					alg->header.error = st.data[i]->error = _("Extra variable group out-specifier");
				} else {
					currentGroup.accessDefined = true;
					currentGroup.access = AST::AccessArgumentOut;
				}
			} else if (type == LxSecInout) {
				if (currentGroup.accessDefined) {
					alg->header.error = st.data[i]->error = _("Extra variable group inout-specifier");
				} else {
					currentGroup.accessDefined = true;
					currentGroup.access = AST::AccessArgumentInOut;
				}
			}
			currentGroup.groupLexem = st.data[i];
		} else if (type == LxPriAlgHeader) {
			alg->header.error = st.data[i]->error = _("'alg' instead of 'arg'");
			return;
		} else if (type == LxOperComa && currentGroup.lexems.isEmpty()) {
			if (currentGroup.groupLexem == 0) {
				alg->header.error = st.data[i]->error = _("Extra ','");
				return;
			} else {
				alg->header.error = currentGroup.groupLexem->error = _("No variables declared after '%1'", currentGroup.groupLexem->data);
				return;
			}
		} else {
			currentGroup.lexems << st.data[i];
		}
	}
	if (!currentGroup.lexems.isEmpty()) {
		groups << currentGroup;
	}

	QString localError;

	if (groups.isEmpty()) {
		const QString err = _("Extra brackets for algorithm without arguments");
		alg->header.error = st.data[argsStartLexem - 1]->error = st.data.last()->error = err;
		return;
	}

	for (int i = 0; i < groups.size(); i++) {
		QList<AST::VariablePtr> vars = parseVariables(str, groups[i], st.mod, st.alg, true);
		for (int j = 0; j < vars.size(); j++) {
			alg->header.arguments << vars[j];
		}
		bool mustStop = false;
		for (int j = 0; j < groups[i].lexems.size() - 1; j++) {
			if (!groups[i].lexems[j]->error.isEmpty()) {
				localError = groups[i].lexems[j]->error;
				mustStop = true;
				break;
			}
		}
		if (mustStop) {
			break;
		}
	}
	alg->header.broken = localError.length() > 0;
	alg->header.error = localError;
}

bool hasFunction(const AST::ExpressionPtr  expr, QList<LexemPtr> &lexems)
{
	if (expr->kind == AST::ExprFunctionCall) {
		lexems += expr->lexems;
		return true;
	} else if (expr->kind == AST::ExprSubexpression) {
		foreach (const AST::ExpressionPtr  sub, expr->operands) {
			if (hasFunction(sub, lexems)) {
				return true;
			}
		}
	}
	return false;
}

bool hasArrayElement(const AST::ExpressionPtr  expr, QList<LexemPtr> &lexems)
{
	if (expr->kind == AST::ExprArrayElement) {
		lexems += expr->lexems;
		return true;
	} else if (expr->kind == AST::ExprSubexpression) {
		foreach (const AST::ExpressionPtr  sub, expr->operands) {
			if (hasArrayElement(sub, lexems)) {
				return true;
			}
		}
	}
	return false;
}

QList<AST::VariablePtr> SyntaxAnalizer::parseVariables(int statementIndex, VariablesGroup &group, AST::ModulePtr mod, AST::AlgorithmPtr alg, bool algHeader)
{
	// First -- check brackets

	QStack<LexemPtr> openBrackets;
	QStack<LexemPtr> openSqBrackets;
	QStack<LexemPtr> openBraces;

	const QList<LexemPtr> lexems = group.lexems;


	for (int i = 0; i < lexems.size(); i++) {
		LexemPtr lx = lexems.at(i);
		if (lx->type == LxOperLeftBr) {
			openBrackets.push_back(lx);
		} else if (lx->type == LxOperLeftSqBr) {
			openSqBrackets.push_back(lx);
		} else if (lx->type == LxOperLeftBrace) {
			openBraces.push_back(lx);
		} else if (lx->type == LxOperRightBr) {
			if (openBrackets.size() == 0) {
				lx->error = _("Unpaired )");
				return QList<AST::VariablePtr>();
			} else {
				openBrackets.pop_back();
			}
		} else if (lx->type == LxOperRightSqBr) {
			if (openSqBrackets.size() == 0) {
				lx->error = _("Unpaired ]");
				return QList<AST::VariablePtr>();
			} else {
				openSqBrackets.pop_back();
			}
		} else if (lx->type == LxOperRightBrace) {
			if (openBraces.size() == 0) {
				lx->error = _("Unpaired }");
				return QList<AST::VariablePtr>();
			} else {
				openBraces.pop_back();
			}
		}
	}

	foreach (LexemPtr lx, openBrackets) {
		lx->error = _("Unpaired (");
	}
	foreach (LexemPtr lx, openSqBrackets) {
		lx->error = _("Unpaired [");
	}
	foreach (LexemPtr lx, openBraces) {
		lx->error = _("Unpaired {");
	}

	if (openBrackets.size() + openSqBrackets.size() + openBraces.size() > 0) {
		return QList<AST::VariablePtr>();
	}

	//  Pежим работы автомата-парсера.
	//  type -- разбор типа
	//  name -- разбор имени переменной
	//  tn -- разбор типа или имени
	//  tab -- разбор массива
	//  lbN -- разбор левой границы
	//  lbN -- разбор правой границы
	//  value -- разбор константного значения

	enum Parcing { type, name, tn, lb, rb, value };

	//  Состояния автомата
	Parcing par = type; // режим разбора
	AST::Type cType; // тип текущей величины
	cType.kind = AST::TypeNone;
	bool array = false;
	QString cName = ""; // имя текущей величины
	QList<LexemPtr> cBound; // текущая граница массива
	QList<LexemPtr> initValue; // значение инициализации
	QList<AST::Variable::Bound> bounds;
	int deepV = 0; // глубина вложенности {}
	int deep1 = 0; // глубина вложенности [] внутри правой границы
	int deep2 = 0; // глубина вложенности () внутри правой границы
	int dim = 0;
	bool massDeclared = false;
	AST::VariablePtr  var;
	QList<AST::VariablePtr> result;

	int z = 0;
	int nameStart = 0;
	int arrayStart = 0;
	int arrayBoundStart = 0;
	int typePos = 0;

	LexemPtr endMark(new Lexem);
	endMark->type = LxOperSemicolon;
	group.lexems << endMark;

	for (int curPos = 0; curPos < group.lexems.size(); curPos++) {
		//      Предполагается, что модификатор группы уже задан, поэтому
		//      если встречается арг, рез или аргрез, то это ошибка
		if (group.lexems[curPos]->type == LxSecIn) {
			group.lexems[curPos]->error = _("Extra variable group in-specifier");
			return result;
		} else if (group.lexems[curPos]->type == LxSecOut) {
			group.lexems[curPos]->error = _("Extra variable group out-specifier");
			return result;
		} else if (group.lexems[curPos]->type == LxSecInout) {
			group.lexems[curPos]->error = _("Extra variable group inout-specifier");
			return result;
		} else if ((par == type) || (par == tn)) {
			// Check if no coma before not first declaration
			if (curPos > 0
				&& group.lexems[curPos]->type != LxOperComa
				&& group.lexems[curPos]->type != LxOperSemicolon
				&& group.lexems[curPos]->type != LxOperEqual
				&& group.lexems[curPos - 1]->type != LxOperComa
				&& group.lexems[curPos - 1]->type != LxOperSemicolon
			) {
				group.lexems[curPos]->error = _("No coma before declaration");
				return result;
			}

			//          Pазбираемся с типом переменных
			if (group.lexems[curPos]->type == LxNameClass) {
				AST::VariableBaseType bt = lexer_->baseTypeByClassName(group.lexems[curPos]->data);
				if (bt == AST::TypeNone) {
					AST::Type userType;
					AST::ModulePtr  userTypeModule;
					if (findUserType(group.lexems[curPos]->data, userType, userTypeModule, mod)) {
						cType = userType;
					}
				} else {
					cType.kind = bt;
				}
				array = lexer_->isArrayClassName(group.lexems[curPos]->data);
				//              Переходим в режим поиска имени
				par = name;
				cName = "";
				nameStart = curPos + 1;
				typePos = curPos;
				continue;
			} else if (par == tn) {
				par = name;
			} else {
				// не указан тип переменной
				if (typePos != 0) {
					group.lexems[typePos]->error = _("Type not declared before");
				} else {
					for (int a = 0; a < curPos + 1; a++) {
						group.lexems[a]->error = _("Type not declared before");
					}
				}
				return result;
			}
		} // endif ( par == type || par == tn )
		if (par == name) {
			if (group.lexems[curPos]->type == LxNameClass) {
				group.lexems[curPos]->error = cName.isEmpty()
					? _("Extra type")
					: _("No coma before type");
				return result;
			}
			if (group.lexems[curPos]->data == QString::fromLatin1("$") || group.lexems[curPos]->data == QString::fromLatin1(".")) {
				group.lexems[curPos]->type = LxTypeEmpty;
				group.lexems[curPos]->error = _("Bad symbol in name");
				return result;
			}
			if (group.lexems[curPos]->type & LxTypeConstant) {
				group.lexems[curPos]->error = _("Constant can not be a name");
				return result;
			}
			if (group.lexems[curPos]->type == LxOperRightSqBr) {
				group.lexems[curPos]->error = _("No pairing '['");
				return result;
			}
			if (group.lexems[curPos]->type == LxOperLeftBrace) {
				int searchPos = curPos;
				int deep = 0;
				for (; searchPos < group.lexems.size(); searchPos++) {
					const LexemPtr lx = group.lexems[searchPos];
					if (lx->type == LxOperLeftBrace) {
						deep++;
					}
					if (lx->type == LxOperRightBrace) {
						deep--;
						if (deep == 0) {
							break;
						}
					}
				}
				for (int i = curPos; i <= searchPos; i++) {
					group.lexems[i]->error = _("Must be a variable name");
				}
				return result;
			}
			if (group.lexems[curPos]->type == LxOperLeftSqBr) {
				if (massDeclared) {
					group.lexems[curPos]->error = _("Extra '['");
					return result;
				}
				if (cName.trimmed().isEmpty()) {
					group.lexems[curPos]->error = group.lexems[curPos - 1]->error = _("No variable name");
					return result; // нет имени переменной
				} else {
					if (!array) {
						for (int a = curPos; a < group.lexems.size(); a++) {
							group.lexems[a]->error = _("Extra array bound");
							if (group.lexems[a]->type == LxOperRightSqBr) {
								break;
							}
						}
						return result; // границы не нужны
					} else {
						cBound.clear();
						par = lb;
						arrayStart = curPos;
						arrayBoundStart = curPos;
					}
				}
			} // if ( '[' )
			else if (group.lexems[curPos]->type == LxPriAssign) {
				group.lexems[curPos]->error = _("':=' should not be here");
				return result;
			} else if (group.lexems[curPos]->type == LxOperComa || group.lexems[curPos]->type == LxOperSemicolon || group.lexems[curPos]->type == LxOperEqual) {
				if (cName.trimmed().isEmpty()) {
					int index = typePos;
					if (nameStart - 1 >= 0) {
						index = nameStart - 1;
					}
					group.lexems[index]->error = _("Variable name is empty");
					return result;
				}
				QString loc_err = lexer_->testName(cName);
				if (!loc_err.isEmpty()) {
					int endPos = curPos;
					if (arrayStart > nameStart) {
						endPos = arrayStart;
					}
					for (int k = nameStart; k < endPos; k++) {
						group.lexems[k]->error = loc_err;
					}
					return result;
				}
				if (cName.count("\"") || cName.count("'")) {
					group.lexems[nameStart]->error = _("Constant instead of name");
					return result;
				}
				cName = cName.trimmed();
				//                if ( cName == QObject::trUtf8("знач") )
				//                    return PV_ZNACH_NAME;
				AST::VariablePtr  vv;
				if (alg) {
					if (findLocalVariable(cName, alg, vv)) {
						group.lexems[nameStart]->error = _("Variable already declared");
						return result; // переменная уже объявлена
					}
				} else {
					if (findGlobalVariable(cName, mod, vv)) {
						group.lexems[nameStart]->error = _("Variable already declared");
						return result; // переменная уже объявлена
					}
				}

				AST::AlgorithmPtr  aa;
				AST::ModulePtr aaModule;
				QVariantList aaTemplateParameters;
				if (findAlgorithm(cName, mod, alg, aaModule, aa, aaTemplateParameters)) {
					group.lexems[nameStart]->error = _("Name is used by algorithm");
					return result;
				}
				if (array && !massDeclared /*&& group.lexems[curPos]->type!=LxOperEqual*/) {
					group.lexems[nameStart]->error = _("Array bounds no specified");
					return result; // нет границ
				}
				AST::Type typee;
				QVariant cvalue;
				if (tryInputOperatorAlgorithm(cName, typee, cvalue, mod)) {
					group.lexems[nameStart]->error = _("Name is used by module %1", typee.actor->localizedModuleName(QLocale::Russian));
					return result;
				}
				var = AST::VariablePtr(new AST::Variable);
				var->name = cName;
				var->baseType = cType;
				var->accessType = group.access;
				var->dimension = array ? dim : 0;
				var->bounds = bounds;
				result << var;
				statements_[statementIndex].variables << var;
				if (alg) {
					alg->impl.locals << var;
				} else {
					mod->impl.globals << var;
				}
				cName = "";
				par = tn;
				dim = 0;
				massDeclared = false;
				nameStart = curPos + 1;
				if (group.lexems[curPos]->type == LxOperEqual) {
					par = value;
					z = curPos + 1;
				}
				bounds.clear();
			} else if (group.lexems[curPos]->type & LxTypePrimaryKwd ||
				group.lexems[curPos]->type & LxTypeSecondaryKwd) {
				group.lexems[curPos]->error = _("Name contains keyword");
				return result;
			} else {
				if (!cName.isEmpty()) {
					cName += " ";
				}
				cName += group.lexems[curPos]->data;
			}
		} // endif ( par == name || par == tn )
		else if (par == lb) {
			if (group.lexems[curPos]->type == LxOperLeftSqBr) {
				deep1 ++;
				cBound << group.lexems[curPos];
			} else if (group.lexems[curPos]->type == LxOperLeftBr) {
				deep2 ++;
				cBound << group.lexems[curPos];
			} else if (group.lexems[curPos]->type == LxOperRightSqBr && deep1 > 0) {
				deep1 --;
				cBound << group.lexems[curPos];

			} else if (group.lexems[curPos]->type == LxOperRightBr) {
				deep2 --;
				cBound << group.lexems[curPos];
			} else if (group.lexems[curPos]->type == LxOperComa && deep2 == 0 && deep1 == 0) {
				if (cBound.isEmpty()) {
					for (int a = arrayStart; a <= curPos; a++) {
						group.lexems[a]->error = _("No left array bound");
					}
					return result;
				} else {
					group.lexems[curPos]->error = _("',' instead of ':'");
					return result;
				}
			}
			//          Pазбор левой границы массива
			else if (group.lexems[curPos]->type == LxOperSemicolon) {
				group.lexems[arrayStart]->error = _("No pairing ']'");
				return result;
			} else if (group.lexems[curPos]->type == LxOperColon) {

				if (cBound.isEmpty()) {
					for (int a = arrayStart; a <= curPos; a++) {
						group.lexems[a]->error = _("Left array bound not specified");
					}
					return result;
				}


				AST::ExpressionPtr  left = parseExpression(cBound, mod, alg);


				if (left == 0) { // error
					return result; // ошибка разбора левой границы
				}

				if (algHeader) {
					QList<LexemPtr> leftFunction;
					QList<LexemPtr> leftArray;

					if (hasFunction(left, leftFunction)) {
						foreach (LexemPtr a, leftFunction) {
							a->error = _("Function in array bound");
						}
						return result;
					}
					if (hasArrayElement(left, leftArray)) {
						foreach (LexemPtr a, leftArray) {
							a->error = _("Array element in array bound");
						}
						return result;
					}
				}

				if (left->baseType.kind != AST::TypeInteger) {
					for (int a = arrayBoundStart; a <= curPos; a++) {
						group.lexems[a]->error = _("Left array bound is not integer");
					}
					return result; // левая граница -- не целое число
				}

				QString pattern = "\\b" + cName + "\\b";
				if (alg && !alg->header.name.isEmpty()) {
					pattern += "|\\b" + alg->header.name + "\\b";
				}
				QRegExp rxRecursion = QRegExp(pattern);
				for (int a = 0; a < cBound.size(); a++) {
					if (rxRecursion.indexIn(cBound[a]->data) != -1) {
						cBound[a]->error = _("Recursive declaration of bound");
						return result;
					}
				}
				QPair<AST::ExpressionPtr, AST::ExpressionPtr> bound;
				bounds.append(bound);
				bounds.last().first = left;
				cBound.clear();
				par = rb;
				arrayBoundStart = curPos;
			} else if (group.lexems[curPos]->type == LxOperRightSqBr && deep1 == 0) {
				group.lexems[curPos]->error = cBound.isEmpty()
					? _("Left array bound is empty")
					: _("Right array bound is empty");
				return result;
			} else {
				cBound << group.lexems[curPos];
			}
		} // endif ( par == lb )
		else if (par == rb) {
			if (group.lexems[curPos]->type == LxOperLeftSqBr) {
				deep1 ++;
				cBound << group.lexems[curPos];
			} else if (group.lexems[curPos]->type == LxOperLeftBr) {
				deep2 ++;
				cBound << group.lexems[curPos];
			} else if (group.lexems[curPos]->type == LxOperRightSqBr && deep1 > 0) {
				deep1 --;
				cBound << group.lexems[curPos];

			} else if (group.lexems[curPos]->type == LxOperRightBr) {
				deep2 --;
				cBound << group.lexems[curPos];
			} else if (group.lexems[curPos]->type == LxOperComa && deep1 == 0 && deep2 == 0) {
				if (dim > 2) {
					for (int a = arrayStart; a < group.lexems.size(); a++) {
						group.lexems[a]->error = _("Array dimension must not be > 3");
						if (group.lexems[a]->type == LxOperRightSqBr) {
							break;
						}
					}
					return result; // размерность > 3
				}
				dim++;

				AST::ExpressionPtr  right = parseExpression(cBound, mod, alg);
				if (right == 0) {
					return result; // ошибка разбора левой границы
				}
				if (algHeader) {
					QList<LexemPtr> rightFunction;
					QList<LexemPtr> rightArray;

					if (hasFunction(right, rightFunction)) {
						foreach (LexemPtr a, rightFunction) {
							a->error = _("Function in array bound");
						}
						return result;
					}

					if (hasArrayElement(right, rightArray)) {
						foreach (LexemPtr a, rightArray) {
							a->error = _("Array element in array bound");
						}
						return result;
					}
				}


				if (right->baseType.kind != AST::TypeInteger) {
					for (int a = arrayBoundStart; a <= curPos; a++) {
						group.lexems[a]->error = _("Right array bound is not integer");
					}
					return result; // правая граница -- не целое число
				}

				QString pattern = "\\b" + cName + "\\b";
				if (alg && !alg->header.name.isEmpty()) {
					pattern += "|\\b" + alg->header.name + "\\b";
				}
				QRegExp rxRecursion = QRegExp(pattern);
				for (int a = 0; a < cBound.size(); a++) {
					if (rxRecursion.indexIn(cBound[a]->data) != -1) {
						cBound[a]->error = _("Recursive declaration of bound");
						return result;
					}
				}
				cBound.clear();
				par = lb;
				arrayBoundStart = curPos;
				bounds.last().second = right;
			} else if (group.lexems[curPos]->type == LxOperRightSqBr && deep1 == 0) {
				if (dim > 2) {
					for (int a = arrayStart; a < group.lexems.size(); a++) {
						group.lexems[a]->error = _("Array dimension must not be > 3");
						if (group.lexems[a]->type == LxOperRightSqBr) {
							break;
						}
					}
					return result;
				}

				dim++;

				if (cBound.isEmpty()) {
					for (int a = arrayBoundStart; a <= curPos; a++) {
						group.lexems[a]->error = _("No right array bound");
					}
					return result;
				}

				AST::ExpressionPtr  right = parseExpression(cBound, mod, alg);

				if (right == 0) {
					return result;
				}

				if (algHeader) {
					QList<LexemPtr> rightFunction;
					QList<LexemPtr> rightArray;

					if (hasFunction(right, rightFunction)) {
						foreach (LexemPtr a, rightFunction) {
							a->error = _("Function in array bound");
						}
						return result;
					}

					if (hasArrayElement(right, rightArray)) {
						foreach (LexemPtr a, rightArray) {
							a->error = _("Array element in array bound");
						}
						return result;
					}
				}
				if (right->baseType.kind != AST::TypeInteger) {
					for (int a = arrayBoundStart + 1; a < curPos; a++) {
						group.lexems[a]->error = _("Right array bound is not integer");
					}
					return result; // правая граница -- не целое число
				}
				QString pattern = "\\b" + cName + "\\b";
				if (alg && !alg->header.name.isEmpty()) {
					pattern += "|\\b" + alg->header.name + "\\b";
				}
				QRegExp rxRecursion = QRegExp(pattern);
				for (int a = 0; a < cBound.size(); a++) {
					if (rxRecursion.indexIn(cBound[a]->data) != -1) {
						cBound[a]->error = _("Recursive declaration of bound");
						return result;
					}
				}
				par = tn;
				massDeclared = true;
				bounds.last().second = right;

			} else if (group.lexems[curPos]->type == LxOperSemicolon) {
				const QString err = _("No pairing ']'");

				group.lexems[arrayStart]->error = err;

				return result;
			} else {
				cBound << group.lexems[curPos];
			}
		} // endif ( par == rb )
		else if (par == value) {
			if (group.access != AST::AccessRegular) {
				group.lexems[z - 1]->error = _("Initial values should not be specified in algorithm header");
				return result;
			}
			if (group.lexems[curPos]->type == LxOperLeftBrace) {
				deepV ++;
			} else if (group.lexems[curPos]->type == LxOperRightBrace) {
				deepV --;
			} else if (LxOperLeftSqBr == group.lexems[curPos]->type && var->dimension > deepV) {
				group.lexems[curPos]->error = _("[ instead of {");
				return result;
			}
			if (deepV == 0 && (group.lexems[curPos]->type == LxOperComa || group.lexems[curPos]->type == LxOperSemicolon)) {

				if (initValue.isEmpty()) {
					group.lexems[z - 1]->error = _("Initial value is empty");
					return result;
				}
				int maxDim = 0;
				QVariant constValue = parseConstant(initValue.toStdList(),
						var->baseType.kind,
						maxDim);
				if (constValue == QVariant::Invalid) {
					// Try to parse constant of custom typee
					AST::Type userConstType;
					QVariant userConstValue;
					QString longLexem;
					bool success = false;
					foreach (const LexemPtr lx, initValue) {
						if (longLexem.length() > 0) {
							longLexem += " ";
						}
						longLexem += lx->data;
					}
					if (tryInputOperatorAlgorithm(longLexem, userConstType, userConstValue, mod) &&
						userConstType == cType
					) {
						constValue = userConstValue;
						foreach (LexemPtr lx, initValue) {
							lx->type = Shared::LxTypeConstant;
							lx->error.clear();
						}
						success = true;
					} else if (longLexem.contains('(')) {
						// Trying to call function
						bool bracketFound = false;
						foreach (const LexemPtr lx, initValue) {
							if (Shared::LxOperLeftBr == lx->type) {
								bracketFound = true;
							}
							if (bracketFound) {
								lx->error.clear();
							} else {
								lx->error = _("Can't call function while initialization");
							}
						}
					}
					if (!success) {
						return result;
					}
				}
				if (QVariant::Double == constValue.type()
					&& AST::TypeInteger == cType.kind) {
					static const QString realSpecificSymbols =
						QString::fromUtf8("еЕeE.");
					bool isRealConstant = false;
					QString strValue;
					foreach (const AST::LexemPtr clx, initValue) {
						if (strValue.length() > 0) {
							strValue.append(' ');
						}
						strValue += clx->data;
					}

					for (int ss = 0; ss < realSpecificSymbols.length(); ss++) {
						if (strValue.contains(realSpecificSymbols[ss])) {
							isRealConstant = true;
							break;
						}
					}
					if (!isRealConstant) {
						foreach (AST::LexemPtr clx, initValue) {
							clx->error = _("Integer constant too big");
						}
						return result;
					}
				}
				if (var->dimension == 0 && lexer_->isArrayClassName(group.lexems[typePos]->data)) {
					if (maxDim > 3) {
						for (int a = z - 1; a <= curPos; a++) {
							group.lexems[a]->error = _("Table dimension > 3");
							return result;
						}
					}
					var->dimension = maxDim;
				}
				if (var->dimension > 0) {
					if (var->dimension != maxDim) {
						for (int a = z - 1; a <= curPos; a++) {
							group.lexems[a]->error = _("Unmatched array dimensions");
							return result;
						}
					}
				} else if (maxDim > 0) {
					for (int a = z - 1; a <= curPos; a++) {
						group.lexems[a]->error = _("Must be a scalar constant");
						return result;
					}
				}
				var->initialValue = constValue;

				par = tn;
				initValue.clear();
			} else if (deepV != 0 && group.lexems[curPos]->type == LxOperSemicolon) {
				if (deepV > 0) {
					// extra open
					for (int i = z; i < curPos; i++) {
						if (group.lexems[i]->type == LxOperLeftBrace) {
							group.lexems[i]->error = _("Extra open brace");
							return result;
						}
					}
				} else {
					// extra close
					for (int i = curPos - 1; i > z; i--) {
						if (group.lexems[i]->type == LxOperRightBrace) {
							group.lexems[i]->error = _("Extra close brace");
							return result;
						}
					}
				}
			} else {
				initValue << group.lexems[curPos];
			}
		} // endif ( par==value )
	}
	return result;
}

QVariant SyntaxAnalizer::parseConstant(const std::list<LexemPtr> &constant,
	const AST::VariableBaseType pt,
	int &maxDim
) const
{
	int localErr = 0;
	AST::VariableBaseType ct;
	maxDim = 0;
	if (constant.size() == 0) {
		return QVariant::Invalid;
	}
	if (constant.size() == 1 && constant.front()->data.trimmed() == "..." && constant.front()->type != LxConstLiteral) {
		constant.front()->error = _("Constant value not typed");
		return QVariant::Invalid;
	}
	std::stack <LexemPtr> openBrackets;
	std::stack <LexemPtr> openSqBrackets;
	std::stack <LexemPtr> openBraces;

	for (std::list<LexemPtr>::const_iterator it = constant.begin(); it != constant.end(); ++it) {
		LexemPtr lx = *it;
		if (lx->type == LxOperLeftBr) {
			openBrackets.push(lx);
		} else if (lx->type == LxOperLeftSqBr) {
			openSqBrackets.push(lx);
		} else if (lx->type == LxOperLeftBrace) {
			openBraces.push(lx);
		} else if (lx->type == LxOperRightBr) {
			if (openBrackets.size() == 0) {
				lx->error = _("Unpaired )");
				return QVariant::Invalid;
			} else {
				openBrackets.pop();
			}
		} else if (lx->type == LxOperRightSqBr) {
			if (openSqBrackets.size() == 0) {
				lx->error = _("Unpaired ]");
				return QVariant::Invalid;
			} else {
				openSqBrackets.pop();
			}
		} else if (lx->type == LxOperRightBrace) {
			if (openBraces.size() == 0) {
				lx->error = _("Unpaired }");
				return QVariant::Invalid;
			} else {
				openBraces.pop();
			}
		}
	}
	bool error = openBrackets.size() + openSqBrackets.size() + openBraces.size() > 0;
	while (openBrackets.size() > 0) {
		LexemPtr lx = openBrackets.top();
		lx->error = _("Unpaired (");
		openBrackets.pop();
	}
	while (openSqBrackets.size() > 0) {
		LexemPtr lx = openSqBrackets.top();
		lx->error = _("Unpaired [");
		openSqBrackets.pop();
	}
	while (openBraces.size() > 0) {
		LexemPtr lx = openBraces.top();
		lx->error = _("Unpaired {");
		openBraces.pop();
	}
	if (error) {
		return QVariant::Invalid;
	}
	bool array = false;
	if (constant.front()->type == LxOperLeftBrace) {
		if (constant.back()->type == LxOperRightBrace) {
			array = true;
		} else {
			for (std::list<LexemPtr>::const_reverse_iterator it = constant.rbegin(); it != constant.rend(); ++it) {
				LexemPtr lx = *it;
				if (lx->type == LxOperRightBrace) {
					break;
				}
				lx->error = _("Garbage after }");
			}
			return QVariant::Invalid;
		}
	}
	if (constant.back()->type == LxOperRightBrace) {
		if (constant.front()->type == LxOperLeftBrace) {
			array = true;
		} else {
			constant.back()->error = _("Extra close brace");
			return QVariant::Invalid;
		}
	}
	if (!array) {
		maxDim = 0;
		ct = testConst(constant, localErr);
		if (ct == AST::TypeNone) {
			QString errorText = _("Not a constant value");
			if (constant.size() > 1) {
				bool allLiterals = true;
				for (std::list<LexemPtr>::const_iterator it = constant.begin(); it != constant.end(); it++) {
					LexemPtr lx = *it;
					if (LxConstLiteral != lx->type) {
						allLiterals = false;
						break;
					}
				}
				if (allLiterals) {
					errorText = _("Too many qoutes in constant");
				}
			}
			for (std::list<LexemPtr>::const_iterator it = constant.begin(); it != constant.end(); it++) {
				LexemPtr lx = *it;
				lx->error = errorText;
			}
			return QVariant::Invalid;
		} else if ((ct == pt) ||
			(ct == AST::TypeInteger && pt == AST::TypeReal) ||
			(ct == AST::TypeCharect && pt == AST::TypeString)) {
			QString val;
			for (std::list<LexemPtr>::const_iterator it = constant.begin(); it != constant.end(); it++) {
				val += (*it)->data;
			}
			bool integerOverflow = false;
			bool isHex = false;

			if (pt == AST::TypeInteger) {
				integerOverflow = !Kumir::Converter::isCorrectIntegerConstant(val.toStdWString());
				isHex = val.startsWith("$") || val.startsWith("-$") || val.startsWith("0x") || val.startsWith("-0x");
				if (!isHex) {
					// Check for leading zeroes
					int startPos = 0;
					std::list<LexemPtr>::const_iterator it = constant.begin();
					if (val.startsWith('-')) {
						startPos += 1;
						it ++;
					}
					const QString digits = val.mid(startPos);
					if (digits != "0" && digits.startsWith("0")) {
						for (; it != constant.end(); it++) {
							LexemPtr lx = * it;
							lx->error = _("Leading zeroes not allowed in constants");
							return QVariant::Invalid;
						}
					}

				}
			}
			if (ct == AST::TypeReal ||
				(pt == AST::TypeReal || (integerOverflow && !isHex))
			) {
				ct = AST::TypeReal;
				Kumir::Converter::ParseError local_error = Kumir::Converter::NoError;
				Kumir::real value = Kumir::Converter::parseReal(val.toStdWString(), Kumir::Char('.'), local_error);
				QString err;
				if (local_error == Kumir::Converter::WrongReal) {
					err = _("Wrong real number");
				} else if (local_error == Kumir::Converter::WrongExpForm) {
					err = _("Wrong E-real number");
				} else if (local_error == Kumir::Converter::BadSymbol) {
					err = _("Wrong real number");
				} else if (local_error == Kumir::Converter::Overflow) {
					err = _("Too big real value");
				}
				if (err.length() > 0) {
					for (std::list<LexemPtr>::const_iterator it = constant.begin(); it != constant.end(); it++) {
						LexemPtr lx = * it;
						lx->error = err;
						return QVariant::Invalid;
					}
				}
				return QVariant(value);
			}

			if (pt == AST::TypeInteger && integerOverflow) {
				if (val.startsWith("$")) {
					for (std::list<LexemPtr>::const_iterator it = constant.begin(); it != constant.end(); it++) {
						LexemPtr lx = * it;
						lx->error = _("Integer constant too big");
					}
					return QVariant::Invalid;
				}
			}
			return createConstValue(val, ct);
		} else {
			for (std::list<LexemPtr>::const_iterator it = constant.begin(); it != constant.end(); it++) {
				LexemPtr lx = *it;
				lx->error = _("Constant type mismatch");
			}
			return QVariant::Invalid; // FIXME: type mismatch
		}
	} else {
		maxDim = 0;
		std::list<LexemPtr> alist = constant;
		alist.pop_front();
		alist.pop_back();
		std::list< std::list<LexemPtr> > values;
		typedef std::list< std::list<LexemPtr> >::const_iterator values_iterator;
		std::list<LexemPtr> operators;
		splitLexemsByOperator(alist, LxOperComa, values, operators);
		QVariantList result;
		QList<int> dimensions;
		for (values_iterator it = values.begin(); it != values.end(); ++it) {
			int valDim = 0;
			QVariant value = QVariant::Invalid;
			if ((*it).size() > 0) {
				value = parseConstant(*it, pt, valDim);
				if (value == QVariant::Invalid) {
					return QVariant::Invalid;
				} else {
					result << value;
					dimensions << valDim;
				}
			} else {
				result << QVariant::Invalid;
				dimensions << -1;
			}
		}
		if (dimensions.size() > 0) {
			int firstDim = -1;
			std::list<LexemPtr> firstDimLexems;
			Q_ASSERT((size_t) dimensions.size() == values.size());
			int i = 0;
			values_iterator itt = values.begin();
			for (i = 0; i < dimensions.size(); i++, ++itt) {
				if (dimensions[i] != -1) {
					firstDim = dimensions[i];
					firstDimLexems = (*itt);
					break;
				}
			}
			i = 0;
			for (values_iterator it = values.begin(); it != values.end(); ++it, i++) {
				if (dimensions[i] == -1) {
					continue;
				}
				if (dimensions[i] != firstDim) {
					for (std::list<LexemPtr>::const_iterator lx = it->begin(); lx != it->end(); lx++) {
						(*lx)->error = _("Table constant element of variant type");
					}
					for (std::list<LexemPtr>::const_iterator lx = itt->begin(); lx != itt->end(); lx++) {
						(*lx)->error = _("Table constant element of variant type");
					}
					return QVariant::Invalid;
				}
			}
			if (firstDim == -1) {
				maxDim = 1;
			} else {
				maxDim = firstDim + 1;
			}
		} else {
			maxDim = 1;
		}
		return result;
	}
	return QVariant::Invalid;
}

AST::VariableBaseType SyntaxAnalizer::testConst(const std::list<LexemPtr> &lxs, int &err) const
{
	err = 0;
	LexemPtr lx;
	if (lxs.size() == 1) {
		lx = *lxs.begin();
		if (lx->type == LxConstInteger) {
			return AST::TypeInteger;
		}
		if (lx->type == LxConstReal) {
			return AST::TypeReal;
		}
		if (lx->type == LxConstBoolTrue || lx->type == LxConstBoolFalse) {
			return AST::TypeBoolean;
		}
		if (lx->type == LxConstLiteral && lx->data.size() == 1) {
			return AST::TypeCharect;
		}
		if (lx->type == LxConstLiteral) {
			return AST::TypeString;
		}
	} else if (lxs.size() == 2) {
		std::list<LexemPtr>::const_iterator it = lxs.begin();
		LexemPtr llx = *it;
		it++;
		lx = *it;
		static const QString PlusMinus = QString::fromLatin1("-+");
		if (llx->data.length() == 1 && PlusMinus.contains(llx->data[0])) {
			if (lx->type == LxConstInteger) {
				return AST::TypeInteger;
			} else if (lx->type == LxConstReal) {
				return AST::TypeReal;
			}
		}
	}

	// TODO implement overflow range checking
	return AST::TypeNone;
}

QVariant SyntaxAnalizer::createConstValue(const QString &str
	, const AST::VariableBaseType type) const
{
	QVariant result = QVariant::Invalid;
	if (type == AST::TypeBoolean) {
		result = QVariant(lexer_->boolConstantValue(str));
	} else if (type == AST::TypeCharect) {
		result = QVariant(QChar(str[0]));
	} else if (type == AST::TypeInteger) {
		bool ok;
		if (str.startsWith("$")) {
			result = QVariant(str.mid(1).toInt(&ok, 16));
		} else if (str.startsWith("-$")) {
			result = QVariant(str.mid(2).toInt(&ok, 16));
			result = QVariant(-1 * result.toULongLong());
		} else {
			result = QVariant(str.toInt());
		}
	} else if (type == AST::TypeReal) {
		result = QVariant(str.toDouble());
	} else if (type == AST::TypeString) {
		result = QVariant(str);
	}
	return result;
}

static bool moduleUsedByMain(const AST::DataPtr ast, const AST::ModulePtr module)
{
	AST::ModulePtr mainModule = ast->findModuleByType(AST::ModTypeUserMain);
	return mainModule && module->isEnabledFor(mainModule);
}

bool SyntaxAnalizer::findAlgorithm(
	const QString &name,
	const AST::ModulePtr currentModule,
	const AST::AlgorithmPtr currentAlgorithm,
	AST::ModulePtr &module,
	AST::AlgorithmPtr &algorithm,
	QVariantList &templateParameters
) const
{
	algorithm.clear();
	module.clear();
	for (int i = 0; i < ast_->modules.size(); i++) {
		module = ast_->modules[i];
		bool moduleAvailable =
			module->builtInID == 0xF0 ||
			module->isEnabledFor(currentModule) ||
			alwaysEnabledModules_->contains(module->header.name) ||

			// #kumir2-1821 -- bug here!
			(currentAlgorithm &&
				currentAlgorithm->header.name.startsWith("@") &&
				moduleUsedByMain(ast_, module));
		;
		bool sameFileModule =
			module->header.type == AST::ModTypeUser ||
			module->header.type == AST::ModTypeUserMain ||
			module->header.type == AST::ModTypeTeacherMain ||
			module->header.type == AST::ModTypeTeacher;
		if (moduleAvailable) {
			if (currentModule == module ||
				(sameFileModule && currentModule->header.type == AST::ModTypeTeacher) ||
				(sameFileModule && currentModule->header.type == AST::ModTypeTeacherMain)

			) {
				// The same module - find includes private members
				if (findAlgorithmInModule(name, module, true, true, algorithm, templateParameters)) {
					return true;
				}
			} else {
				// Find only public algorhitm
				if (findAlgorithmInModule(name, module, false, false, algorithm, templateParameters)) {
					return true;
				}
			}
		}
	}
	module.clear();
	return false;
}

bool SyntaxAnalizer::findAlgorithmInModule(const QString &name,
	const ModulePtr &module,
	const bool allowPrivate,
	const bool allowBroken,
	AlgorithmPtr &algorithm,
	QVariantList &templateParameters) const
{
	algorithm = AlgorithmPtr();
	templateParameters = QVariantList();
	const QList<AlgorithmPtr> &algList = allowPrivate
		? module->impl.algorhitms : module->header.algorhitms;
	Q_FOREACH (AlgorithmPtr a, algList) {
		bool skip = !allowBroken && a->header.broken;
		if (skip) {
			continue;
		}
		if (module->impl.actor && a->header.name.contains("%")) {
			const QVariantList actorTemplateParameters =
				module->impl.actor->templateParameters();
			QVariantList usedTemplateParameters;
			QString pattern = a->header.name;
			for (int t = 0; t < actorTemplateParameters.size(); ++t) {
				if (-1 != pattern.indexOf("%" + QString::number(t + 1))) {
					pattern.replace("%" + QString::number(t + 1),
						actorTemplateParameters[t].toString());
					usedTemplateParameters.push_back(actorTemplateParameters[t]);
				}
			}
			pattern = pattern.simplified();
			if (name == pattern) {
				templateParameters = usedTemplateParameters;
				algorithm = a;
				return true;
			}
		}
//        if (module->header.nameTemplate.length() > 0) {
//            // construct fully-qualified name and search by regexp
//            QString pattern = a->header.name;
//            QVariantList params;
//            for (int t=0; t<module->header.templateParameters.size(); t++) {
//                if (-1 != pattern.indexOf("%" + QString::number(t+1))) {
//                    pattern.replace("%" + QString::number(t+1), module->header.templateParameters[t].toString());
//                    params.append(module->header.templateParameters[t]);
//                }
//            }
//            pattern = pattern.simplified();
//            if (name == pattern) {
//                templateParameters = params;
//                algorithm = a;
//                return true;
//            }
//        }
		else {
			// check plaint name match
			if (name == a->header.name) {
				algorithm = a;
				return true;
			}
		}
	}
	return false;
}

bool SyntaxAnalizer::tryInputOperatorAlgorithm(
	const QString &lexem,
	AST::Type &type,
	QVariant &constantValue
	, const AST::ModulePtr currentModule
) const
{

	AST::AlgorhitmExternalReference ref;
	for (int i = 0; i < ast_->modules.size(); i++) {
		const AST::ModulePtr  mod = ast_->modules[i];
		const bool moduleAvailable =
			analizer_->isModuleAlwaysEnabled(mod)
			||
			mod->isEnabledFor(currentModule);
		if (!moduleAvailable) {
			continue;
		}
		for (int j = 0; j < mod->header.operators.size(); j++) {
			const AST::AlgorithmPtr  alg = mod->header.operators[j];
			if (alg->header.arguments.size() == 2
				&& alg->header.arguments[0]->baseType.kind == AST::TypeString
				&& alg->header.arguments[0]->dimension == 0
				&& alg->header.arguments[1]->baseType.kind == AST::TypeBoolean
				&& alg->header.arguments[1]->dimension == 0
				&& alg->header.arguments[1]->accessType == AST::AccessArgumentOut
				&& alg->header.returnType.kind != AST::TypeNone
				&& alg->header.name == lexer_->inputLexemName()
			) {
				ref = alg->header.external;
				Shared::ActorInterface *actor = 0;
				QList<ExtensionSystem::KPlugin *> actors = ExtensionSystem::PluginManager::instance()->loadedPlugins("Actor*");
				for (int k = 0; k < actors.size(); k++) {
					actor = qobject_cast<Shared::ActorInterface *>(actors[k]);
					if (!actor) {
						continue;
					}
					if (actor->localizedModuleName(QLocale::Russian) == mod->header.name) {
						break;
					} else {
						actor = 0;
					}
				}
				if (!actor) {
					continue;
				}
				const Shared::ActorInterface::TypeList tList = actor->typeList();
				for (int k = 0; k < tList.size(); k++) {
					const Shared::ActorInterface::RecordSpecification &clazz = tList[k];
					constantValue = actor->customValueFromString(clazz.asciiName, lexem);
					if (constantValue.isValid()) {
						type.kind = AST::TypeUser;
						type.actor = AST::ActorPtr(actor);
						type.name = clazz.localizedNames.contains(QLocale::Russian)
							? clazz.localizedNames[QLocale::Russian]
							: QString::fromLatin1(clazz.asciiName);
						type.asciiName = clazz.asciiName;
						for (int f = 0; f < clazz.record.size(); f++) {
							const Shared::ActorInterface::Field &field =
								clazz.record[f];
							const QString &fieldName = field.first;
							const ActorInterface::FieldType &fieldType =
								field.second;
							AST::VariableBaseType bt = AST::TypeNone;
							if (fieldType == ActorInterface::Int) {
								bt = AST::TypeInteger;
							} else if (fieldType == ActorInterface::Real) {
								bt = AST::TypeReal;
							} else if (fieldType == ActorInterface::Bool) {
								bt = AST::TypeBoolean;
							} else if (fieldType == ActorInterface::Char) {
								bt = AST::TypeCharect;
							} else if (fieldType == ActorInterface::String) {
								bt = AST::TypeString;
							}
							QPair<QString, AST::Type> astField;
							astField.first = fieldName;
							astField.second.kind = bt;
							type.userTypeFields.push_back(astField);
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool SyntaxAnalizer::findConversionAlgorithm(const AST::Type &from
	, const AST::Type &to
	, AST::ModulePtr   &mod
	, AST::AlgorithmPtr   &alg
	, const AST::ModulePtr currentModule
) const
{
	for (int i = 0; i < ast_->modules.size(); i++) {
		mod = ast_->modules[i];
		const bool moduleAvailable =
			mod->isEnabledFor(currentModule)
			||
			analizer_->isModuleAlwaysEnabled(mod);
		if (!moduleAvailable) {
			continue;
		}
		for (int j = 0; j < mod->header.operators.size(); j++) {
			alg = mod->header.operators[j];
			if (alg->header.arguments.size() == 1) {
				if (alg->header.arguments[0]->baseType == from
					&& alg->header.arguments[0]->dimension == 0) {
					if (alg->header.returnType == to) {
						return true;
					}
				}
			}
		}
	}
	mod.clear();
	alg.clear();
	return false;
}

template <typename TOut>
TOut SyntaxAnalizer::makeCustomUnaryOperation(
	const QString &operatorName
	, AST::ExpressionPtr argument
	, const AST::ModulePtr currentModule
) const
{
	QString argTypeName;
	bool checkOnly = sizeof(TOut) == 1; // return type is a boolean
	if (argument->baseType.kind == AST::TypeUser) {
		argTypeName = argument->baseType.name;
	} else {
		argTypeName = lexer_->classNameByBaseType(argument->baseType.kind);
	}
	for (int i = 0; i < ast_->modules.size(); i++) {
		AST::ModulePtr  mod = ast_->modules[i];
		if (!mod->isEnabledFor(currentModule)) {
			continue;
		}
		for (int j = 0; j < mod->header.operators.size(); j++) {
			AST::AlgorithmPtr  alg = mod->header.operators[j];
			bool omatch = alg->header.name == operatorName;
			if (omatch && alg->header.arguments.size() >= 1) {
				AST::Type formaltype = alg->header.arguments[0]->baseType;
				AST::Type facttype   = argument->baseType;
				AST::AlgorithmPtr  convAlg;
				AST::ModulePtr  convMod;
				bool typematch = formaltype.name == facttype.name
					|| findConversionAlgorithm(facttype, formaltype, convMod, convAlg, currentModule);
				if (typematch) {
					if (checkOnly) {
						return TOut(argument);
					} else {
						AST::ExpressionPtr  res = AST::ExpressionPtr(new AST::Expression);
						res->kind = AST::ExprFunctionCall;
						res->baseType = alg->header.returnType;
						res->function = alg;
						if (convAlg) {
							AST::ExpressionPtr  convExpr = AST::ExpressionPtr(new AST::Expression);
							convExpr->kind = AST::ExprFunctionCall;
							convExpr->function = convAlg;
							convExpr->operands << argument;
							res->operands << convExpr;
						} else {
							res->operands << argument;
						}
						return TOut(res);
					}
				}
			}
		}
	}
	return TOut(AST::ExpressionPtr());
}

AST::ExpressionPtr  SyntaxAnalizer::makeCustomBinaryOperation(const QString &operatorName
	, AST::ExpressionPtr  leftExpression
	, AST::ExpressionPtr  rightExpression
	, const AST::ModulePtr currentModule
) const
{
	QString headTypeName;
	if (leftExpression->baseType.kind == AST::TypeUser) {
		headTypeName = leftExpression->baseType.name;
	} else {
		headTypeName = lexer_->classNameByBaseType(leftExpression->baseType.kind);
	}
	QString tailTypeName;
	if (rightExpression->baseType.kind == AST::TypeUser) {
		tailTypeName = rightExpression->baseType.name;
	} else {
		tailTypeName = lexer_->classNameByBaseType(rightExpression->baseType.kind);
	}
	for (int i = 0; i < ast_->modules.size(); i++) {
		AST::ModulePtr  mod = ast_->modules[i];
		const bool moduleAvailabale =
			mod->isEnabledFor(currentModule)
			||
			analizer_->isModuleAlwaysEnabled(mod);
		if (!moduleAvailabale) {
			continue;
		}
		for (int j = 0; j < mod->header.operators.size(); j++) {
			AST::AlgorithmPtr  alg = mod->header.operators[j];
			bool omatch = alg->header.name == operatorName;
			if (omatch && alg->header.arguments.size() == 2) {
				AST::Type ltype = alg->header.arguments[0]->baseType;
				AST::Type rtype = alg->header.arguments[1]->baseType;
				AST::AlgorithmPtr  lconvAlg;
				AST::AlgorithmPtr  rconvAlg;
				AST::ModulePtr  lconvMod ;
				AST::ModulePtr  rconvMod ;
				bool lmatch = ltype.name == headTypeName
					|| findConversionAlgorithm(leftExpression->baseType, ltype, lconvMod, lconvAlg, currentModule);
				bool rmatch = rtype.name == tailTypeName
					|| findConversionAlgorithm(rightExpression->baseType, rtype, rconvMod, rconvAlg, currentModule);
				if (lmatch && rmatch && omatch) {

					AST::ExpressionPtr  res = AST::ExpressionPtr(new AST::Expression);
					res->kind = AST::ExprFunctionCall;
					res->baseType = alg->header.returnType;
					res->function = alg;
					if (lconvAlg) {
						AST::ExpressionPtr  lconv = AST::ExpressionPtr(new AST::Expression);
						lconv->kind = AST::ExprFunctionCall;
						lconv->function = lconvAlg;
						lconv->operands.append(leftExpression);
						res->operands.append(lconv);
					} else {
						res->operands.append(leftExpression);
					}

					if (rconvAlg) {
						AST::ExpressionPtr  rconv = AST::ExpressionPtr(new AST::Expression);
						rconv->kind = AST::ExprFunctionCall;
						rconv->function = rconvAlg;
						rconv->operands.append(rightExpression);
						res->operands.append(rconv);
					} else {
						res->operands.append(rightExpression);
					}

					return res;
				}
			}
		}
	}
	return AST::ExpressionPtr();
}

bool SyntaxAnalizer::findGlobalVariable(const QString &name, const AST::ModulePtr module, AST::VariablePtr &var) const
{
	var.clear();
	for (int i = 0; i < module->impl.globals.size(); i++) {
		AST::VariablePtr  v = module->impl.globals[i];
		if (v->name == name) {
			var = v;
			break;
		}
	}
	if (!var && (module->header.type == AST::ModTypeTeacher || module->header.type == AST::ModTypeTeacherMain)) {
		AST::ModulePtr userMainModule;
		for (int i = 0; i < ast_->modules.size(); i++) {
			AST::ModulePtr mod = ast_->modules[i];
			if (mod->header.type == AST::ModTypeUserMain) {
				userMainModule = mod;
				break;
			}
		}
		if (userMainModule) {
			for (int i = 0; i < userMainModule->impl.globals.size(); i++) {
				AST::VariablePtr  v = userMainModule->impl.globals[i];
				if (v->name == name) {
					var = v;
					break;
				}
			}
		}
	}
	return !var.isNull();
}

bool SyntaxAnalizer::findLocalVariable(const QString &name, const AST::AlgorithmPtr alg, AST::VariablePtr &var) const
{
	var.clear();
	for (int i = 0; i < alg->impl.locals.size(); i++) {
		AST::VariablePtr  v = alg->impl.locals[i];
		if (v->name == name) {
			var = v;
			break;
		}
	}
	return !var.isNull();
}

bool SyntaxAnalizer::findUserType(const QString &name, AST::Type &type, AST::ModulePtr module, const AST::ModulePtr currentModule) const
{
	module.clear();
	for (int i = 0; i < ast_->modules.size(); i++) {
		AST::ModulePtr  mod = ast_->modules[i];
		if (mod->isEnabledFor(currentModule) ||
			(mod->header.type == AST::ModTypeExternal &&
				alwaysEnabledModules_->contains(mod->header.name))
		) {
			for (int j = 0; j < mod->header.types.size(); j++) {
				AST::Type tp = mod->header.types[j];
				if (tp.name == name) {
					module = mod;
					type = tp;
					return true;
				}
			}
		}
	}
	return false;
}

bool SyntaxAnalizer::findVariable(const QString &name
	, const AST::ModulePtr module
	, const AST::AlgorithmPtr algorhitm
	, AST::VariablePtr &var) const
{
	bool found = false;
	if (algorhitm) {
		found = findLocalVariable(name, algorhitm, var);
	}
	if (!found) {
		found = findGlobalVariable(name, module, var);
	}
	return found;
}

enum BlockType {
	None,
	Simple,
	Element,
	Function,
	SubExpression
} ;


inline bool hasBoolOpBefore(const QList<SubexpressionElement> &alist, int no)
{
	for (int i = no - 1; i >= 0; i--) {
		if (alist[i].o) {
			const LexemType t = alist[i].o->type;
			if (
				t == LxOperBoolEqual ||
				t == LxOperBoolNotEqual ||
				t == LxSecAnd ||
				t == LxSecOr ||
				t == LxSecNot
			) {
				return true;
			}
		}
	}
	return false;
}

AST::ExpressionPtr  SyntaxAnalizer::parseExpression(
	QList<LexemPtr> lexems
	, const AST::ModulePtr mod
	, const AST::AlgorithmPtr alg
) const
{
	if (lexems.isEmpty()) {
		return AST::ExpressionPtr();
	}
	AST::ExpressionPtr  result;
	BlockType blockType = None;

	LexemPtr prevOper;
	LexemPtr oper;
	QList<SubexpressionElement> subexpression;
	QList<LexemPtr> block;

	int curPos = -1;

	// First -- check brackets

	QStack<LexemPtr> openBrackets;
	QStack<LexemPtr> openSqBrackets;
	QStack<LexemPtr> openBraces;


	for (int i = 0; i < lexems.size(); i++) {
		LexemPtr lx = lexems.at(i);
		if (lx->type == LxOperLeftBr) {
			openBrackets.push_back(lx);
		} else if (lx->type == LxOperLeftSqBr) {
			openSqBrackets.push_back(lx);
		} else if (lx->type == LxOperLeftBrace) {
			openBraces.push_back(lx);
		} else if (lx->type == LxOperRightBr) {
			if (openBrackets.size() == 0) {
				lx->error = _("Unpaired )");
				return AST::ExpressionPtr();
			} else {
				openBrackets.pop_back();
			}
		} else if (lx->type == LxOperRightSqBr) {
			if (openSqBrackets.size() == 0) {
				lx->error = _("Unpaired ]");
				return AST::ExpressionPtr();
			} else {
				openSqBrackets.pop_back();
			}
		} else if (lx->type == LxOperRightBrace) {
			if (openBraces.size() == 0) {
				lx->error = _("Unpaired }");
				return AST::ExpressionPtr();
			} else {
				openBraces.pop_back();
			}
		}
	}

	foreach (LexemPtr lx, openBrackets) {
		lx->error = _("Unpaired (");
	}
	foreach (LexemPtr lx, openSqBrackets) {
		lx->error = _("Unpaired [");
	}
	foreach (LexemPtr lx, openBraces) {
		lx->error = _("Unpaired {");
	}

	if (openBrackets.size() + openSqBrackets.size() + openBraces.size() > 0) {
		return AST::ExpressionPtr();
	}

	while (curPos < lexems.size()) {
		blockType = None;
		block.clear();
		prevOper = oper;
		oper.clear();
		curPos ++;
		for (; curPos < lexems.size(); curPos++) {
			if (IS_OPERATOR(lexems[curPos]->type)) {
				oper = lexems[curPos];
				break;
			}
			block << lexems[curPos];
		}

		if (oper && oper->type == LxOperLeftBr) {
			if (block.size() > 0 && block[0]->type != LxSecNot) {
				blockType = Function;
			} else {
				blockType = SubExpression;
			}
		} else if (oper && oper->type == LxOperLeftSqBr) {
			if (block.isEmpty()) {
				lexems[curPos]->error = _("Array name not specified before '['");
				return AST::ExpressionPtr();
			}
			blockType = Element;
		} else {
			if (block.size() > 0) {
				blockType = Simple;
			} else if (prevOper && prevOper->type == LxOperComa && (!oper || (oper && oper->type != LxOperPlus && oper->type != LxOperMinus))) {
				prevOper->error = _("Extra ','");
				return AST::ExpressionPtr();
			}
		}

		if (oper && oper->type == LxOperComa) {
			oper->error = _("Extra coma");
			return AST::ExpressionPtr();
		}

		if (oper && oper->type == LxOperColon) {
			oper->error = _("Extra colon");
			return AST::ExpressionPtr();
		}

		if (oper && oper->type == LxOperRightBr) {
			oper->error = _("Extra )");
			return AST::ExpressionPtr();
		}

		if (oper && oper->type == LxOperLeftBrace) {
			oper->error = _("Extra {");
			return AST::ExpressionPtr();
		}

		if (oper && oper->type == LxOperRightBrace) {
			oper->error = _("Extra }");
			return AST::ExpressionPtr();
		}

		if (oper && oper->type == LxOperRightSqBr) {
			oper->error = _("Extra ]");
			return AST::ExpressionPtr();
		}

		LexemPtr notFlag;
		bool notInTheMiddle = false;
		if (block.size() > 1 && block.last()->type == LxSecNot) {
			block.last()->error = _("'not' at the end");
			return AST::ExpressionPtr();
		}
		for (QList<LexemPtr>::iterator it = block.begin(); it != block.end();) {
			if ((*it)->type == LxSecNot && notFlag == 0) {
				notFlag = (*it);
				it = block.erase(it);
				notInTheMiddle = it != block.begin();
			} else if ((*it)->type == LxSecNot && notFlag != 0) {
				(*it)->error = _("Too many 'not'");
				return AST::ExpressionPtr();
			} else {
				it++;
			}
		}



		if (blockType == Function) {
			int deep = 0;
			int openBrPos = curPos;
			for (; curPos < lexems.size(); curPos++) {
				block << lexems[curPos];
				if (lexems[curPos]->type == LxOperLeftBr) {
					deep++;
				}
				if (lexems[curPos]->type == LxOperRightBr) {
					deep--;
				}
				if (deep == 0) {
					break;
				}
			}
			if (deep > 0) {
				lexems[openBrPos]->error = _("No pairing ')'"); // FIXME error code for unmatched open bracket
				return AST::ExpressionPtr();
			}
			AST::ExpressionPtr  operand = parseFunctionCall(block, mod, alg);
			if (!operand) {
				return AST::ExpressionPtr();
			}
			if (notFlag) {
//                if (operand->baseType!=AST::TypeBoolean) {
//                    delete operand;
//                    notFlag->error = _("Use of 'not' for non-boolean value");
//                    return 0;
//                }
				if (operand->baseType.kind != AST::TypeBoolean && notInTheMiddle) {
					notFlag->error = _("'not' must be before boolean expression");

					return AST::ExpressionPtr();
				}
				const LexemType lxType = block.first()->type;
				foreach (LexemPtr lx, block) {
					lx->type = lxType;
					// set correct lexem type to all parts split by 'not'
				}
				subexpression << SubexpressionElement(notFlag);
			}
			curPos++;
			if (curPos >= lexems.size()) {
				oper.clear();
			} else if (IS_OPERATOR(lexems[curPos]->type)) {
				oper = lexems[curPos];
			} else {
				lexems[curPos]->error = _("Forgotten operator after bracket"); // FIXME garbage error

				return AST::ExpressionPtr();
			}
			subexpression << SubexpressionElement(operand);
			if (oper && oper->type == LxOperLeftBr) {

				oper->error = _("No operator before (");
				return AST::ExpressionPtr();
			}
			if (oper) {
				subexpression << SubexpressionElement(oper);
			}
			if (subexpression.size() > 1 &&
				operand->baseType.kind == AST::TypeNone &&
				operand->kind == AST::ExprFunctionCall
			) {
				for (int i = 0; i < block.size(); i++) {
					block[i]->error = _("Assignment of non-returning algorithm");
				}

				return AST::ExpressionPtr();
			}
		} // end if (blockType==Function)
		else if (blockType == Simple) {
			if (block.isEmpty() && notFlag) {
				bool isUnaryOperatorAfterNot =
					curPos < lexems.length() && (
						lexems[curPos]->type == LxOperPlus ||
						lexems[curPos]->type == LxOperMinus);
				if (curPos == lexems.length() - 1 || !isUnaryOperatorAfterNot) {
					notFlag->error = _("Extra 'not'");
					return AST::ExpressionPtr();
				} else {
					subexpression << SubexpressionElement(notFlag);
					subexpression << SubexpressionElement(lexems[curPos]);
					continue;
				}
			}

			bool hasUnaryMinusBefore =
				subexpression.size() == 1 &&
				subexpression[subexpression.size() - 1].o &&
				subexpression[subexpression.size() - 1].o->type == LxOperMinus
				;

			if (block.size() == 1 &&
				(block.at(0)->type == LxConstInteger ||
					block.at(0)->type == LxConstReal)) {
				bool nextOperHasHigherPriority = false;
				bool hex = block.at(0)->data.trimmed().startsWith("$");
				if (oper) {
					nextOperHasHigherPriority =
						oper->type == LxOperAsterisk ||
						oper->type == LxOperSlash ||
						oper->type == LxOperPower;
				}
				if (hasUnaryMinusBefore && !nextOperHasHigherPriority && !hex) {
					subexpression.pop_back();
					block.prepend(prevOper);
					prevOper->type = block.at(1)->type;
				}
			}


			AST::ExpressionPtr  operand = parseSimpleName(block.toStdList(),
					mod,
					alg);
			if (!operand) {
				return AST::ExpressionPtr();
			}
			if (notFlag) {
//                if (operand->baseType!=AST::TypeBoolean) {
//                    delete operand;
//                    notFlag->error =  _("Use of 'not' for non-boolean value");
//                    return 0;
//                }
				if (operand->baseType.kind != AST::TypeBoolean && notInTheMiddle) {
					notFlag->error = _("'not' must be before boolean expression");

					return AST::ExpressionPtr();
				}
				const LexemType lxType = block.first()->type;
				foreach (LexemPtr lx, block) {
					lx->type = lxType;
					// set correct lexem type to all parts split by 'not'
				}
				subexpression << SubexpressionElement(notFlag);
			}
			subexpression << SubexpressionElement(operand);
			if (oper) {
				subexpression << SubexpressionElement(oper);
			}
			if (subexpression.size() > 1 &&
				operand->baseType.kind == AST::TypeNone &&
				operand->kind == AST::ExprFunctionCall
			) {
				for (int i = 0; i < block.size(); i++) {
					block[i]->error = _("Assignment of non-returning algorithm");
				}

				return AST::ExpressionPtr();
			}
		} // end if (blockType==Variable)
		else if (blockType == Element) {
			int deep = 0;
			int openBrPos = curPos;
			int closeBrPos = -1;
			for (; curPos < lexems.size(); curPos++) {
				LexemPtr clx = lexems[curPos];
				if (deep == 0 && IS_OPERATOR(clx->type) && clx->type != LxOperLeftSqBr && clx->type != LxOperRightSqBr) {
					curPos--;
					break;
				}
				block << clx;
				if (clx->type == LxOperLeftSqBr) {
					if (deep == 0) {
						openBrPos = curPos;
					}
					deep++;
				}
				if (clx->type == LxOperRightSqBr) {
					deep--;
					closeBrPos = curPos;
				}
			}
			if (deep > 0) {
				lexems[openBrPos]->error = _("No pairing ']'");
				return AST::ExpressionPtr();
			} else if (deep < 0 && closeBrPos != -1) {
				lexems[closeBrPos]->error = _("No pairing '['");
				return AST::ExpressionPtr();
			}
			AST::ExpressionPtr  operand = parseElementAccess(block, mod, alg);
			if (!operand) {
				return AST::ExpressionPtr();
			}
			if (notFlag) {
//                if (operand->baseType!=AST::TypeBoolean) {
//                    delete operand;
//                    notFlag->error =  _("Use of 'not' for non-boolean value");
//                    return 0;
//                }
				const LexemType lxType = block.first()->type;
				foreach (LexemPtr lx, block) {
					lx->type = lxType;
					// set correct lexem type to all parts split by 'not'
				}
				subexpression << SubexpressionElement(notFlag);
			}
			curPos++;
			if (curPos >= lexems.size()) {
				oper.clear();
			} else if (IS_OPERATOR(lexems[curPos]->type)) {
				oper = lexems[curPos];
			} else {
				lexems[curPos]->error = _("Forgotten operator after bracket"); // FIXME garbage error

				return AST::ExpressionPtr();
			}
			subexpression << SubexpressionElement(operand);
			if (oper) {
				subexpression << SubexpressionElement(oper);
			}
		} // end if (blockType==Element)
		else if (blockType == SubExpression) {
			int deep = 0;
			int openBrPos = curPos;
			for (; curPos < lexems.size(); curPos++) {
				block << lexems[curPos];
				if (lexems[curPos]->type == LxOperLeftBr) {
					deep++;
				}
				if (lexems[curPos]->type == LxOperRightBr) {
					deep--;
				}
				if (deep == 0) {
//                    curPos++;
					break;
				}
			}
			if (curPos >= lexems.size()) {
				oper.clear();
			}
//            else if (IS_OPERATOR(lexems[curPos]->type)) {
//                oper = lexems[curPos];
//            }
			if (deep > 0) {
				lexems[openBrPos]->error = _("No pairing ')'"); // FIXME error code for unmatched open bracket
				return AST::ExpressionPtr();
			}
			if (block.size() < 3) {
				if (block.size() > 0) {
					block[0]->error = _("Nothing between '(' and ')'"); // FIXME error code: nothing between brackets
				}
				if (block.size() > 1) {
					block[1]->error = block[0]->error;
				}
				return AST::ExpressionPtr();
			}
			if (block.first()->type == LxOperLeftBr && block.last()->type == LxOperRightBr) {
				block.pop_back(); // remove ")"
				block.pop_front(); // remove "("
			}
			AST::ExpressionPtr  operand = parseExpression(block, mod, alg);
			if (!operand) {
				return AST::ExpressionPtr();
			}
			if (notFlag) {
//                if (operand->baseType!=AST::TypeBoolean) {
//                    delete operand;
//                    notFlag->error =  _("Use of 'not' for non-boolean value");
//                    return 0;
//                }
				if (operand->baseType.kind != AST::TypeBoolean && notInTheMiddle) {
					notFlag->error = _("'not' must be before boolean expression");

					return AST::ExpressionPtr();
				}
				subexpression << SubexpressionElement(notFlag);
			}
			curPos++;
			if (curPos == lexems.size()) {
				oper.clear();
			} else if (IS_OPERATOR(lexems[curPos]->type)) {
				oper = lexems[curPos];
				if (oper->type == LxOperLeftSqBr) {
					oper->error = _("No name before [");

					return AST::ExpressionPtr();
				}
				if (oper->type == LxOperLeftBr) {
					oper->error = _("No operator before (");

					return AST::ExpressionPtr();
				}
			} else {
				lexems[curPos]->error = _("Forgotten operator after bracket"); // FIXME garbage error

				return AST::ExpressionPtr();
			}
			if (oper && oper->type == LxOperComa) {
				oper->error = _("Extra ','");

				return AST::ExpressionPtr();
			}
			subexpression << SubexpressionElement(operand);
			if (oper) {
				subexpression << SubexpressionElement(oper);
			}
			operand->expressionIsClosed = true;
		} // end if (blockType==SubExpression)
		else if (blockType == None) {
			if (oper) {
				if (oper->type == LxOperPlus || oper->type == LxOperMinus) {
					subexpression << SubexpressionElement(oper);
				} else {
					oper->error = _("Extra operator");
					return AST::ExpressionPtr();
					//                qDebug() << "Check me: " << __FILE__ << ", line: " << __LINE__;
				}
			} else {
				if (lexems.last()->type == LxOperRightBr) {
					lexems.last()->error = _("No pairing '('");
					return AST::ExpressionPtr();
				}
				if (lexems.last()->type == LxOperRightSqBr) {
					lexems.last()->error = _("No pairing '['");
					return AST::ExpressionPtr();
				}

			}
		}

	}
	for (int i = 0; i < subexpression.size(); i++) {
		const SubexpressionElement &el = subexpression.at(i);
		if (el.o && (el.o->type == LxOperEqual || el.o->type == LxOperNotEqual)) {
			bool isBooleanComparision =
				(i > 0 && subexpression[i - 1].e && subexpression[i - 1].e->baseType.kind == AST::TypeBoolean)
//                    ||
//                    i>0 && hasBoolOpBefore(subexpression, i)
				||
				(i < subexpression.size() - 1 && subexpression[i + 1].e && subexpression[i + 1].e->baseType.kind == AST::TypeBoolean)
				||
				(i < subexpression.size() - 1 && subexpression[i + 1].o && subexpression[i + 1].o->type == LxSecNot);
			if (isBooleanComparision) {
				if (el.o->type == LxOperEqual) {
					el.o->type = LxOperBoolEqual;
				} else {
					el.o->type = LxOperBoolNotEqual;
				}
			}
		}
	}
	result = makeExpressionTree(subexpression, mod);
	convertDuplicateOperandsToCacheItems(result);
	optimizeUnaryPlus(result);
	if (result) {
		result->lexems = lexems;
	}
	return result;
}

static void convertDuplicateOperandsToCacheItems_r(AST::ExpressionPtr root, QSet<AST::ExpressionPtr> &cache)
{
	cache.insert(root);
	Q_FOREACH (AST::ExpressionPtr child, root->operands) {

		convertDuplicateOperandsToCacheItems_r(child, cache);
	}
}

void SyntaxAnalizer::convertDuplicateOperandsToCacheItems(AST::ExpressionPtr root) const
{
	if (!root) {
		return;
	}
	using namespace AST;
	QSet<AST::ExpressionPtr> cache;
	convertDuplicateOperandsToCacheItems_r(root, cache);
}

void SyntaxAnalizer::optimizeUnaryPlus(ExpressionPtr &root) const
{
	if (!root) {
		return;
	}
	using namespace AST;
	if (ExprSubexpression == root->kind && OpSumm == root->operatorr && 1 == root->operands.size()) {
		root = root->operands.at(0);
	}
	for (int i = 0; i < root->operands.size(); ++i) {
		optimizeUnaryPlus(root->operands[i]);
	}
}

static QStringList possibleModuleImports(
	const QString &nameToFind,
	const AST::DataPtr ast
	, const AST::ModulePtr currentModule
)
{
	QStringList result;

	for (int i = 0; i < ast->modules.size(); i++) {
		if (!ast->modules[i]->isEnabledFor(currentModule)) {
			// Check for disabled modules only
			for (int j = 0; j < ast->modules[i]->header.algorhitms.size(); j++) {
				AST::AlgorithmPtr  alg = ast->modules[i]->header.algorhitms[j];
				if (alg->header.name == nameToFind) {
					result.push_back(ast->modules[i]->header.name);
				}
			}
		}
	}

	if (result.isEmpty() && nameToFind.contains(' ')) {
		// Try to find a type
		const QString firstWord = nameToFind.left(nameToFind.indexOf(' '));
		for (int i = 0; i < ast->modules.size(); i++) {
			if (!ast->modules[i]->isEnabledFor(currentModule)) {
				for (int j = 0; j < ast->modules[i]->header.types.size(); j++) {
					const AST::Type &type = ast->modules[i]->header.types[j];
					if (type.kind == AST::TypeUser && type.name == firstWord) {
						result.push_back(ast->modules[i]->header.name);
					}
				}
			}
		}
	}

	return result;
}

const TextStatement &SyntaxAnalizer::findSourceStatementByLexem(
	const LexemPtr lexem
) const
{
	for (int i = 0; i < statements_.size(); i++) {
		bool found = false;
		for (int j = 0; j < statements_[i].data.size(); j++) {
			if (lexem == statements_[i].data[j]) {
				found = true;
				break;
			}
		}
		if (found) {
			return statements_[i];
		}
	}
	static const TextStatement dummy;
	return dummy;
}

AST::ExpressionPtr  SyntaxAnalizer::parseFunctionCall(const QList<LexemPtr> &lexems, const AST::ModulePtr mod, const AST::AlgorithmPtr alg) const
{
	AST::ExpressionPtr  result;
	QString name;
	QList<LexemPtr> nameLexems;
	LexemPtr openBracket;
	int openBracketIndex = -1;
	for (int i = 0; i < lexems.size(); i++) {
		if (lexems[i]->type == LxNameClass || lexems[i]->type & LxTypePrimaryKwd || lexems[i]->type & LxTypeSecondaryKwd) {
			lexems[i]->error = _("Keyword in name");
			return AST::ExpressionPtr();
		}
		if (lexems[i]->type == LxOperLeftBr) {
			openBracket = lexems[i];
			openBracketIndex = i;
			break;
		} else {
			if (i > 0) {
				name += " ";
			}
			name += lexems[i]->data;
			nameLexems << lexems[i];
		}
	}
	QList<AST::ExpressionPtr> realArguments;
	QList< QList<LexemPtr> > arguments;
	QList<LexemPtr> comas;

	AST::AlgorithmPtr  function;
	AST::ModulePtr functionModule;
	QVariantList functionTemplateParameters;
	if (!findAlgorithm(name, mod, alg, functionModule, function, functionTemplateParameters)) {
		if (openBracketIndex == -1) {
			openBracketIndex = lexems.size();
		}
		for (int i = 0; i < openBracketIndex; i++) {
			lexems[i]->error = _("Algorhitm not found");
		}
		const QStringList possibleImports = possibleModuleImports(name, ast_, mod);
		if (possibleImports.size() > 0) {
			const TextStatement &sourceStatement =
				findSourceStatementByLexem(lexems.first());
			sourceStatement.suggestedImportModuleNames = possibleImports;
		}
		return AST::ExpressionPtr();
	}

	if (mod->header.type == AST::ModTypeUser || mod->header.type == AST::ModTypeUserMain) {
		if (name.startsWith("@")) {
			for (int i = 0; i < openBracketIndex; i++) {
				lexems[i]->error = _("Available only to teacher");
			}
			return AST::ExpressionPtr();
		}
	}

	if (!function->header.error.isEmpty()) {
		foreach (LexemPtr lx, nameLexems) {
			lx->error = _("This algorhitm is broken");
		}
		return AST::ExpressionPtr();
	}

	foreach (LexemPtr lx, nameLexems) lx->type = LxNameAlg;

	if (!openBracket) {
		// No arguments
		// Do nothing, because of implemented as other
		// block type (see parseSimpleName);
		qWarning() << "!!! Something strange at " << __FILE__ << ", line: " << __LINE__;
		return AST::ExpressionPtr();
	}

	//
	QList<LexemPtr> argLine = lexems.mid(openBracketIndex);
	LexemPtr ob;
	LexemPtr cb;
	int deep = 0;
	int cbPos = -1;
	for (int i = 0; i < argLine.size(); i++) {
		if (argLine[i]->type == LxOperLeftBr) {
			ob = argLine[i];
			deep ++;
		}
		if (argLine[i]->type == LxOperRightBr) {
			cb = argLine[i];
			cbPos = i;
			deep --;
		}
	}
	if (deep > 0) {
		ob->error = _("Unpaired '('");
		return AST::ExpressionPtr();
	}
	if (deep < 0) {
		for (int i = argLine.size() - 1; i >= 0; i--) {
			if (argLine[i]->type == LxOperRightBr) {
				deep--;
			}
			if (deep == 0) {
				argLine[i]->error = _("Unpaired ')'");
				return AST::ExpressionPtr();
			}
		}
	}
	if ((cbPos + 1) < argLine.size()) {
		for (int i = cbPos + 1; i < argLine.size(); i++) {
			argLine[cbPos]->error = _("Garbage in expression");
		}
		return AST::ExpressionPtr();
	}
	argLine.pop_back();
	argLine.pop_front();
	if (argLine.isEmpty()) {
		openBracket->error = _("Nothing between '(' and ')'");
		cb->error = _("Nothing between '(' and ')'");
		return AST::ExpressionPtr();
	}
	splitLexemsByOperator(argLine, LxOperComa, arguments, comas);
	int functionArgumentsCount = function->header.arguments.size();
	functionArgumentsCount -= function->header.external.algorithmAsciiName.count("%");
	int diff = arguments.size() - functionArgumentsCount;
	if (diff > 0) {
		for (int i = 0; i < diff; i++) {
			QList<LexemPtr> lxs = arguments[arguments.size() - 1 - i];
			if (lxs.isEmpty()) {
				LexemPtr lx = comas[comas.size() - 1 - i];
				lx->error = _("Extra coma");
			} else {
				foreach (LexemPtr lx, lxs) {
					lx->error = _("Extra algorithm arguments");
				}
			}
		}
		return AST::ExpressionPtr();
	}
	if (diff < 0) {
		cb->error = _("Not enought algorithm arguments");
		return AST::ExpressionPtr();
	}
	for (int i = 0; i < arguments.size(); i++) {
		AST::Type extType = function->header.arguments[i]->baseType;
		int extDim = function->header.arguments[i]->dimension;
		bool allowTypeChange = function->header.arguments[i]->accessType == AST::AccessArgumentIn;
		AST::ExpressionPtr  argument;
		if (arguments[i].isEmpty()) {
			const QString err = _("Void argument");
			if (i > 0) {
				comas[i - 1]->error = err;
			} else {
				ob->error = err;
			}
			return AST::ExpressionPtr();
		}
		if (extDim == 0) {
			argument = parseExpression(arguments[i], mod, alg);
			if (!argument) {
				return AST::ExpressionPtr();
			}
		} else {
			QString arrName;
			foreach (const LexemPtr lx, arguments[i]) {
				if (!arrName.isEmpty()) {
					arrName += " ";
				}
				arrName += lx->data;
			}
			AST::VariablePtr  var;
			if (findVariable(arrName, mod, alg, var)) {
				argument = AST::ExpressionPtr(new AST::Expression);
				argument->kind = AST::ExprVariable;
				argument->dimension = var->dimension;
				argument->variable = var;
				argument->baseType = var->baseType;
			} else {
				const QString err = _("Array required here");
				foreach (LexemPtr lx, arguments[i]) lx->error = err;
				return AST::ExpressionPtr();
			}
		}
		realArguments << argument;
		AST::Type intType = argument->baseType;
		int intDim = argument->dimension;
		bool typesCompatible = (intType.kind == AST::TypeInteger && extType.kind == AST::TypeReal)
			|| (intType.kind == AST::TypeCharect && extType.kind == AST::TypeString);
		QString err;
		if (intDim != extDim) {
			if (intDim == 0 || extDim == 0) {
				err = _("Incompatible types");
			} else {
				err = _("Array dimension mismatch");
			}
		}
		if (intType != extType) {
			if (!allowTypeChange || !typesCompatible) {
				err = _("Incompatible types");
			}
		}
		if (argument->kind != AST::ExprVariable
			&& function->header.arguments[i]->accessType == AST::AccessArgumentOut) {
			err = _("Can't pass this expression as out-argument");
		}
		if (argument->kind != AST::ExprVariable
			&& function->header.arguments[i]->accessType == AST::AccessArgumentInOut) {
			err = _("Can't pass this expression as in/out-argumeny");
		}
		if (argument->kind == AST::ExprVariable
			&& argument->variable->accessType == AST::AccessArgumentIn) {
			if (function->header.arguments[i]->accessType == AST::AccessArgumentOut) {
				err = _("Can't pass in-argument as out-argument");
			}
			if (function->header.arguments[i]->accessType == AST::AccessArgumentInOut) {
				err = _("Can't pass in-argument as in/out-argument");
			}
		}
		if (!err.isEmpty()) {
			foreach (LexemPtr l, arguments[i]) l->error = err;
			return AST::ExpressionPtr();
		}
	} // end for arguments loop
	result = AST::ExpressionPtr(new AST::Expression);
	result->kind = AST::ExprFunctionCall;
	result->baseType = function->header.returnType;
	result->dimension = 0;
	result->function = function;
	addTemplateParametersToFunctionCall(result, functionTemplateParameters);
	result->operands.append(realArguments);
	result->lexems = lexems;
	return result;
}

void SyntaxAnalizer::updateSliceDSCall(AST::ExpressionPtr  expr, AST::VariablePtr  var) const
{
	static AST::AlgorithmPtr  strlenAlg;
	static AST::ModulePtr strlenMod;
	static AST::ModulePtr  stdlibMod;
	static QVariantList functionTemplateParameters;
	if (!strlenAlg) {
		findAlgorithm(QString::fromUtf8("длин"), stdlibMod, AST::AlgorithmPtr(), strlenMod, strlenAlg, functionTemplateParameters);
	}
	if (expr->kind == AST::ExprFunctionCall
		&& expr->function == strlenAlg
		&& expr->operands.size() == 0) {
		AST::ExpressionPtr  varExpr = AST::ExpressionPtr(new AST::Expression);
		varExpr->kind = AST::ExprVariable;
		varExpr->baseType = AST::TypeString;
		varExpr->dimension = 0;
		varExpr->variable = var;
		expr->operands.append(varExpr);
	} else
		foreach (AST::ExpressionPtr  subExpr, expr->operands) {
			updateSliceDSCall(subExpr, var);
		}
}

bool SyntaxAnalizer::checkWrongDSUsage(ExpressionPtr expression)
{
	static AST::AlgorithmPtr  strlenAlg;
	static AST::ModulePtr strlenMod;
	static AST::ModulePtr  stdlibMod;
	static QVariantList functionTemplateParameters;
	bool hasError = false;
	if (!strlenAlg) {
		findAlgorithm(QString::fromUtf8("длин"), stdlibMod, AST::AlgorithmPtr(), strlenMod, strlenAlg, functionTemplateParameters);
	}
	if (expression->kind == AST::ExprFunctionCall
		&& expression->function == strlenAlg
		&& expression->operands.size() == 0) {
		static const QString errorMessage = _("Wrong 'sl' usage");
		foreach (LexemPtr lx, expression->lexems) {
			lx->error = errorMessage;
		}
		hasError = true;
	} else
		foreach (AST::ExpressionPtr  subExpr, expression->operands) {
			hasError = hasError || checkWrongDSUsage(subExpr);
		}
	return hasError;
}

AST::ExpressionPtr  SyntaxAnalizer::parseElementAccess(const QList<LexemPtr> &lexems, const AST::ModulePtr mod, const AST::AlgorithmPtr alg) const
{
	AST::ExpressionPtr  result;
	QString name;
	LexemPtr openBracket;
	int openBracketIndex = -1;
	bool nameIsNumericConstant = true;
	for (int i = 0; i < lexems.size(); i++) {
		if (lexems[i]->type == LxOperLeftSqBr) {
			openBracket = lexems[i];
			openBracketIndex = i;
			break;
		} else {
			if (i > 0) {
				name += " ";
			}
			name += lexems[i]->data;
			nameIsNumericConstant = nameIsNumericConstant &&
				(lexems[i]->type == LxConstInteger
					|| lexems[i]->type == LxConstReal
					|| lexems[i]->type == LxConstBoolTrue
					|| lexems[i]->type == LxConstBoolFalse
				);
		}
	}
	if (nameIsNumericConstant) {
		if (openBracketIndex == -1) {
			openBracketIndex = lexems.size();
		}
		for (int i = 0; i < openBracketIndex; i++) {
			lexems[i]->error = _("Non-literal constant is not indexable");
		}
		return AST::ExpressionPtr();
	}
	QList<AST::ExpressionPtr> realArguments;
	QList< QList<LexemPtr> > arguments;
	QList<LexemPtr> comas;

	AST::VariablePtr  variable;
	if (!findVariable(name, mod, alg, variable)) {
		if (lexer_->isReturnVariable(name)) {
			if (!alg || alg->header.returnType.kind == AST::TypeNone) {
				if (openBracketIndex == -1) {
					openBracketIndex = lexems.size();
				}
				for (int i = 0; i < openBracketIndex; i++) {
					if (!alg) {
						lexems[i]->error = _("Access to return value outside of algorithm");
					} else {
						lexems[i]->error = _("This algorithm has no return value");
					}
				}
				return AST::ExpressionPtr();
			} else {
				bool found = findLocalVariable(alg->header.name, alg, variable);
				Q_ASSERT(found);
				Q_UNUSED(found);
			}
		} else {
			AST::AlgorithmPtr  a ;
			AST::ModulePtr am;
			QVariantList aTemplateParameters;
			if (findAlgorithm(name, mod, alg, am, a, aTemplateParameters)) {
				int a = qMax(0, openBracketIndex);
				for (int i = a; i < lexems.size(); i++) {
					lexems[i]->error = _("'[...]' instead of '(...)'");
				}
				if (openBracketIndex == -1) {
					openBracketIndex = lexems.size();
				}
				for (int i = 0; i < openBracketIndex; i++) {
					lexems[i]->type = LxNameAlg;
				}
			} else {
				if (openBracketIndex == -1) {
					openBracketIndex = lexems.size();
				}
				for (int i = 0; i < openBracketIndex; i++) {
					lexems[i]->error = _("Variable not found");
				}
			}
			return AST::ExpressionPtr();
		}
	}

	if (lexer_->isReturnVariable(name) && alg->header.returnType.kind != AST::TypeString) {
		if (openBracketIndex == -1) {
			openBracketIndex = lexems.size();
		}
		for (int i = 0; i < openBracketIndex; i++) {
			lexems[i]->error = _("Return value is not indexable");
		}
		return AST::ExpressionPtr();
	}

	if (!openBracket) {
		// No arguments
		// Do nothing, because of implemented as other
		// block type (see parseSimpleName);
		qWarning() << "!!! Something strange at " << __FILE__ << ", line: " << __LINE__;
		return AST::ExpressionPtr();
	}

	//
	QList<LexemPtr> argLine = lexems.mid(openBracketIndex);
	LexemPtr ob;
	LexemPtr cb;
	int deep = 0;
	int cbPos = -1;
	int extraIndecesStart = -1;
	for (int i = 0; i < argLine.size(); i++) {
		if (argLine[i]->type == LxOperLeftSqBr) {
			ob = argLine[i];
			if (deep == 0 && i > 0) {
				extraIndecesStart = i;
			}
			deep ++;
		}
		if (argLine[i]->type == LxOperRightSqBr) {
			cb = argLine[i];
			cbPos = i;
			deep --;
		}
	}
	if (deep > 0) {
		ob->error = _("Unpaired '['");
		return AST::ExpressionPtr();
	}
	if (deep < 0) {
		for (int i = argLine.size() - 1; i >= 0; i--) {
			if (argLine[i]->type == LxOperRightSqBr) {
				deep--;
			}
			if (deep == 0) {
				argLine[i]->error = _("Unpaired ']'");
				return AST::ExpressionPtr();
			}
		}
	}
	if ((cbPos + 1) < argLine.size()) {
		for (int i = cbPos + 1; i < argLine.size(); i++) {
			argLine[i]->error = _("Garbage in expression");
		}
		return AST::ExpressionPtr();
	}
	if (extraIndecesStart != -1) {
		static const QString err = _("Indeces was specified before");
		for (int i = extraIndecesStart; i < argLine.size(); i++) {
			argLine[i]->error = err;
		}
		return AST::ExpressionPtr();
	}
	argLine.pop_back();
	argLine.pop_front();
	if (argLine.isEmpty()) {
		openBracket->error = _("Nothing between '[' and ']'");
		cb->error = openBracket->error;
		return AST::ExpressionPtr();
	}
	splitLexemsByOperator(argLine, LxOperComa, arguments, comas);
	for (int i = 0; i < arguments.size(); i++) {
		if (arguments[i].isEmpty()) {
			const QString err = _("Void index");
			if (i > 0) {
				comas[i - 1]->error = err;
			} else {
				ob->error = err;
			}
			return AST::ExpressionPtr();
		}
	}

	int varDimension = variable->dimension;
	if (variable->baseType.kind == AST::TypeString) {
		varDimension ++;
	}

	for (int i = 0; i < arguments.size(); i++) {

		QList<LexemPtr> colons;
		QList< QList<LexemPtr> > slice;

		splitLexemsByOperator(arguments[i], LxOperColon, slice, colons);

		if (slice.size() > 1 && variable->baseType.kind != AST::TypeString) {
			foreach (LexemPtr lx, arguments[i]) {
				lx->error = _("Not a simple index");
			}
			return AST::ExpressionPtr();
		}
		if (slice.size() > 1 && variable->baseType.kind == AST::TypeString && i < arguments.size() - 1) {
			foreach (LexemPtr lx, arguments[i]) {
				lx->error = _("Slice of string array, but not string");
			}
			return AST::ExpressionPtr();
		}
		if (slice.size() > 2) {
			QString err = _("Extra indeces in slice");
			for (int a = 1; a < colons.size(); a++) {
				colons[a]->error = err;
			}
			for (int b = 2; b < slice.size(); b++) {
				for (int a = 0; a < slice[b].size(); a++) {
					slice[b][a]->error = err;
				}
			}
			return AST::ExpressionPtr();
		}
		for (int j = 0 ; j < slice.size(); j++) {
			if (slice[j].isEmpty()) {
				const QString err = _("Void index");
				if (j > 0) {
					colons[j - 1]->error = err;
				} else {
					openBracket->error = err;
				}

				return AST::ExpressionPtr();
			}
			AST::ExpressionPtr  argument = parseExpression(slice[j], mod, alg);
			if (!argument) {
				return AST::ExpressionPtr();
			}
			realArguments << argument;
			AST::Type intType = argument->baseType;
			int intDim = argument->dimension;
			if (intDim > 0) {
				foreach (LexemPtr lx, slice[j]) {
					lx->error = _("Passing table as index");
				}

				return AST::ExpressionPtr();
			}
			if (intType.kind != AST::TypeInteger) {
				foreach (LexemPtr lx, slice[j]) {
					lx->error = _("Index is not integer");
				}

				return AST::ExpressionPtr();
			}
			updateSliceDSCall(argument, variable);
		}
	} // end for arguments loop

	int diff = arguments.size() - varDimension;
	if (diff > 0 && varDimension == 0) {
		for (int i = openBracketIndex; i < lexems.size(); i++) {
			lexems[i]->error = _("Extra indeces");
		}
		return AST::ExpressionPtr();
	} else if (diff > 0) {
		int errorStartIndex = 0;
		for (int i = comas.size() - 1; i >= 0; i--) {
			deep--;
//            if (deep==0) {
			errorStartIndex = argLine.indexOf(comas[i]) + 1;
//            }
		}
		for (int i = errorStartIndex; i < cbPos - 1; i++) {
			argLine[i]->error = _("Extra indeces");
		}

		return AST::ExpressionPtr();
	}
	if ((diff < 0 && variable->baseType.kind != AST::TypeString)
		|| (diff < -1 && variable->baseType.kind == AST::TypeString)
	) {
		cb->error = _("Not enought indeces");

		return AST::ExpressionPtr();
	}

	result = AST::ExpressionPtr(new AST::Expression);
	result->kind = AST::ExprArrayElement;
	if (variable->baseType.kind == AST::TypeString && diff == 0) {
		// Element of string: indeces count == array dimension + 1
		if (realArguments.size() > arguments.size()) {
			// String slice -> res type is string
			result->baseType.kind = AST::TypeString;
			result->isStringPart = true;
		} else {
			// String element -> res type is character
			result->baseType.kind = AST::TypeCharect;
			result->isStringPart = true;
		}
	} else {
		// Element of ordinary array
		result->baseType = variable->baseType;
	}
	result->dimension = 0;
	result->variable = variable;
	result->operands = realArguments;
	result->lexems = lexems;
	return result;
}


AST::ExpressionPtr  SyntaxAnalizer::parseSimpleName(const std::list<LexemPtr> &lexems,
	const AST::ModulePtr mod,
	const AST::AlgorithmPtr alg) const
{
	AST::ExpressionPtr  result;
	if (lexems.size() == 1 && lexems.front()->type == LxSecCurrentStringLength) {
		result = AST::ExpressionPtr(new AST::Expression);
		result->kind = AST::ExprFunctionCall;
		result->baseType = AST::TypeInteger;
		result->dimension = 0;
		result->lexems = QList<LexemPtr>::fromStdList(lexems);
		const AST::ModulePtr  dummy;
		AST::ModulePtr dummy2;
		QVariantList dummyTemplateParameters;
		findAlgorithm(QString::fromUtf8("длин"), dummy, AST::AlgorithmPtr(), dummy2, result->function, dummyTemplateParameters);
		return result;
	}
	if (lexems.size() >= 1 &&
		lexems.front()->type & LxTypeConstant &&
		lexems.front()->type != LxTypeConstant // Exact type of constant is known
	) {
		int err = 0;
		AST::VariableBaseType type = testConst(lexems, err);
		if (err) {
			lexems.front()->error = err;
			return AST::ExpressionPtr();
		} else {
			result = AST::ExpressionPtr(new AST::Expression);
			result->kind = AST::ExprConst;
			result->baseType.kind = type;
			int maxDim = 0;
			result->constant = parseConstant(lexems, type, maxDim);
			if (result->constant.type() == QVariant::Double) {
				result->baseType.kind = AST::TypeReal;
			}
			if (!result->constant.isValid()) {
				result = AST::ExpressionPtr();
			}
			return result;
		}
	}
	if (lexems.size() > 1 && lexems.front()->type == LxConstLiteral) {
		bool allLiterals = true;
		for (std::list<LexemPtr>::const_iterator it = lexems.begin(); it != lexems.end(); it++) {
			allLiterals = allLiterals && (*it)->type == LxConstLiteral;
		}
		if (allLiterals) {
			const QString err = _("Many quotes in constant");
			for (std::list<LexemPtr>::const_iterator it = lexems.begin(); it != lexems.end(); it++) {
				(*it)->error = err;
			}
			return AST::ExpressionPtr();
		}
	}
	if (lexems.size() == 1 && lexems.front()->type == LxSecNewline) {
		result = AST::ExpressionPtr(new AST::Expression);
		result->kind = AST::ExprConst;
		result->baseType.kind = AST::TypeCharect;
		result->dimension = 0;
		result->constant = QVariant(QChar('\n'));
		return result;
	} else {
		// First -- try to parse as a constant of custom type
		QString longLexem;
		for (std::list<LexemPtr>::const_iterator it = lexems.begin(); it != lexems.end(); it++) {
			if (it != lexems.begin()) {
				longLexem += " ";
			}
			longLexem += (*it)->data;
		}
		AST::Type userConstType;
		QVariant userConstValue;
		if (tryInputOperatorAlgorithm(longLexem, userConstType, userConstValue, mod)) {
			result = AST::ExpressionPtr(new AST::Expression);
			result->kind = AST::ExprConst;
			result->baseType = userConstType;
			result->dimension = 0;
			result->constant = userConstValue;
			for (std::list<LexemPtr>::const_iterator it = lexems.begin(); it != lexems.end(); it++) {
				LexemPtr lx = (*it);
				lx->type = LxTypeConstant;
			}
			return result;
		}
		// If not a custom type constant -- try more...
		QString name;
		bool expCandidate = true;
		for (std::list<LexemPtr>::const_iterator it = lexems.begin(); it != lexems.end(); it++) {
			if (it != lexems.begin()) {
				name += " ";
			}
			name += (*it)->data;
			expCandidate = expCandidate && (*it)->type == LxConstReal;
			if ((*it)->type == LxNameClass) {
				(*it)->error = _("Keyword in name");
				return AST::ExpressionPtr();
			}
		}
		QString err;
		if (expCandidate) {
			// If exp is correct, then it should be
			// caught above under condition lexems.size()==1
			err = _("Wrong E-real number");
		} else {
			err = lexer_->testName(name);
		}
		bool retval = lexer_->isReturnVariable(name);
		if (!err.isEmpty() && !retval) {
			for (std::list<LexemPtr>::const_iterator it = lexems.begin(); it != lexems.end(); it++) {
				(*it)->error = err;
			}
			return AST::ExpressionPtr();
		}

		// Catch keyword in name
		if (!retval) {
			for (std::list<LexemPtr>::const_iterator it = lexems.begin(); it != lexems.end(); it++) {
				LexemPtr lx = (*it);
				if (lx->type & LxTypePrimaryKwd || lx->type & LxTypeSecondaryKwd || lx->type == LxNameClass) {
					lx->error = _("Keyword in name");
					return AST::ExpressionPtr();
				}
			}
		}

		// Catch abrakadabra like 'yes "Something"' or 'yes 3' or 'yes "Something"'

		LexemPtr boolConst;
		LexemPtr numericConst;
		LexemPtr stringConst;
//        LexemType prevType = LxTypeEmpty;
		for (std::list<LexemPtr>::const_iterator it = lexems.begin(); it != lexems.end(); it++) {
			LexemPtr lx = (*it);
			LexemType curType = lx->type;
			if (curType == LxConstBoolTrue || curType == LxConstBoolFalse)
				if (!boolConst) {
					boolConst = lx;
				}
			if (curType == LxConstInteger || curType == LxConstReal)
				if (!numericConst) {
					numericConst = lx;
				}
			if (curType == LxConstLiteral)
				if (!stringConst) {
					stringConst = lx;
				}
		}

		if (boolConst) {
			boolConst->error = _("'%1' can't be part of name", boolConst->data);
			return AST::ExpressionPtr();
		}


		if (stringConst) {
			stringConst->error = _("Literal can't be part of name");
			return AST::ExpressionPtr();
		}

		if (numericConst && numericConst == (*lexems.begin())) {
			numericConst->error = _("Name starts with digit");
			return AST::ExpressionPtr();
		}

		AST::AlgorithmPtr  a;
		AST::ModulePtr am;
		AST::VariablePtr  v;
		QVariantList aTemplateParameters;

		err = "";
		if (retval) {
			for (std::list<LexemPtr>::const_iterator it = lexems.begin(); it != lexems.end(); it++) {
				(*it)->type = LxTypeSecondaryKwd;
			}
			if (!alg) {
				err = _("Access to return value outside of algorithm");
			}
			if (alg && alg->header.returnType.kind == AST::TypeNone) {
				err = _("This algorithm has no return value");
			}
			if (err.isEmpty()) {
				bool found = findLocalVariable(alg->header.name, alg, v);
				Q_ASSERT(found);
				Q_UNUSED(found);
				result = AST::ExpressionPtr(new AST::Expression);
				result->kind = AST::ExprVariable;
				result->baseType = alg->header.returnType;
				result->variable = v;
				return result;
			}
		} else {
			if (findAlgorithm(name, mod, alg, am, a, aTemplateParameters)) {
				foreach (LexemPtr lx, lexems) lx->type = LxNameAlg;
				if (a->header.arguments.size() - aTemplateParameters.size() > 0) {
					err = _("No arguments");
				} else {
					result = AST::ExpressionPtr(new AST::Expression);
					result->kind = AST::ExprFunctionCall;
					result->function = a;
					result->baseType = a->header.returnType;
					addTemplateParametersToFunctionCall(result, aTemplateParameters);
					return result;
				}
			} else if (findVariable(name, mod, alg, v)) {
				if (v->dimension > 0) {
					err = _("Array without index"); // FIXME: error code for array in expression
				} else {
					result = AST::ExpressionPtr(new AST::Expression);
					result->kind = AST::ExprVariable;
					result->variable = v;
					result->baseType = v->baseType;
					return result;
				}
			} else {
				err = _("Name not declared");
				// Try to find algorithms from disabled modules
				const QStringList possibleImports = possibleModuleImports(name, ast_, mod);
				if (possibleImports.size() > 0) {
					const TextStatement &sourceStatement =
						findSourceStatementByLexem(lexems.front());
					sourceStatement.suggestedImportModuleNames = possibleImports;
				}
			}
		}
		if (!err.isEmpty()) {
			for (std::list<LexemPtr>::const_iterator it = lexems.begin(); it != lexems.end(); it++) {
				(*it)->error = err;
			}
			return AST::ExpressionPtr();
		}
	}
	return result;
}

void SyntaxAnalizer::addTemplateParametersToFunctionCall(ExpressionPtr &callNode, const QVariantList &parameters)
{
	const AST::AlgorithmPtr &func = callNode->function;
	Q_ASSERT(func);
	Q_ASSERT(func->header.arguments.size() >= parameters.size());
	Q_ASSERT(callNode->operands.isEmpty());
	for (int i = 0; i < parameters.size(); i++) {
		AST::ExpressionPtr paramConst = AST::ExpressionPtr(new AST::Expression());
		paramConst->kind = AST::ExprConst;
		paramConst->baseType = func->header.arguments[i]->baseType;
		paramConst->dimension = func->header.arguments[i]->dimension;
		paramConst->constant = parameters[i];
		callNode->operands.append(paramConst);
	}
}

int findOperatorByPriority(const QList<SubexpressionElement> &s)
{
	int result = -1;
	static const QList<QSet <LexemType> > Omega = QList<QSet<LexemType> >()
		<< (QSet<LexemType>() << LxSecOr)
		<< (QSet<LexemType>() << LxSecAnd)
		<< (QSet<LexemType>() << LxOperBoolEqual << LxOperBoolNotEqual)
		<< (QSet<LexemType>() << LxSecNot)
		<< (QSet<LexemType>() << LxOperNotEqual << LxOperEqual << LxOperGreater << LxOperGreaterOrEqual << LxOperLess << LxOperLessOrEqual << LxPriAssign)
		<< (QSet<LexemType>() << LxOperPlus << LxOperMinus)
		<< (QSet<LexemType>() << LxOperAsterisk << LxOperSlash)
		<< (QSet<LexemType>() << LxOperPower);
	for (int i = 0; i < Omega.count(); i++) {
		for (int j = s.size() - 1; j >= 0; j--) {
			if (s[j].o && Omega[i].contains(s[j].o->type)) {
				result = j;
				break;
			}
		}
		if (result != -1) {
			break;
		}
	}
	return result;
}

#define IS_NUMERIC(x) ( x==AST::TypeInteger || x==AST::TypeReal )
#define IS_LITERAL(x) ( x==AST::TypeCharect || x==AST::TypeString )
#define IS_BOOLEAN(x) ( x==AST::TypeBoolean )

bool IS_NUMERIC_LIST(const QList<AST::ExpressionPtr> &list)
{
	bool result = true;
	for (int i = 0; i < list.size(); i++) {
		bool c = true;
		if (list[i]->baseType.kind == AST::TypeBoolean &&
			list[i]->kind == AST::ExprSubexpression) {
			c = IS_NUMERIC_LIST(list[i]->operands);
		} else {
			c = IS_NUMERIC(list[i]->baseType.kind);
		}
		result = result && c;
	}
	return result;
}

bool IS_LITERAL_LIST(const QList<AST::ExpressionPtr> &list)
{
	bool result = true;
	for (int i = 0; i < list.size(); i++) {
		result = result && IS_LITERAL(list[i]->baseType.kind);
	}
	return result;
}

bool IS_BOOLEAN_LIST(const QList<AST::ExpressionPtr> &list)
{
	bool result = true;
	for (int i = 0; i < list.size(); i++) {
		bool c = true;
		if (list[i]->baseType.kind == AST::TypeBoolean &&
			list[i]->kind == AST::ExprSubexpression) {
			c = IS_BOOLEAN_LIST(list[i]->operands);
		} else {
			c = IS_BOOLEAN(list[i]->baseType.kind);
		}
		result = result && c;
	}
	return result;
}

AST::ExpressionOperator operatorByLexem(const LexemPtr lx)
{
	if (lx->type == LxOperPlus) {
		return AST::OpSumm;
	} else if (lx->type == LxOperMinus) {
		return AST::OpSubstract;
	} else if (lx->type == LxOperAsterisk) {
		return AST::OpMultiply;
	} else if (lx->type == LxOperSlash) {
		return AST::OpDivision;
	} else if (lx->type == LxOperPower) {
		return AST::OpPower;
	} else if (lx->type == LxSecNot) {
		return AST::OpNot;
	} else if (lx->type == LxSecAnd) {
		return AST::OpAnd;
	} else if (lx->type == LxSecOr) {
		return AST::OpOr;
	} else if (lx->type == LxOperEqual || lx->type == LxOperBoolEqual) {
		return AST::OpEqual;
	} else if (lx->type == LxOperNotEqual || lx->type == LxOperBoolNotEqual) {
		return AST::OpNotEqual;
	} else if (lx->type == LxOperLess) {
		return AST::OpLess;
	} else if (lx->type == LxOperGreater) {
		return AST::OpGreater;
	} else if (lx->type == LxOperLessOrEqual) {
		return AST::OpLessOrEqual;
	} else if (lx->type == LxOperGreaterOrEqual) {
		return AST::OpGreaterOrEqual;
	} else {
		return AST::OpNone;
	}


}

AST::Type resType(const AST::Type &a
	, const AST::Type &b
	, AST::ExpressionOperator op)
{
	if (a == AST::TypeNone) {
		if (b == AST::TypeInteger && (op == AST::OpSumm || op == AST::OpSubstract)) {
			return AST::TypeInteger;
		}
		if (b == AST::TypeReal && (op == AST::OpSumm || op == AST::OpSubstract)) {
			return AST::TypeReal;
		}
		if (b == AST::TypeBoolean && op == AST::OpNot) {
			return AST::TypeBoolean;
		}
		return AST::TypeNone;
	}

	if (a == AST::TypeInteger) {
		if (b == AST::TypeInteger) {
			if (op == AST::OpSumm) {
				return AST::TypeInteger;
			}
			if (op == AST::OpSubstract) {
				return AST::TypeInteger;
			}
			if (op == AST::OpMultiply) {
				return AST::TypeInteger;
			}
			if (op == AST::OpDivision) {
				return AST::TypeReal;
			}
			if (op == AST::OpPower) {
				return AST::TypeInteger;
			}

			if (op == AST::OpGreater)

			{
				return AST::TypeBoolean;
			}
			if (op == AST::OpLess)

			{
				return AST::TypeBoolean;
			}
			if (op == AST::OpEqual) {
				return AST::TypeBoolean;
			}


			if (op == AST::OpNotEqual)

			{
				return AST::TypeBoolean;
			}
			if (op == AST::OpGreaterOrEqual)

			{
				return AST::TypeBoolean;
			}
			if (op == AST::OpLessOrEqual)

			{
				return AST::TypeBoolean;
			}

			if (op == AST::OpOr) {
				return AST::TypeNone;
			};
			if (op == AST::OpAnd) {
				return AST::TypeNone;
			};

		}

		if (b == AST::TypeReal) {
			if (op == AST::OpSumm) {
				return AST::TypeReal;
			}
			if (op == AST::OpSubstract) {
				return AST::TypeReal;
			}
			if (op == AST::OpMultiply) {
				return AST::TypeReal;
			}
			if (op == AST::OpDivision) {
				return AST::TypeReal;
			}
			if (op == AST::OpPower) {
				return AST::TypeReal;
			}

			if (op == AST::OpNotEqual)

			{
				return AST::TypeBoolean;
			}
			if (op == AST::OpGreaterOrEqual)

			{
				return AST::TypeBoolean;
			}
			if (op == AST::OpLessOrEqual)

			{
				return AST::TypeBoolean;
			}

			if (op == AST::OpGreater)

			{
				return AST::TypeBoolean;
			}
			if (op == AST::OpLess)

			{
				return AST::TypeBoolean;
			}
			if (op == AST::OpEqual) {
				return AST::TypeBoolean;
			}

			if (op == AST::OpOr) {
				return AST::TypeNone;
			};
			if (op == AST::OpAnd) {
				return AST::TypeNone;
			};


		}

		if (b == AST::TypeBoolean) {
			return AST::TypeNone;
		}

		if (b == AST::TypeCharect) {
			return AST::TypeNone;
		}

		if (b == AST::TypeString) {
			return AST::TypeNone;
		}
		return AST::TypeNone;

	} // end if (a==AST::TypeInteger)


	if (a == AST::TypeBoolean) {
		if (b == AST::TypeBoolean) {
			if (op == AST::OpOr) {
				return AST::TypeBoolean;
			};
			if (op == AST::OpAnd) {
				return AST::TypeBoolean;
			};

			if (op == AST::OpNotEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpEqual) {
				return AST::TypeBoolean;
			}
			return AST::TypeNone;

		}
		if (b == AST::TypeInteger) {
			return AST::TypeNone;
		}
		if (b == AST::TypeReal) {
			return AST::TypeNone;
		}
		if (b == AST::TypeCharect) {
			return AST::TypeNone;
		}
		if (b == AST::TypeString) {
			return AST::TypeNone;
		}

		return AST::TypeNone;
	} // end if (a==AST::TypeBoolean)

	if (a == AST::TypeReal) {
		if (b == AST::TypeInteger) {
			if (op == AST::OpSumm) {
				return AST::TypeReal;
			}
			if (op == AST::OpSubstract) {
				return AST::TypeReal;
			}
			if (op == AST::OpMultiply) {
				return AST::TypeReal;
			}
			if (op == AST::OpDivision) {
				return AST::TypeReal;
			}
			if (op == AST::OpPower) {
				return AST::TypeReal;
			}

			if (op == AST::OpGreater) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLess) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpEqual) {
				return AST::TypeBoolean;
			}

			if (op == AST::OpNotEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpGreaterOrEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLessOrEqual) {
				return AST::TypeBoolean;
			}

			return AST::TypeNone;

		}

		if (b == AST::TypeReal) {
			if (op == AST::OpSumm) {
				return AST::TypeReal;
			}
			if (op == AST::OpSubstract) {
				return AST::TypeReal;
			}
			if (op == AST::OpMultiply) {
				return AST::TypeReal;
			}
			if (op == AST::OpDivision) {
				return AST::TypeReal;
			}
			if (op == AST::OpPower) {
				return AST::TypeReal;
			}

			if (op == AST::OpGreater) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLess) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpEqual) {
				return AST::TypeBoolean;
			}

			if (op == AST::OpNotEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpGreaterOrEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLessOrEqual) {
				return AST::TypeBoolean;
			}
			return AST::TypeNone;

		}

		if (b == AST::TypeString) {
			return AST::TypeNone;
		}

		if (b == AST::TypeCharect) {
			return AST::TypeNone;
		}

		if (b == AST::TypeBoolean) {
			return AST::TypeNone;
		}
		return AST::TypeNone;

	} // end if (a==AST::TypeReal)

	if (a == AST::TypeString) {
		if (b == AST::TypeString) {
			if (op == AST::OpSumm) {
				return AST::TypeString;
			}
			if (op == AST::OpGreater) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLess) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpEqual) {
				return AST::TypeBoolean;
			}

			if (op == AST::OpNotEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpGreaterOrEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLessOrEqual) {
				return AST::TypeBoolean;
			}

			return AST::TypeNone;
		}
		if (b == AST::TypeCharect) {
			if (op == AST::OpSumm) {
				return AST::TypeString;
			}
			if (op == AST::OpGreater) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLess) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpEqual) {
				return AST::TypeBoolean;
			}

			if (op == AST::OpNotEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpGreaterOrEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLessOrEqual) {
				return AST::TypeBoolean;
			}

			return AST::TypeNone;
		}
		if (b == AST::TypeBoolean) {
			return AST::TypeNone;
		}
		if (b == AST::TypeReal) {
			return AST::TypeNone;
		}
		if (b == AST::TypeInteger) {
			return AST::TypeNone;
		}
		return AST::TypeNone;
	} //string


	if (a == AST::TypeCharect) {
		if (b == AST::TypeString) {
			if (op == AST::OpSumm) {
				return AST::TypeString;
			}
			if (op == AST::OpGreater) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLess) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpEqual) {
				return AST::TypeBoolean;
			}

			if (op == AST::OpNotEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpGreaterOrEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLessOrEqual) {
				return AST::TypeBoolean;
			}
			return AST::TypeNone;
		}
		if (b == AST::TypeCharect) {
			if (op == AST::OpSumm) {
				return AST::TypeString;
			}
			if (op == AST::OpGreater) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLess) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpEqual) {
				return AST::TypeBoolean;
			}

			if (op == AST::OpNotEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpGreaterOrEqual) {
				return AST::TypeBoolean;
			}
			if (op == AST::OpLessOrEqual) {
				return AST::TypeBoolean;
			}
			return AST::TypeNone;
		}
		if (b == AST::TypeBoolean) {
			return AST::TypeNone;
		}
		if (b == AST::TypeReal) {
			return AST::TypeNone;
		}
		if (b == AST::TypeInteger) {
			return AST::TypeNone;
		}
		return AST::TypeNone;
	} //string

	return AST::TypeNone;

}

struct CNFReference {
	AST::ExpressionPtr comparision;
	AST::ExpressionPtr operand;
};

static CNFReference findRightmostCNFSubexpression(AST::ExpressionPtr  e)
{
	static const QSet<AST::ExpressionOperator> ComparisonOperators = QSet<AST::ExpressionOperator>()
		<< AST::OpLess << AST::OpLessOrEqual << AST::OpEqual << AST::OpNotEqual << AST::OpGreaterOrEqual << AST::OpGreater;
	if (ComparisonOperators.contains(e->operatorr)) {
		CNFReference result;
		result.comparision = e;
		result.operand = e->operands.last();
		return result;
	} else {
		return findRightmostCNFSubexpression(e->operands.last());
	}
}

AST::ExpressionPtr  SyntaxAnalizer::makeExpressionTree(const QList<SubexpressionElement> &s, const AST::ModulePtr currentModule) const
{
	if (s.isEmpty()) {
		return AST::ExpressionPtr();
	}
	int l = findOperatorByPriority(s);
	if (l == -1) {
		for (int i = 0; i < s.size(); i++) {
			if (s[i].e) {
				return s[i].e;
			}
		}
	} else {
		if (s[l].o->type == LxPriAssign) {
			s[l].o->error = _("':=' or '=' ?");
			return AST::ExpressionPtr();
		}
		QList<SubexpressionElement> head = s.mid(0, l);
		QList<SubexpressionElement> tail = s.mid(l + 1);
		if (head.isEmpty()) {
			static QSet<LexemType> UnaryOperations = QSet<LexemType>()
				<< LxOperPlus << LxOperMinus << LxSecNot;
			if (!UnaryOperations.contains(s[l].o->type)) {
				s[l].o->error = _("No operand before non-unary operator");
				return AST::ExpressionPtr();
			}
		}
		if (head.isEmpty() && tail.isEmpty()) {
			s[l].o->error = _("No operands");
			return AST::ExpressionPtr();
		}
		if (tail.isEmpty()) {
			s[l].o->error = _("Extra operator");
			return AST::ExpressionPtr();
		}
		AST::ExpressionPtr  headExpr = makeExpressionTree(head, currentModule);
		AST::ExpressionPtr  tailExpr = makeExpressionTree(tail, currentModule);
		if (!head.isEmpty() && !headExpr) {
			return AST::ExpressionPtr();
		}
		if (!tailExpr) {
			return AST::ExpressionPtr();
		}
		AST::Type headType = headExpr ? headExpr->baseType : AST::Type();
		AST::Type tailType = tailExpr->baseType;
		static const QSet<LexemType> ComparisonOperators = QSet<LexemType>()
			<< LxOperLess << LxOperLessOrEqual << LxOperEqual << LxOperNotEqual << LxOperGreaterOrEqual << LxOperGreater << LxPriAssign;
		static const QSet<LexemType> CheckEqualOperators = QSet<LexemType>()
			<< LxOperBoolEqual << LxOperBoolNotEqual;

		bool tailIsNumeric = IS_NUMERIC(tailType.kind);
		bool headIsBool = headType.kind == AST::TypeBoolean;
		bool hasHeadExpr = headExpr != NULL;
		bool headIsSubexpr = hasHeadExpr && headExpr->kind == AST::ExprSubexpression && headExpr->operands.size() >= 2;
		bool isComparision = ComparisonOperators.contains(s[l].o->type);
		bool isEqualCheck = CheckEqualOperators.contains(s[l].o->type);
		bool numericOperands = headExpr && IS_NUMERIC_LIST(headExpr->operands);
		bool tailIsLiteral = IS_LITERAL(tailType.kind);
		bool tailIsBoolean = IS_BOOLEAN(tailType.kind);
		bool literalOperands = headExpr && IS_LITERAL_LIST(headExpr->operands);
		bool booleanOperands = headExpr && IS_BOOLEAN_LIST(headExpr->operands);

		bool makeCNF = headIsBool && headIsSubexpr && (
				(isComparision && tailIsNumeric && numericOperands)
				||
				(isComparision && tailIsLiteral && literalOperands)
				||
				(isEqualCheck && tailIsBoolean && booleanOperands)
			);

		if (headExpr && headExpr->expressionIsClosed) {
			makeCNF = false;
		}

		if (makeCNF) {
			AST::ExpressionPtr  res = AST::ExpressionPtr(new AST::Expression);
			res->kind = AST::ExprSubexpression;
			res->baseType.kind = AST::TypeBoolean;
			res->operands << headExpr;
			res->operatorr = AST::OpAnd;
			AST::ExpressionPtr  subRes = AST::ExpressionPtr(new AST::Expression);
			subRes->kind = AST::ExprSubexpression;
			subRes->baseType.kind = AST::TypeBoolean;
//            subRes->operands << new AST::Expression(headExpr->operands.last());
			CNFReference cnfReference = findRightmostCNFSubexpression(headExpr);
			AST::ExpressionPtr cacheReference = cnfReference.operand;
			AST::ExpressionPtr comparisionReference = cnfReference.comparision;
			AST::ExpressionPtr firstOperand = AST::ExpressionPtr(new AST::Expression(cacheReference));
			subRes->operands << firstOperand;
			firstOperand->useFromCache = true;
			cacheReference->keepInCache = true;
			comparisionReference->clearCacheOnFailure = true;
			firstOperand->cacheReference = cacheReference.toWeakRef();
//            subRes->operands << findRightmostCNFSubexpression(AST::ExpressionPtr(new AST::Expression(headExpr)));
			subRes->operands << tailExpr;
			subRes->operatorr = operatorByLexem(s[l].o);
			res->operands << subRes;
			return res;
		} else if (!headExpr && tailExpr->kind == AST::ExprConst) {
			// Merge unary +, - and 'not' into constant
			if (s[l].o->type == LxOperPlus) {
				if (tailExpr->baseType.kind == AST::TypeInteger) {
					// pass
				} else if (tailExpr->baseType.kind == AST::TypeReal) {
					// pass
				} else {
					s[l].o->error = _("Can't +%1", lexer_->classNameByBaseType(tailExpr->baseType.kind));
				}
			} else if (s[l].o->type == LxOperMinus) {
				if (tailExpr->baseType.kind == AST::TypeInteger) {
					tailExpr->constant = QVariant(0 - tailExpr->constant.toInt());
				} else if (tailExpr->baseType.kind == AST::TypeReal) {
					tailExpr->constant = QVariant(0.0 - tailExpr->constant.toDouble());
				} else {
					s[l].o->error = _("Can't -%1", lexer_->classNameByBaseType(tailExpr->baseType.kind));
				}
			} else if (s[l].o->type == LxSecNot) {
				if (tailExpr->baseType.kind == AST::TypeBoolean) {
					tailExpr->constant = QVariant(! tailExpr->constant.toBool());
				} else {
					s[l].o->error = _("Can't not %1", lexer_->classNameByBaseType(tailExpr->baseType.kind));
				}
			}
			return tailExpr;
		} else {
			AST::ExpressionOperator operation = operatorByLexem(s[l].o);
			AST::Type rt = resType(headType, tailType, operation);
			if (rt.kind == AST::TypeNone && headExpr && tailExpr) {
				// Try to find custom-overriden operator
				const QString &operatorName = s[l].o->data;
				AST::ExpressionPtr  customOperation = makeCustomBinaryOperation(operatorName, headExpr, tailExpr, currentModule);
				if (customOperation) {
					customOperation->operatorr = operation;
					return customOperation;
				}
			}
			if (rt.kind == AST::TypeNone) {
				QString type1name, type2name;
				if (headType.kind != AST::TypeUser) {
					type1name = lexer_->classNameByBaseType(headType.kind);
				} else {
					type1name = headType.name;
				}
				if (tailType.kind != AST::TypeUser) {
					type2name = lexer_->classNameByBaseType(tailType.kind);
				} else {
					type2name = tailType.name;
				}
				s[l].o->error = _("Can't %1 %2 %3"
						, type1name
						, s[l].o->data
						, type2name
					);
				return AST::ExpressionPtr();
			}
			AST::ExpressionPtr  res = AST::ExpressionPtr(new AST::Expression);
			res->kind = AST::ExprSubexpression;
			res->baseType = rt;
			if (headExpr) {
				res->operands << headExpr;
			}
			res->operands << tailExpr;
			res->operatorr = operation;
			return res;
		}

	}
	return AST::ExpressionPtr();
}



SyntaxAnalizer::~SyntaxAnalizer()
{
}

} // namespace KumirAnalizer
