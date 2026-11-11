#ifndef ANALIZER_H
#define ANALIZER_H

#include "statement.h"
#include <kumir2/analizerinterface.h>
#include <kumir2/analizer_instanceinterface.h>
#include <kumir2/actorinterface.h>

#include <kumir2-libs/dataformats/ast.h>

#include <vector>
#include <QSet>
class QDir;

namespace KumirAnalizer
{

class Analizer : public QObject,
	public Shared::Analizer::InstanceInterface,
	public Shared::Analizer::ASTCompilerInterface,
	public Shared::Analizer::HelperInterface
{
	Q_OBJECT
	Q_INTERFACES(Shared::Analizer::InstanceInterface
		Shared::Analizer::ASTCompilerInterface
		Shared::Analizer::HelperInterface
	)
	friend class AnalizerPrivate;

public:
	explicit Analizer(class KumirAnalizerPlugin *plugin, bool teacherMode);
	~Analizer();

	Shared::AnalizerInterface *plugin();

	/**
	  * Set application-wide (while initialization)
	  * Kumir source language (Russian, Ukrainian, etc.)
	  */
	static void setSourceLanguage(const QDir &resourcesRoot, const QLocale::Language &language);
	static void setModuleAlwaysAvailable(const QByteArray &moduleName);
	void connectSignalImportsChanged(QObject *receiver, const char *slot);
	bool isKnownLexem(const QString &lexem, int lineNo, int colNo, const QString &context) const;

public slots:
	void setSourceDirName(const QString &dirName);
	Shared::Analizer::TextAppend closingBracketSuggestion(int lineNo) const;
	QStringList importModuleSuggestion(int lineNo) const;
	QString sourceText() const;
	std::string rawSourceData() const;
	void setSourceText(const QString &text);

	QList<Shared::Analizer::Suggestion> suggestAutoComplete(
		int lineNo,
		const QString &before,
		const QString &after
	) const;

	Shared::Analizer::ApiHelpItem itemUnderCursor(
		const QString &text,
		int lineNo, int colNo,
		bool includeRightBound
	) const;


	QList<Shared::Analizer::Error> errors() const;

	QList<Shared::Analizer::LineProp> lineProperties() const;

	bool multipleStatementsInLine(int lineNo) const;

	Shared::Analizer::LineProp lineProp(int lineNo, const QString &text) const;

	QStringList algorithmsAvailabaleForModule(const AST::ModulePtr currentModule) const;
	static bool isModuleAlwaysEnabled(const AST::ModulePtr module);
	static bool isModuleAlwaysEnabled(const QString &localizedName);
	QStringList moduleNames() const;

	QList<QPoint> lineRanks() const;

	QStringList imports() const;
	QString createImportStatementLine(const QString &importName) const;

	const AST::DataPtr abstractSyntaxTree() const;

	const AST::ModulePtr findModuleByLine(int lineNo) const;

	QString suggestFileName() const;
	QRegExp lineCommentStartLexemPattern() const;


private /*typedefs*/:

	typedef AST::Data AST_Data;
	typedef QList<AST::StatementPtr> *LAS;

	enum CompilationStage {
		CS_StructureAndNames, CS_Contents
	};

	struct ModuleStatementsBlock {
		QList<TextStatementPtr> statements;
		TextStatementPtr begin;
		TextStatementPtr end;
		bool teacher;

		ModuleStatementsBlock(): teacher(false) {}
		operator bool() const
		{
			return statements.size() > 0;
		}
	};

private /*methods*/:

	const AST::AlgorithmPtr findAlgorhitmByLine(
		const AST::ModulePtr mod,
		int lineNo
	) const;

	void createModuleFromActor_stage1(
		Shared::ActorInterface *actor,
		quint8 forcedId
	);

	void createModuleFromActor_stage2(
		Shared::ActorInterface *actor
	);

	QStringList gatherExtraTypeNames(
		const AST::ModulePtr currentModule
	) const;

	void removeAllVariables(const AST::VariablePtr var);


	/** Find algorhitm in AST by real line number */
	static AST::AlgorithmPtr findAlgorhitmByPos(AST::DataPtr data, int pos);

	/** Find begin/end iterators in AST instructions list,
	  * containing provided lexem groups
	  * @param IN data - AST
	  * @param IN statements - list of lexem groups
	  * @param OUT lst - list of AST instructions
	  * @param OUT begin - begin iterator
	  * @param OUT end - end iterator
	  * @param OUT mod - module reference
	  * @param OUT alg - algorhitm reference
	  * @return true on found, false if not found
	  */
	static bool findInstructionsBlock(
		AST::DataPtr data,
		const QList<TextStatement *> _statements,
		LAS &lst,
		int &begin, int &end,
		AST::ModulePtr &mod,
		AST::AlgorithmPtr &alg
	);

	/** Find context of AST instructions list,
	  * containing provided lexem groups
	  * @param IN data - AST
	  * @param IN statements - list of lexem groups
	  * @param IN pos - position, where lexems inserted
	  * @param OUT lst - list of AST instructions
	  * @param OUT outPos - position, where statements to be insert
	  * @param OUT mod - module reference
	  * @param OUT alg - algorhitm reference
	  * @return true on found, false if not found
	  */
	static bool findInstructionsBlock(
		AST::DataPtr data,
		const QList<TextStatement *> _statements,
		int pos,
		LAS &lst,
		int &outPos,
		AST::ModulePtr &mod,
		AST::AlgorithmPtr &alg
	);

	/** Find instruction in AST by source text position
	 * @param IN data - AST
	 * @param IN statements - list of lexem groups
	 * @param IN lineNo - line number in source text
	 * @param IN colNo - column number in source text line
	 * @return pointer to lexem group and lexem, or null pointers pair if not found
	 */
	static QPair<TextStatementPtr, AST::LexemPtr> findSourceLexemContext(
		AST::DataPtr data,
		const QList<TextStatementPtr> _statements,
		int lineNo, int colNo,
		bool includeRightBound
	);

	void doCompilation(
		QList<TextStatementPtr> &allStatements,
		CompilationStage stage
	);

	static QList<struct ModuleStatementsBlock> splitIntoModules(
		const QList<TextStatementPtr> &_statements
	);

private /*fields*/:
	std::vector<Shared::ActorInterface *> _builtinModules;

	class Lexer *_lexer;
	class PDAutomata *_pdAutomata;
	class SyntaxAnalizer *_syntaxAnalizer;
	AST::DataPtr _ast;

	QStringList _sourceText;
	QList<TextStatementPtr> _statements;

	static QSet<QString> _AlwaysAvailableModulesName;
	static QLocale::Language _NativeLanguage;

	bool _teacherMode;
	class KumirAnalizerPlugin *_plugin;
};

} // namespace KumirAnalizer

#endif // ANALIZER_H
