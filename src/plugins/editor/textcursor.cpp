#include "editor.h"
#include "textcursor.h"
#include "textdocument.h"
#include "clipboard.h"
#include "editcommands.h"
#include "settingspage.h"
#include <kumir2-libs/extensionsystem/pluginmanager.h>
#include <QApplication>
#include <QDebug>

namespace Editor
{

TextCursor::TextCursor(EditorInstance *editor)
	: QObject(editor)
	, editor_(editor)
	, editMode_(EM_Insert)
	, viewMode_(VM_Hidden)
	, blinkTimerId_(-1)
	, enabledFlag_(true)
	, visibleFlag_(false)
	, row_(0)
	, column_(0)
	, keptColumn_(-1)
	, recordingMacro_(nullptr)

{
	teacherModeFlag_ = false;
	blinkTimerId_ = startTimer(QApplication::cursorFlashTime() / 2);
	emitPositionChanged();

	selectionRect_ = QRect(-1, -1, 0, 0);
}

void TextCursor::setViewMode(ViewMode mode)
{
	viewMode_ = mode;
	if (mode == VM_Hidden) {
		visibleFlag_ = false;
	} else if (mode == VM_Visible) {
		visibleFlag_ = true;
	}
	emit updateRequest();
}



void TextCursor::emitPositionChanged()
{

	emit positionChanged(row_, column_);

}

void TextCursor::timerEvent(QTimerEvent *e)
{
	if (viewMode_ == VM_Blinking) {
		visibleFlag_ = !visibleFlag_;
	} else if (viewMode_ == VM_Hidden) {
		visibleFlag_ = false;
	} else if (viewMode_ == VM_Visible) {
		visibleFlag_ = true;
	}
	emit updateRequest();
	e->accept();

}

TextCursor::~TextCursor()
{
	if (blinkTimerId_ != -1) {
		killTimer(blinkTimerId_);
	}
}

void TextCursor::toggleComment()
{
	if (!enabledFlag_) {
		return;
	}
	if (editor_->analizerInstance_ == 0) {
		return;
	}
	if (modifiesProtectedLiines()) {
		return;
	}
	if (hasSelection()) {
		int start = -1;
		int end = -1;
		for (uint i = 0; i < editor_->document()->linesCount(); i++) {
			if (editor_->document()->lineEndSelectedAt(i) || editor_->document()->selectionMaskAt(i).contains(true)) {
				if (start == -1) {
					start = i;
				}
				end = i;
			}
		}
		editor_->document()->undoStack()->push(new ToggleCommentCommand(
				editor_->document(),
				start,
				end,
				this,
				editor_->analizerInstance_
			));
	} else if (hasRectSelection()) {
		int start = selectionRect_.top();
		int end = selectionRect_.bottom();
		editor_->document()->undoStack()->push(new ToggleCommentCommand(
				editor_->document(),
				start,
				end,
				this,
				editor_->analizerInstance_
			));
	} else {
		if (row_ < editor_->document()->linesCount()) {
			editor_->document()->undoStack()->push(new ToggleCommentCommand(
					editor_->document(),
					row_,
					row_,
					this,
					editor_->analizerInstance_
				));
		}
	}
}

void TextCursor::toggleLock()
{
	if (!enabledFlag_) {
		return;
	}
	TextDocument *doc = editor_->document();
	if (hasSelection()) {
		int start = -1;
		int end = -1;
		for (uint i = 0; i < doc->linesCount(); i++) {
			if ((i > 0 && doc->lineEndSelectedAt(i - 1)) || doc->selectionMaskAt(i).contains(true)) {
				if (start == -1) {
					start = i;
				}
				end = i;
			}
		}
		bool allProtected = true;
		for (int i = start; i <= end; ++i) {
			const TextLine &line = doc->at(i);
			allProtected = allProtected && line.protecteed;
		}

		bool newFlag = allProtected ? false : true;
		for (int i = start; i <= end; ++i) {
			TextLine &line = doc->at(i);
			line.protecteed = newFlag;
		}
	} else {
		if (row_ < doc->linesCount()) {
			doc->at(row_).protecteed = ! doc->at(row_).protecteed;
		}
	}
	emit updateRequest();
}

bool TextCursor::isFreeCursorMovement() const
{
	using namespace Shared;
	uint settingValue = editor_->mySettings()->value(
			SettingsPage::KeyFreeCursorMovement,
			SettingsPage::DefaultFreeCursorMovement
		).toUInt();

	SettingsPage::FreeCursorMovementType setting =
		SettingsPage::FreeCursorMovementType(settingValue);


	switch (setting) {

	case SettingsPage::Always:
		return true;
		break;

	case SettingsPage::TextsOnly:
		return editor_->analizerInstance_ == nullptr;
		break;

	default: {
		if (editor_->analizerInstance_) {
			const QList<LexemType> &lineProp = editor_->document()->highlightAt(row_);
			static const QList<LexemType> AllowedLexemsForFreeCursor
				= QList<LexemType>()
					<< LxTypeComment << LxTypeDoc;

			const int indentColumns = 2 * editor_->document()->indentAt(row_);
			const uint textPos = qMax(0, int(column_) - indentColumns);

			const bool hasProp = lineProp.size() > 0;
			if (!hasProp) {
				return false;
			}
			LexemType lxType = textPos < (uint) lineProp.size()
				? lineProp.at(textPos)
				: lineProp.last();
			bool result = AllowedLexemsForFreeCursor.contains(lxType);
			return result;
		} else {
			return true;
		}
	}

	}
	return false; // Prevent some compilers warning
}

void TextCursor::startRecordMacro()
{
	recordingMacro_ = QSharedPointer<Macro>(new Macro);
}

QSharedPointer<Macro> TextCursor::endRecordMacro()
{
	QSharedPointer<Macro> result = recordingMacro_;
	recordingMacro_.clear();
	return result;
}

void TextCursor::normalizePlainText(QString &s)
{
	// 1. Replace text processor specific characters
	static const QString from = QString::fromUtf8("–«»“”");
	static const QString to = QString::fromLatin1("-\"\"\"\"");
	Q_ASSERT(from.length() == to.length());
	for (int i = 0; i < from.length(); i++) {
		const QChar &f = from[i];
		const QChar &t = to[i];
		s = s.replace(f, t);
	}

	// 2. Replace leading indents
	QStringList lines = s.split('\n', QString::KeepEmptyParts);
	const int textLength = s.length();
	s.clear();
	s.reserve(textLength);
	for (int i = 0; i < lines.size(); ++i) {
		QString &line = lines[i];
		while (line.startsWith(". ")) {
			line.remove(0, 2);
		}
	}
	s = lines.join(QString::fromLatin1("\n"));
}

void TextCursor::evaluateCommand(const KeyCommand &command)
{
	if (recordingMacro_) {
		recordingMacro_->commands.push_back(command);
	}
	uint prevRow = row_;
	uint prevCol = column_;
	uint prevLines = editor_->document()->linesCount();
	bool clearCurrentLineError = false;
	switch (command.type) {
	case KeyCommand::MoveToNextChar:
		movePosition(QTextCursor::NextCell, TextCursor::MM_Move);
		break;
	case KeyCommand::MoveToNextLexem:
		movePosition(QTextCursor::NextWord, TextCursor::MM_Move);
		break;
	case KeyCommand::SelectNextLexem:
		movePosition(QTextCursor::NextWord, TextCursor::MM_Select);
		break;
	case KeyCommand::MoveToPreviousLexem:
		movePosition(QTextCursor::PreviousWord, TextCursor::MM_Move);
		break;
	case KeyCommand::SelectPreviousLexem:
		movePosition(QTextCursor::PreviousWord, TextCursor::MM_Select);
		break;
	case KeyCommand::SelectNextChar:
		movePosition(QTextCursor::NextCell, TextCursor::MM_Select);
		break;
	case KeyCommand::SelectNextColumn:
		movePosition(QTextCursor::NextCell, TextCursor::MM_RectSelect);
		break;
	case KeyCommand::MoveToPreviousChar:
		movePosition(QTextCursor::PreviousCell, TextCursor::MM_Move);
		break;
	case KeyCommand::SelectPreviousChar:
		movePosition(QTextCursor::PreviousCell, TextCursor::MM_Select);
		break;
	case KeyCommand::SelectPreviousColumn:
		movePosition(QTextCursor::PreviousCell, TextCursor::MM_RectSelect);
		break;
	case KeyCommand::MoveToNextLine:
		movePosition(QTextCursor::NextRow, TextCursor::MM_Move);
		break;
	case KeyCommand::SelectNextLine:
		movePosition(QTextCursor::NextRow, TextCursor::MM_Select);
		break;
	case KeyCommand::SelectNextRow:
		movePosition(QTextCursor::NextRow, TextCursor::MM_RectSelect);
		break;
	case KeyCommand::MoveToPreviousLine:
		movePosition(QTextCursor::PreviousRow, TextCursor::MM_Move);
		break;
	case KeyCommand::SelectPreviousLine:
		movePosition(QTextCursor::PreviousRow, TextCursor::MM_Select);
		break;
	case KeyCommand::SelectPreviousRow:
		movePosition(QTextCursor::PreviousRow, TextCursor::MM_RectSelect);
		break;
	case KeyCommand::MoveToStartOfLine:
		movePosition(QTextCursor::StartOfBlock, TextCursor::MM_Move);
		break;
	case KeyCommand::SelectStartOfLine:
		movePosition(QTextCursor::StartOfBlock, TextCursor::MM_Select);
		break;
	case KeyCommand::MoveToEndOfLine:
		movePosition(QTextCursor::EndOfBlock, TextCursor::MM_Move);
		break;
	case KeyCommand::SelectEndOfLine:
		movePosition(QTextCursor::EndOfBlock, TextCursor::MM_Select);
		break;
	case KeyCommand::MoveToPreviousPage:
		movePosition(QTextCursor::PreviousRow, TextCursor::MM_Move, 10);
		break;
	case KeyCommand::SelectPreviousPage:
		movePosition(QTextCursor::PreviousRow, TextCursor::MM_Select, 10);
		break;
	case KeyCommand::MoveToNextPage:
		movePosition(QTextCursor::NextRow, TextCursor::MM_Move, 10);
		break;
	case KeyCommand::SelectNextPage:
		movePosition(QTextCursor::NextRow, TextCursor::MM_Select, 10);
		break;
	case KeyCommand::MoveToStartOfDocument:
		movePosition(QTextCursor::Start, TextCursor::MM_Move);
		break;
	case KeyCommand::SelectStartOfDocument:
		movePosition(QTextCursor::Start, TextCursor::MM_Select);
		break;
	case KeyCommand::MoveToEndOfDocument:
		movePosition(QTextCursor::End, TextCursor::MM_Move);
		break;
	case KeyCommand::SelectEndOfDocument:
		movePosition(QTextCursor::End, TextCursor::MM_Select);
		break;
	case KeyCommand::SelectAll:
		movePosition(QTextCursor::Start, TextCursor::MM_Move);
		movePosition(QTextCursor::End, TextCursor::MM_Select);
		break;
	case KeyCommand::Copy:
		if (hasSelection()) {
			ClipboardData data;
			data.text = selectedText();
			data.type = ClipboardData::Text;
			int rstart, rend, cstart, cend;
			selectionBounds(rstart, cstart, rend, cend);
			uint fromLine = qMax(0, rstart);
			uint toLine = qMin(rend, int(editor_->document()->linesCount()));
			data.rtf = editor_->document()->toRtf(fromLine, toLine);
			Clipboard::instance()->push(data);
		} else if (hasRectSelection()) {
			ClipboardData data;
			data.text = selectedText();
			data.type = ClipboardData::Block;
			data.block = rectSelectionText();
			uint fromLine = qMax(0, selectionRect_.top());
			uint toLine = qMin(selectionRect_.bottom(), int(editor_->document()->linesCount()));
			data.rtf = editor_->document()->toRtf(fromLine, toLine);
			Clipboard::instance()->push(data);
		}
		break;

	case KeyCommand::InsertText:
		insertText(command.text);
		clearCurrentLineError = !command.text.contains("\n");
		break;
	case KeyCommand::Backspace:
		clearCurrentLineError = true;
		removePreviousChar();
		break;
	case KeyCommand::Delete:
		clearCurrentLineError = column_ >= 2 * editor_->document()->indentAt(row_) || hasSelection() || hasRectSelection();
		removeCurrentChar();
		break;
	case KeyCommand::RemoveLine:
		removeCurrentLine();
		break;
	case KeyCommand::RemoveTail:
		clearCurrentLineError = true;
		removeLineTail();
		break;
	case KeyCommand::Cut:
		evaluateCommand(KeyCommand::Copy);
		if (hasSelection()) {
			clearCurrentLineError = !selectedText().contains("\n");
			removeSelectedText();
		} else if (hasRectSelection()) {
			removeRectSelection();
		}
		break;
	case KeyCommand::Paste:
		if (Clipboard::instance()->hasContent()) {
			ClipboardData data = Clipboard::instance()->content();
			if (data.type == ClipboardData::Block) {
				insertBlock(data.block);
			} else if (data.type == ClipboardData::Text) {
				QString textToInsert = data.text;
				bool removeLeadingSpaces = false;
				if (editor_->analizer() &&
					Shared::AnalizerInterface::HardIndents == editor_->analizerPlugin_->indentsBehaviour()) {
					if (editor_->analizer()) {
						normalizePlainText(textToInsert);
						// Check if must remove leading spaces
						if (row() >= editor_->document()->linesCount()) {
							removeLeadingSpaces = true;
						} else {
							QString currentLineText =
								editor_->document()->at(row()).text;
							bool spacesOnlyBeforeCursor = true;
							for (uint i = 0; i < (uint) column(); i++) {
								if ((uint) currentLineText.length() >= i) {
									break;
								}
								if (!currentLineText.at(i).isSpace()) {
									spacesOnlyBeforeCursor = false;
									break;
								}
							}
							if (spacesOnlyBeforeCursor) {
								removeLeadingSpaces = true;
							}
						}
					}
				}
				if (removeLeadingSpaces) {
					QRegExp rxLeadingSpaces("^\\s+");
					QRegExp rxLineComment = editor_->analizer()->helper()
						? editor_->analizer()->helper()->lineCommentStartLexemPattern()
						: QRegExp();
					QStringList lines = textToInsert.split("\n", QString::KeepEmptyParts);
					for (int i = 0; i < lines.size(); i++) {
						QString &line = lines[i];
						if (0 == rxLeadingSpaces.indexIn(line)) {
							const int commentPos = rxLineComment.indexIn(line);
							const int leadingSpacesCount = rxLeadingSpaces.cap().length();
							if (commentPos != leadingSpacesCount) {
								line = line.mid(leadingSpacesCount);
							}
						}
					}
					textToInsert = lines.join("\n");
				}
				insertText(textToInsert);
				clearCurrentLineError = !data.text.contains("\n");
			}
		}
		break;
	case KeyCommand::ToggleComment:
		toggleComment();
		clearCurrentLineError = true;
		break;
	case KeyCommand::InsertImport:
		insertImport(command.text);
	default:
		break;
	}
	if (clearCurrentLineError && row_ < editor_->document()->linesCount()) {
		editor_->document()->marginAt(row_).errors.clear();
		TextLine &line = editor_->document()->at(row_);
		for (int lxNo = 0; lxNo < line.highlight.size(); lxNo++) {
			line.highlight[lxNo] = Shared::LexemType(
					line.highlight[lxNo] & ~Shared::LxTypeError
				);
		}
		line.changed = true;
	}

	if (prevRow != row_ || prevLines != editor_->document()->linesCount()) {
		editor_->document()->checkForCompilationRequest(QPoint(column(), row()));
		if (editor_->analizerInstance_ && editor_->analizerInstance_->helper()
			&& editor_->mySettings()->value(
				SettingsPage::KeyAutoInsertPairingBraces,
				SettingsPage::DefaultAutoInsertPairingBraces).toBool()
			&& command.type == KeyCommand::InsertText
			&& command.text == "\n") {
			// Try to complete closing bracket
			Shared::Analizer::TextAppend append =
				editor_->analizerInstance_->helper()->closingBracketSuggestion(prevRow);
			// Ask for a compiler
			if (append.first.length() > 0) {
				// If found suggesstion
				const QStringList extraLines = append.first.split('\n', QString::KeepEmptyParts);
				for (int i = 0; i < extraLines.size(); i++) {
					// insert closing bracket text lines
					QString line = extraLines[i];
					evaluateCommand(KeyCommand::MoveToStartOfLine);
					if (i < extraLines.size() - 1) {
						line += "\n";
					}
					evaluateCommand(line);
				}
				for (quint32 i = 0; i < append.second; i++)
					// move cursor back required times
				{
					evaluateCommand(KeyCommand::MoveToPreviousLine);
				}
				evaluateCommand(KeyCommand::MoveToEndOfLine);
				if (editor_->document()->textAt(row_).trimmed().length() > 0) {
					evaluateCommand(" ");
				}
			}

			// Try to import module used
			const QStringList modulesList =
				editor_->analizerInstance_->helper()->importModuleSuggestion(prevRow);
			if (modulesList.size() == 1) {
				// Found explicit module name
				evaluateCommand(KeyCommand(
						KeyCommand::InsertImport,
						modulesList.first()
					));
			}

			// Try to continue line comment
			if (prevRow >= 0 && prevRow < editor_->document()->linesCount()) {
				const TextLine &prevLine = editor_->document()->at(prevRow);
				if (prevLine.text.length() > 0) {
					int i = prevLine.highlight.size() - 1;
					Shared::LexemType lastLexem = prevLine.highlight[i];
					while (i > 0 && Shared::LxTypeEmpty == lastLexem) {
						lastLexem = prevLine.highlight[--i];
					}
					if (Shared::LxTypeComment == lastLexem) {
						QRegExp commentSymbolPattern =
							editor_->analizerInstance_->helper()->lineCommentStartLexemPattern();
						const int commentPos = commentSymbolPattern.indexIn(prevLine.text);
						const int prevIndent = 2 * editor_->document()->indentAt(prevRow);
						const int prevTextPos = prevCol - prevIndent;
						if (-1 != commentPos && prevTextPos >= commentPos) {
							const QString commentLexem = commentSymbolPattern.cap();
							QString textToInsert = QString(" ").repeated(commentPos) + commentLexem + " ";
							evaluateCommand(KeyCommand(KeyCommand::InsertText, textToInsert));
							editor_->document()->checkForCompilationRequest(QPoint(column(), row()));
						}
					}
				}
			}
		}
	}
	if (ExtensionSystem::PluginManager::instance()->currentGlobalState() == PluginInterface::GS_Observe
		&& command.type & KeyCommand::CommandModifiesTextMask
	) {
		ExtensionSystem::PluginManager::instance()->switchGlobalState(PluginInterface::GS_Unlocked);
	}
}

void TextCursor::moveTo(int row, int col)
{
	bool hardIndents = editor_->analizer() &&
		Shared::AnalizerInterface::HardIndents == editor_->analizer()->plugin()->indentsBehaviour();
	visibleFlag_ = false;
	updateRequest();
	row_ = qMax(0, row);
	column_ = qMax(0, col);
	if (!isFreeCursorMovement()) {
		uint indent = hardIndents ? qMax(0, int(2 * editor_->document()->indentAt(row_))) : 0;
		uint maxColumn = indent + editor_->document()->textAt(row_).length();
		column_ = qMin(column_, maxColumn);
	}
	if (viewMode_ != VM_Hidden) {
		visibleFlag_ = true;
		updateRequest();
	}
	emitPositionChanged();
}

void TextCursor::selectRangeText(int fromRow, int fromCol, int toRow, int toCol)
{
	bool hardIndents = editor_->analizer() &&
		Shared::AnalizerInterface::HardIndents == editor_->analizer()->plugin()->indentsBehaviour();
	visibleFlag_ = false;
	updateRequest();

	int endX = toCol;
	int endY = toRow;

	removeSelection();
	selectionRect_ = QRect(-1, -1, 0, 0);

	bool leftToRight = true;

	if (toRow < fromRow) {
		leftToRight = false;
		int tmp = toRow;
		toRow = fromRow;
		fromRow = tmp;
	} else if (toRow == fromRow) {
		leftToRight = toCol > fromCol;
	}

	for (int i = fromRow + 1; i < toRow; i++) {
		if ((uint) i < editor_->document()->linesCount()) {
			QString text = editor_->document()->textAt(i);
			for (int j = 0; j < text.size(); j++) {
				editor_->document()->setSelected(i, j, true);
			}
			editor_->document()->setEndOfLineSelected(i, true);
		}
	}
	if (leftToRight) {
		if ((uint) fromRow < editor_->document()->linesCount()) {
			QString text = editor_->document()->textAt(fromRow);
			QList<bool> sm = editor_->document()->selectionMaskAt(fromRow);
			int indent = hardIndents ? editor_->document()->indentAt(fromRow) * 2 : 0;
			int start = fromCol - indent;
			int end = (fromRow == toRow) ? toCol - indent : text.size();
			start = qMax(0, start);
			start = qMin(start, sm.size());
			end = qMax(0, end);
			end = qMin(end, sm.size());
			for (int j = start; j < end; j++) {
				editor_->document()->setSelected(fromRow, j, true);
			}
			editor_->document()->setEndOfLineSelected(fromRow, fromRow != toRow);
		}
		if ((uint) toRow < editor_->document()->linesCount()) {
			QList<bool> sm = editor_->document()->selectionMaskAt(toRow);
			int indent = hardIndents ? editor_->document()->indentAt(toRow) * 2 : 0;
			int start = (fromRow == toRow) ? toCol - indent : 0;
			int end = toCol - indent;
			start = qMax(0, start);
			start = qMin(start, sm.size());
			end = qMax(0, end);
			end = qMin(end, sm.size());
			for (int j = start; j < end; j++) {
				editor_->document()->setSelected(toRow, j, true);
			}
		}
	} else {
		if ((uint) fromRow < editor_->document()->linesCount()) {
			const QString text = editor_->document()->textAt(fromRow);
			QList<bool> sm = editor_->document()->selectionMaskAt(fromRow);
			int indent = hardIndents ? editor_->document()->indentAt(fromRow) * 2 : 0;
			int start = toCol - indent;
			int end = (fromRow == toRow) ? fromCol - indent : text.size();
			start = qMax(0, start);
			start = qMin(start, sm.size());
			end = qMax(0, end);
			end = qMin(end, sm.size());
			for (int j = start; j < end; j++) {
				editor_->document()->setSelected(fromRow, j, true);
			}
			editor_->document()->setEndOfLineSelected(fromRow, fromRow != toRow);
		}
		if ((uint) toRow < editor_->document()->linesCount()) {
			QList<bool> sm = editor_->document()->selectionMaskAt(toRow);
			int indent = hardIndents ? editor_->document()->indentAt(toRow) * 2 : 0;
			int start = (fromRow == toRow) ? fromCol - indent : 0;
			int end = fromCol - indent;
			start = qMax(0, start);
			start = qMin(start, sm.size());
			end = qMax(0, end);
			end = qMin(end, sm.size());
			for (int j = start; j < end; j++) {
				editor_->document()->setSelected(toRow, j, true);
			}
		}
	}
	row_ = endY;
	column_ = endX;
	if (!isFreeCursorMovement()) {
		uint indent = hardIndents ? qMax(0, int(2 * editor_->document()->indentAt(row_))) : 0;
		uint maxColumn = indent + editor_->document()->textAt(row_).length();
		column_ = qMin(column_, maxColumn);
	}
	if (viewMode_ != VM_Hidden) {
		visibleFlag_ = true;
	}
	updateRequest();
	emitPositionChanged();
}

void TextCursor::selectRangeBlock(int fromRow, int fromCol, int toRow, int toCol)
{
	visibleFlag_ = false;
	updateRequest();

	removeSelection();
	selectionRect_ = QRect(qMin(fromCol, toCol),
			qMin(fromRow, toRow),
			abs(fromCol - toCol) + 1,
			abs(fromRow - toRow) + 1
		);

	row_ = toRow;
	column_ = toCol;

	if (viewMode_ != VM_Hidden) {
		visibleFlag_ = true;
	}
	updateRequest();
	emitPositionChanged();
}

static uint countLeadingSpacesInString(const QString &s)
{
	uint result = 0u;
	for (int i = 0; i < s.length(); i++) {
		if (s[i].isSpace()) {
			result ++;
		} else {
			break;
		}
	}
	return result;
}


void TextCursor::movePosition(QTextCursor::MoveOperation op, MoveMode m, int n)
{
	visibleFlag_ = false;
	bool hardIndents = editor_->analizer() &&
		Shared::AnalizerInterface::HardIndents == editor_->analizer()->plugin()->indentsBehaviour();
	updateRequest();
	bool wasRectSelection = hasRectSelection();
	if (m == MM_Move) {
		removeSelection();
		selectionRect_ = QRect(-1, -1, 0, 0);
	} else if (m == MM_Select) {
		selectionRect_ = QRect(-1, -1, 0, 0);
	} else if (m == MM_RectSelect) {
		removeSelection();
		if (!selectionRect_.isValid()) {
			selectionRect_ = QRect(column_, row_, 0, 0);
		}
	}
	for (int i = 0; i < n; i++) {
		if (op == QTextCursor::NextCell) {
			keptColumn_ = -1;
			if (m == MM_Move) {
				if (isFreeCursorMovement()) {
					column_ ++;
				} else {
					uint indent = hardIndents ? qMax(0, 2 * int(editor_->document()->indentAt(row_))) : 0;
					uint textPos = qMax(0, int(column_) - int(indent));
					if (textPos < (uint) editor_->document()->textAt(row_).length()
						||
						column_ < indent
					) {
						column_ ++;
					} else {
						row_ ++;
						column_ = 0;
					}
				}
			} else if (m == MM_Select) {
				int indent = hardIndents ? 2 * editor_->document()->indentAt(row_) : 0;
				if (row_ < editor_->document()->linesCount()) {
					int textPos = column_ - indent;
					if (textPos < 0) {
						column_ = indent;
						textPos = 0;
					}
					if (textPos < editor_->document()->textAt(row_).length()) {
						editor_->document()->setSelected(row_, textPos, !editor_->document()->selectionMaskAt(row_)[textPos]);
						column_++;
					} else {
						if (row_ < editor_->document()->linesCount()) {
							editor_->document()->setEndOfLineSelected(row_, !editor_->document()->lineEndSelectedAt(row_));
						}
						row_++;
						column_ = hardIndents ? editor_->document()->indentAt(row_) * 2 : 0;
					}
				}
			} else if (m == MM_RectSelect) {
				column_++;
				if (column_ > (uint) selectionRect_.right()) {
					selectionRect_.setRight(column_);
				} else {
					selectionRect_.setLeft(column_);
				}
				selectionRect_.setHeight(qMax(1, selectionRect_.height()));
			}

		} else if (op == QTextCursor::PreviousCell) {
			keptColumn_ = -1;
			if (m == MM_Move) {
				if (column_ == 0 && row_ == 0) {}
				else if (column_ == 0 && row_ > 0) {
					row_ --;
					int indent = hardIndents ? editor_->document()->indentAt(row_) * 2 : 0;
					int textPos = 0;
					if (row_ < editor_->document()->linesCount()) {
						textPos = editor_->document()->textAt(row_).length();
					}
					column_ = indent + textPos;

				} else {
					column_--;
				}
			} else if (m == MM_Select) {
				uint indent = hardIndents ? editor_->document()->indentAt(row_) * 2 : 0;
				if (column_ <= indent && row_ == 0) {}
				else if (row_ >= editor_->document()->linesCount()) {
					row_ = editor_->document()->linesCount() - 1;
					column_ = editor_->document()->textAt(row_).length();
					if (hardIndents) {
						column_ += editor_->document()->indentAt(row_) * 2;
					}
					editor_->document()->setEndOfLineSelected(row_, !editor_->document()->lineEndSelectedAt(row_));
				} else if (row_ < editor_->document()->linesCount()
					&& column_ > indent + editor_->document()->textAt(row_).length()) {
					column_ = editor_->document()->textAt(row_).length() - 1;
					if (hardIndents) {
						column_ += editor_->document()->indentAt(row_) * 2;
					}
					editor_->document()->setSelected(row_, editor_->document()->selectionMaskAt(row_).size() - 1,
						!editor_->document()->selectionMaskAt(row_).last());
				} else if (column_ <= indent && row_ > 0) {
					editor_->document()->setEndOfLineSelected(row_ - 1, !editor_->document()->lineEndSelectedAt(row_ - 1));
					row_ --;
					column_ = editor_->document()->textAt(row_).length();
					if (hardIndents) {
						column_ += editor_->document()->indentAt(row_) * 2;
					}
				} else {
					int textPos = column_;
					if (hardIndents) {
						textPos -= editor_->document()->indentAt(row_) * 2;
					}
					if (textPos > 0) {
						editor_->document()->setSelected(row_, textPos - 1, !editor_->document()->selectionMaskAt(row_)[textPos - 1]);
						column_ --;
					}
				}
			} else if (m == MM_RectSelect) {
				if (column_ > 0) {
					column_--;
					if (column_ < (uint) selectionRect_.left()) {
						selectionRect_.setLeft(column_);
					} else {
						selectionRect_.setRight(column_);
					}
					selectionRect_.setHeight(qMax(1, selectionRect_.height()));
				}
			}
		} else if (op == QTextCursor::NextRow) {
			if (m == MM_Move) {
				row_++;
				if (!isFreeCursorMovement()) {
					if (keptColumn_ == -1) {
						keptColumn_ = column_;
					}
					uint indent = hardIndents ? qMax(0, 2 * int(editor_->document()->indentAt(row_))) : 0;
					uint textPos = qMax(0, keptColumn_ - int(indent));
					if (textPos < (uint) editor_->document()->textAt(row_).length()
						||
						(uint) keptColumn_ < indent
					) {
						// Keep and show column position
						column_ = keptColumn_;
					} else {
						// Just keep, but no effect kept column position
						column_ = editor_->document()->textAt(row_).length() + indent;
					}
				} else if (keptColumn_ != -1) {
					column_ = keptColumn_;
				}
			} else if (m == MM_Select) {
				if (row_ < editor_->document()->linesCount()) {
					editor_->document()->setEndOfLineSelected(row_, !editor_->document()->lineEndSelectedAt(row_));
					int textPos = column_;
					if (hardIndents) {
						textPos -= editor_->document()->indentAt(row_) * 2;
					}
					if (textPos < 0) {
						textPos = 0;
					}
					for (int i = textPos; i < editor_->document()->textAt(row_).length(); i++) {
						editor_->document()->setSelected(row_, i, !editor_->document()->selectionMaskAt(row_)[i]);
					}

					row_++;
					textPos = column_;
					if (hardIndents) {
						textPos -= editor_->document()->indentAt(row_) * 2;
					}
					if (textPos < 0) {
						textPos = 0;
					}
					if (row_ >= editor_->document()->linesCount()) {
						textPos = 0;
					} else if (textPos > editor_->document()->textAt(row_).length()) {
						textPos = editor_->document()->textAt(row_).length();
					}
					for (int i = 0; i < textPos; i++) {
						editor_->document()->setSelected(row_, i, !editor_->document()->selectionMaskAt(row_)[i]);
					}
					column_ = textPos + (hardIndents ? editor_->document()->indentAt(row_) * 2 : 0);
				}
			} else if (m == MM_RectSelect) {
				row_++;
				if (row_ > (uint) selectionRect_.bottom()) {
					selectionRect_.setBottom(row_);
				} else {
					selectionRect_.setTop(row_);
				}
				selectionRect_.setWidth(qMax(1, selectionRect_.width()));
			}
		} else if (op == QTextCursor::PreviousRow) {
			if (row_ == 0 && MM_Move == m) {}
			else {
				if (m == MM_Move) {
					row_--;
					if (!isFreeCursorMovement()) {
						if (keptColumn_ == -1) {
							keptColumn_ = column_;
						}
						uint indent = hardIndents ? qMax(0, 2 * int(editor_->document()->indentAt(row_))) : 0;
						uint textPos = qMax(0, keptColumn_ - int(indent));
						if (textPos < (uint) editor_->document()->textAt(row_).length()
							||
							(uint) keptColumn_ < indent
						) {
							// Keep and show column position
							column_ = keptColumn_;
						} else {
							// Just keep, but no effect kept column position
							column_ = editor_->document()->textAt(row_).length() + indent;
						}
					} else if (keptColumn_ != -1) {
						column_ = keptColumn_;
					}
				} else if (m == MM_Select) {
					if (row_ == 0) {
						int textPos = column_;
						if (hardIndents) {
							textPos -= editor_->document()->indentAt(row_) * 2;
						}
						for (int i = 0; i < qMin(textPos, editor_->document()->textAt(row_).length()); i++) {
							editor_->document()->setSelected(row_, i, !editor_->document()->selectionMaskAt(row_)[i]);
						}
						column_ = hardIndents ? editor_->document()->indentAt(0) * 2 : 0;
					} else {
						if (-1 == keptColumn_) {
							keptColumn_ = column_;
						}
						int textPos = column_;
						if (hardIndents) {
							textPos -= editor_->document()->indentAt(row_) * 2;
						}
						if (row_ < editor_->document()->linesCount()) {
							for (int i = 0; i < qMin(textPos, editor_->document()->textAt(row_).length()); i++) {
								editor_->document()->setSelected(row_, i, !editor_->document()->selectionMaskAt(row_)[i]);
							}
							row_ --;
						} else {
							row_ = editor_->document()->linesCount() - 1;
						}
						uint indent = hardIndents ? qMax(0, 2 * int(editor_->document()->indentAt(row_))) : 0;
						textPos = qMax(0, keptColumn_ - int(indent));
						if (textPos > editor_->document()->textAt(row_).length()) {
							textPos = editor_->document()->textAt(row_).length();
						}
						for (int i = textPos; i < editor_->document()->textAt(row_).length(); i++) {
							editor_->document()->setSelected(row_, i, !editor_->document()->selectionMaskAt(row_)[i]);
						}
						editor_->document()->setEndOfLineSelected(row_, !editor_->document()->lineEndSelectedAt(row_));
						column_ = textPos;
						column_ += indent;

					}
				} else if (m == MM_RectSelect) {
					row_--;
					if (row_ < (uint) selectionRect_.top()) {
						selectionRect_.setTop(row_);
					} else {
						selectionRect_.setBottom(row_);
					}
					selectionRect_.setWidth(qMax(1, selectionRect_.width()));
				}
			}
		} else if (op == QTextCursor::StartOfBlock) {
			keptColumn_ = -1;
			if (m == MM_Move) {
				if (row_ >= editor_->document()->linesCount()) {
					column_ = 0;
				} else {
					uint indent = 0;
					QString textLine = editor_->document()->textAt(row_);
					if (hardIndents) {
						indent = editor_->document()->indentAt(row_) * 2;
					}
					uint nonSpacePos = 0;
					for (; nonSpacePos < (uint) textLine.length(); nonSpacePos++) {
						if (!textLine[nonSpacePos].isSpace()) {
							break;
						}
					}
					if (nonSpacePos == (uint) textLine.length()) {
						nonSpacePos = 0;
					}
					uint beforeTextPos = indent + nonSpacePos;
					if (column_ != beforeTextPos) {
						column_ = beforeTextPos;
					} else {
						column_ = indent;
					}
				}
			} else if (m == MM_Select) {
				if (row_ < editor_->document()->linesCount()) {
					int textPos = column_;
					if (hardIndents) {
						textPos -= editor_->document()->indentAt(row_);
					}
					textPos = qMax(0, textPos);
					textPos = qMin(editor_->document()->textAt(row_).length(), textPos);
					int firstNonSpace = countLeadingSpacesInString(editor_->document()->textAt(row_));
					int firstSelected = firstNonSpace == textPos ? 0 : firstNonSpace;
					int a = qMin(firstSelected, textPos);
					int b = qMax(firstSelected, textPos);
					for (int i = a; i < b; i++) {
						editor_->document()->setSelected(row_, i, !editor_->document()->selectionMaskAt(row_)[i]);
					}
					column_ = firstSelected;
					column_ += hardIndents ? editor_->document()->indentAt(row_) * 2 : 0;
				} else {
					column_ = 0;
				}
			}
		} else if (op == QTextCursor::EndOfBlock) {
			keptColumn_ = -1;
			if (m == MM_Move) {
				if (row_ >= editor_->document()->linesCount()) {
					column_ = 0;
				} else {
					const QString textLine = editor_->document()->textAt(row_);
					int indent = hardIndents ? editor_->document()->indentAt(row_) * 2 : 0;
					int nonSpacePos = textLine.length();
					for (int i = textLine.length() - 1; i >= 0; i--) {
						if (textLine[i].isSpace()) {
							nonSpacePos--;
						} else {
							break;
						}
					}
					column_ = indent + nonSpacePos;
					if (textLine.trimmed().isEmpty() && !hardIndents) {
						column_ = textLine.length();
					}
//                    i_column = m_document->indentAt(i_row) * 2 +
//                            m_document->textAt(i_row).length();
				}
			} else if (m == MM_Select) {
				if (row_ < editor_->document()->linesCount()) {
					int textPos = column_;
					if (hardIndents) {
						textPos -= editor_->document()->indentAt(row_) * 2;
					}
					textPos = qMax(0, textPos);
					for (int i = textPos; i < editor_->document()->textAt(row_).length(); i++) {
						editor_->document()->setSelected(row_, i, !editor_->document()->selectionMaskAt(row_)[i]);
					}
					column_ = editor_->document()->textAt(row_).length();
					if (hardIndents) {
						column_ += editor_->document()->indentAt(row_) * 2;
					}
				}
			}
		} else if (op == QTextCursor::Start) {
			keptColumn_ = -1;
			if (m == MM_Move) {
				row_ = 0;
				column_ = 0;
			} else if (m == MM_Select) {
				if (row_ < editor_->document()->linesCount()) {
					int textPos = column_;
					if (hardIndents) {
						textPos -= editor_->document()->indentAt(row_) * 2;
					}
					textPos = qMax(0, textPos);
					textPos = qMin(editor_->document()->textAt(row_).length(), textPos);
					for (int i = 0; i < textPos; i++) {
						editor_->document()->setSelected(row_, i, !editor_->document()->selectionMaskAt(row_)[i]);
					}
					row_--;
				} else {
					row_ = editor_->document()->linesCount() - 1;
				}
				for (int k = row_; k >= 0; k--) {
					for (int i = 0; i < editor_->document()->selectionMaskAt(k).size(); i++) {
						editor_->document()->setSelected(k, i, !editor_->document()->selectionMaskAt(k)[i]);
					}
					editor_->document()->setEndOfLineSelected(k, !editor_->document()->lineEndSelectedAt(k));
				}
				column_ = 0;
				row_ = 0;
			}
		} else if (op == QTextCursor::End) {
			keptColumn_ = -1;
			if (m == MM_Move) {
				row_ = editor_->document()->linesCount() - 1;
				column_ = editor_->document()->textAt(editor_->document()->linesCount() - 1).length();
				if (hardIndents) {
					column_ += editor_->document()->indentAt(row_) * 2;
				}
			} else if (m == MM_Select) {
				if (row_ >= editor_->document()->linesCount()) {
					row_ = editor_->document()->linesCount();
					if (row_ > 0) {
						row_ -= 1;
						column_ = editor_->document()->textAt(editor_->document()->linesCount() - 1).length();
						if (hardIndents) {
							column_ += editor_->document()->indentAt(row_) * 2;
						}
					} else {
						row_ = 0;
						column_ = 0;
					}
				} else {
					int textPos = column_;
					if (hardIndents) {
						textPos -= editor_->document()->indentAt(row_);
					}
					textPos = qMax(0, textPos);
					for (int i = textPos; i < editor_->document()->textAt(row_).length(); i++) {
						editor_->document()->setSelected(row_, i, !editor_->document()->selectionMaskAt(row_)[i]);
					}
					editor_->document()->setEndOfLineSelected(row_, !editor_->document()->lineEndSelectedAt(row_));
					row_ ++;
					for (; row_ < editor_->document()->linesCount(); row_++) {
						for (int i = 0; i < editor_->document()->selectionMaskAt(row_).size(); i++) {
							editor_->document()->setSelected(row_, i, !editor_->document()->selectionMaskAt(row_)[i]);
						}
						if (row_ < editor_->document()->linesCount() - 1) {
							editor_->document()->setEndOfLineSelected(row_, !editor_->document()->lineEndSelectedAt(row_));
						}
					}
					row_ = editor_->document()->linesCount() - 1;
					column_ = editor_->document()->textAt(editor_->document()->linesCount() - 1).length();
					if (hardIndents) {
						column_ += editor_->document()->indentAt(row_) * 2;
					}
				}
			}
		} // end End
		else if (op == QTextCursor::PreviousWord) {
			keptColumn_ = -1;
			uint toRow, toColumn;
			findLexemBound(toRow, toColumn, -1);
			if (m == MM_Move) {
				row_ = toRow;
				column_ = toColumn;
			} else if (m == MM_Select && (row_ > 0 || column_ > 0)) {
				if (row_ >= editor_->document()->linesCount()) {
					row_ = editor_->document()->linesCount() - 1;
					column_ = 2 * editor_->document()->indentAt(row_) +
						editor_->document()->at(row_).text.length() + 1;
				}
				column_ -= 1;
				// Selection starts from left of current column

				forever {
					uint stopAt = row_ == toRow ? toColumn : 2 * editor_->document()->indentAt(row_);
					forever {
						int linePos = column_ - 2 * editor_->document()->indentAt(row_);
						if (linePos >= 0 && linePos < editor_->document()->at(row_).text.length()) {
							editor_->document()->at(row_).selected[linePos] = !editor_->document()->at(row_).selected[linePos];
						}
						if (column_ > stopAt) {
							column_ -= 1;
						} else {
							break;
						}
					}
					if (row_ == toRow) {
						break;
					}
					row_ -= 1;
					editor_->document()->at(row_).lineEndSelected = !editor_->document()->at(row_).lineEndSelected;
					column_ = 2 * editor_->document()->indentAt(row_) +
						editor_->document()->at(row_).text.length();
				}
			}
		} // end PreviousWord
		else if (op == QTextCursor::NextWord) {
			keptColumn_ = -1;
			uint toRow, toColumn;
			findLexemBound(toRow, toColumn, +1);
			if (m == MM_Move) {
				row_ = toRow;
				column_ = toColumn;
			} else if (m == MM_Select) {
				forever {
					forever {
						if (row_ == toRow && column_ == toColumn) {
							break;
						}
						int linePos = column_ - 2 * editor_->document()->indentAt(row_);
						if (linePos >= 0 && linePos < editor_->document()->at(row_).text.length()) {
							editor_->document()->at(row_).selected[linePos] = !editor_->document()->at(row_).selected[linePos];
						}
						if (linePos < editor_->document()->at(row_).text.length()) {
							column_ += 1;
						} else {
							break;
						}
					}
					if (row_ == toRow) {
						break;
					}
					editor_->document()->at(row_).lineEndSelected = !editor_->document()->at(row_).lineEndSelected;
					row_ += 1;
					column_ = 0;
				}
			}
		} // end NextWord
	}
	visibleFlag_ = true;
	emit updateRequest();
	if (m == MM_Select || m == MM_RectSelect || wasRectSelection) {
		emit updateRequest(-1, -1);
	}
	emitPositionChanged();
}

void TextCursor::findLexemBound(uint &row, uint &column, const qint8 dir) const
{
	row = row_;
	column = column_;

	if (dir > 0 && row_ >= editor_->document()->linesCount()) {
		return;
	}
	if (dir < 0 && row_ == 0 && row_ < editor_->document()->linesCount() && column_ <= 2 * editor_->document()->indentAt(0)) {
		return;
	}

	int linePos = column - 2 * editor_->document()->indentAt(row);

	if (dir < 0) {
		// search backward
		int realTextRow = qMin(editor_->document()->linesCount() - 1, row);
		int realTextEndPos = editor_->document()->at(realTextRow).text.length();
		while (realTextEndPos > 0) {
			// find last non space position in text
			if (editor_->document()->at(realTextRow).text.at(realTextEndPos - 1).isSpace()) {
				realTextEndPos -= 1;
			} else {
				break;
			}
		}
		if (linePos > realTextEndPos || row > (uint) realTextRow) {
			// try to act like 'End' key
			row = realTextRow;
			column = realTextEndPos + 2 * editor_->document()->indentAt(row);
			return;
		}

		forever {
			linePos = column - 2 * editor_->document()->indentAt(row);
			linePos = qMax(0, qMin(linePos, editor_->document()->at(row).text.length()));
			// Normalization of linePos

			if (linePos == 0 && row == 0) {
				// found start of document
				return;
			} else if (linePos == 0 && row > 0) {
				// jump to end of previous line
				row -= 1;
				linePos = editor_->document()->at(row).text.length();
				while (linePos > 0 && editor_->document()->at(row).text.at(linePos - 1).isSpace()) {
					linePos -= 1;
				}
				column = linePos + 2 * editor_->document()->indentAt(row);
				return;
			} else {
				// jump to start of lexem (or word in text mode)
				if (editor_->analizerInstance_ && editor_->document()->at(row).highlight.at(linePos - 1) != Shared::LxTypeComment) {
					// jump to start of lexem in this case
					LexemType curLexemType, prevPosLexemType;
					curLexemType = editor_->document()->at(row).highlight.at(linePos - 1);
					while (linePos > 0 && curLexemType == LxTypeEmpty) {
						// ignore whitespaces
						linePos -= 1;
						curLexemType = editor_->document()->at(row).highlight.at(qMax(0, linePos - 1));
					}
					while (linePos > 0) {
						// linePos==0 is : line bound is a delimeter too
						prevPosLexemType = editor_->document()->at(row).highlight.at(linePos - 1);
						if (prevPosLexemType != curLexemType)
							// found a lexem of previous type
						{
							break;
						}
						linePos -= 1;
					}
				} else {
					// jump to last non-space position at left
					if (linePos > 0
						&&
						linePos < editor_->document()->at(row).text.length()
						&& editor_->document()->at(row).text.at(linePos - 1).isSpace()
					) {
						// if already at begin of word, then go left
						// to find previous word
						linePos -= 1;

					}
					while (linePos >= 0) {
						// skip trailing spaces first
						if (linePos >= editor_->document()->at(row).text.length() || editor_->document()->at(row).text.at(linePos).isSpace()) {
							linePos -= 1;
						} else {
							break;
						}
					}
					while (linePos > 0) {
						// do actual search
						bool prevIsSpace = editor_->document()->at(row).text.at(linePos - 1).isSpace();
						if (prevIsSpace)
							// found start of word
						{
							break;
						}
						linePos -= 1;
					}
				}
				column = linePos + 2 * editor_->document()->indentAt(row);
				return;
			}
		}
	} else if (dir > 0) {
		// search forward
		forever {
			linePos = column - 2 * editor_->document()->indentAt(row);
			linePos = qMax(linePos, 0);
			// skip indent pos

			// find text line length excluding trailing spaces
			const QString &lineText = editor_->document()->at(row).text;
			int length = lineText.length();
			int trailingSpaces = 0;
			for (int k = length - 1; k >= 0; k--) {
				if (lineText[k].isSpace()) {
					trailingSpaces += 1;
				} else {
					break;
				}
			}
			length -= trailingSpaces;
			if (row == editor_->document()->linesCount() - 1 && linePos >= length) {
				// found end of document
				return;
			} else if (linePos >= length && row + 1 < editor_->document()->linesCount()) {
				// jump to start of next line
				row += 1;
				linePos = 0;
				while (linePos < lineText.length() && lineText.at(linePos).isSpace()) {
					linePos += 1;
				}
				column = linePos + 2 * editor_->document()->indentAt(row);
				return;
			} else {
				if (editor_->analizerInstance_ && editor_->document()->at(row).highlight.at(linePos) != Shared::LxTypeComment) {
					// jump to end of lexem in this case
					LexemType curLexemType, nextPosLexemType;
					curLexemType = editor_->document()->at(row).highlight.at(linePos);
					while (linePos < length && curLexemType == LxTypeEmpty) {
						// ignore whitespaces
						curLexemType = editor_->document()->at(row).highlight.at(linePos);
						linePos += 1;
					}
					while (linePos < length) {
						linePos += 1;
						if (linePos == length)
							// found end of line (except trailing spaces)
						{
							break;
						}
						nextPosLexemType = editor_->document()->at(row).highlight.at(linePos);
						if (nextPosLexemType != curLexemType && nextPosLexemType != LxTypeEmpty)
							// found a lexem of next type;
						{
							break;
						}
					}
				} else {
					// jump to start of next word, i.e.
					// first non-space symbol after any spaces
					while (linePos < length
						&& !editor_->document()->at(row).text.at(linePos).isSpace()) {
						// at first - go to end of current word
						linePos += 1;
					}
					while (linePos < length
						&& editor_->document()->at(row).text.at(linePos).isSpace()
					) {
						// skip spaces
						linePos += 1;
					}
				}
				column = linePos + 2 * editor_->document()->indentAt(row);
				return;
			}
		}
	}
}

void TextCursor::insertBlock(const QStringList &block)
{
	if (!enabledFlag_) {
		return;
	}
	if (modifiesProtectedLiines()) {
		return;
	}
	if (hasSelection()) {
		removeSelectedText();
	}
	if (hasRectSelection()) {
		removeSelectedBlock();
	}

	editor_->document()->undoStack()->push(new InsertBlockCommand(editor_->document(), this, editor_->analizerInstance_, row_, column_, block));

	emit updateRequest(-1, -1);

	emitPositionChanged();
}

void TextCursor::removeRectSelection()
{
	selectionRect_ = QRect(-1, -1, 0, 0);
	emit updateRequest();
}

bool TextCursor::modifiesProtectedLiines() const
{
	if (teacherModeFlag_) {
		return false;
	}
	if (hasRectSelection()) {
		uint lineStart = selectionRect_.top();
		uint lineEnd = selectionRect_.bottom();

		lineStart = qMin(
				qMax(0u, lineStart),
				editor_->document()->linesCount() - 1
			);

		lineEnd = qMin(
				qMax(1u, lineEnd),
				editor_->document()->linesCount()
			);

		for (uint i = lineStart; i < lineEnd; i++) {
			if (editor_->document()->isProtected(i)) {
				return true;
			}
		}
	}
	const uint linesCount = editor_->document()->linesCount();
	if (linesCount > 0u) {
		TextLine *lastLine = & editor_->document()->at(linesCount - 1);
		if (lastLine->protecteed && row_ >= linesCount) {
			return true;
		}
	}
	for (uint i = 0; i < editor_->document()->linesCount(); i++) {
		if (editor_->document()->isProtected(i)) {
			if (editor_->document()->lineEndSelectedAt(i)) {
				return true;
			}
			if (row_ == i) {
				return true;
			}
			if (editor_->document()->selectionMaskAt(i).contains(true)) {
				return true;
			}
		}
	}
	return false;
}

void TextCursor::changeSelectionToExcludeProtectedLines()
{
	qDebug() << "Change selection";
	// 1. Move selection left to the first non-protected position
	uint firstSelectedLine = 0;
	for (uint lineNo = 0; lineNo < editor_->document()->linesCount(); ++lineNo) {
		TextLine &line = editor_->document()->at(lineNo);
		QList<bool> &selectionMask = line.selected;
		if (selectionMask.contains(true) || line.lineEndSelected) {
			firstSelectedLine = lineNo;
			break;
		}
	}
	for (uint lineNo = firstSelectedLine; lineNo < editor_->document()->linesCount(); ++lineNo) {
		TextLine &line = editor_->document()->at(lineNo);
		QList<bool> &selectionMask = line.selected;
		if (line.protecteed) {
			for (int column = 0; column < selectionMask.size(); ++column) {
				selectionMask[column] = false;
			}
			line.lineEndSelected = false;
		} else {
			firstSelectedLine = lineNo;
			break;
		}
	}
	// 2. Find new selection right bound
	int selectionLastRow = -1;
	int selectionLastColumn = -1;
	for (uint lineNo = firstSelectedLine; lineNo < editor_->document()->linesCount(); ++lineNo) {
		TextLine &line = editor_->document()->at(lineNo);
		QList<bool> &selectionMask = line.selected;
		if (!line.protecteed && !line.hidden && (selectionMask.contains(true) || line.lineEndSelected)) {
			selectionLastRow = lineNo;
			selectionLastColumn = qMax(0, selectionMask.lastIndexOf(true));
		} else {
			break;
		}
	}
	// 3. Set tail not selected
	if (selectionLastColumn != -1 && selectionLastRow != -1) {
		editor_->document()->at(selectionLastRow).lineEndSelected = false;
		for (uint lineNo = selectionLastRow + 1; lineNo < editor_->document()->linesCount(); ++lineNo) {
			TextLine &line = editor_->document()->at(lineNo);
			QList<bool> &selectionMask = line.selected;
			for (int column = 0; column < selectionMask.size(); ++column) {
				selectionMask[column] = false;
			}
			line.lineEndSelected = false;
		}
		row_ = selectionLastRow;
		column_ = selectionLastColumn + editor_->document()->indentAt(row_);
	}
}

void TextCursor::insertText(const QString &text)
{
	keptColumn_ = -1;
	if (!enabledFlag_) {
		emit signalizeNotEditable();
		return;
	}

	if (modifiesProtectedLiines()) {
		return;
	}

	bool sel = hasSelection();
	bool bsel = hasRectSelection();
	bool hardIndents = editor_->analizer() &&
		Shared::AnalizerInterface::HardIndents == editor_->analizer()->plugin()->indentsBehaviour();

	if (sel) {
		editor_->document()->undoStack()->beginMacro("replaceSelectedText");
		removeSelectedText();
	}
	if (bsel) {
		editor_->document()->undoStack()->beginMacro("replaceSelectedBlock");
		removeSelectedBlock();
	}



	int fromLineUpdate = row_;

	const int indent = hardIndents ? editor_->document()->indentAt(row_) : 0;

	// move cursor from protected part
	column_ = qMax(column_, indent * 2u);

	if (editor_->document()->editor_->mySettings()->value(SettingsPage::KeyForcePressTextToLeft, SettingsPage::DefaultForcePressTextToLeft).toBool()) {
		column_ = justifyLeft(text);
	}

	int textPos = column_ - indent * 2;
	editor_->document()->undoStack()->push(new InsertCommand(editor_->document(), this, editor_->analizerInstance_, row_, textPos, text));

	int toLineUpdate = -1;

	emit updateRequest(fromLineUpdate, toLineUpdate);

	if (sel || bsel) {
		editor_->document()->undoStack()->endMacro();
	}

	emitPositionChanged();
}

void TextCursor::insertImport(const QString &importableName)
{
	if (!enabledFlag_) {
		emit signalizeNotEditable();
		return;
	}
	if (modifiesProtectedLiines()) {
		return;
	}

	if (editor_->analizerInstance_) {
		editor_->document()->undoStack()->push(new InsertImportCommand(
				editor_->document(),
				this,
				editor_->analizerInstance_,
				importableName
			)
		);
	}

	emit updateRequest(0, row_ + 1);
}

int TextCursor::justifyLeft(const QString &text) const
{

	if (!editor_->analizerInstance_ || text.trimmed().isEmpty()) {
		return column_;
	}
	bool hardIndents = editor_->analizer() &&
		Shared::AnalizerInterface::HardIndents == editor_->analizer()->plugin()->indentsBehaviour();

	// Emulate text change and get line property

	const int indent = hardIndents ? editor_->document()->indentAt(row_) * 2 : 0;
	int col = column_;
	int textPos = col - indent;
	if (textPos <= 0) {
		return column_; // Nothing to do
	}
	QString s;
	if (editor_->document()->linesCount() > row_) {
		s = editor_->document()->textAt(row_);
	}
	s.insert(textPos, text);
	Shared::Analizer::LineProp lp = editor_->analizerInstance_->lineProp(row_, s);
	if (lp[textPos] == LxTypeComment || lp[textPos] == LxConstLiteral) {
		// Do not justify comments and literal constants
		return column_;
	} else {
		if (textPos > 0 && s[textPos - 1].isSpace()) {
			// If typing new word -- press text to left
			while (textPos > 0 && s[textPos - 1].isSpace()) {
				textPos --;
			}
			if (textPos > 0 && !text[0].isPunct()) {
				textPos++;    // Do not concatenate to previous word if exists
			}
		}
	}
	return indent + textPos;
}

void TextCursor::removePreviousChar()
{
	// TODO Undo-redo stack!
	if (!enabledFlag_) {
		return;
	}
	if (modifiesProtectedLiines()) {
		return;
	}
	if (hasSelection()) {
		removeSelectedText();
		emitPositionChanged();
		return;
	}
	if (hasRectSelection()) {
		removeSelectedBlock();
		emitPositionChanged();
		return;
	}
	removeSelection();
	visibleFlag_ = false;
	emit updateRequest();
	int fromLineUpdate = -1;
	int toLineUpdate = -1;

	bool hardIndents = editor_->analizer() &&
		Shared::AnalizerInterface::HardIndents == editor_->analizer()->plugin()->indentsBehaviour();

	const int indent = hardIndents ? editor_->document()->indentAt(row_) : 0;
	int textPos = column_ - indent * 2;
	if (textPos > 0) {
		// remove previous char in current line
		if (row_ < editor_->document()->linesCount() &&
			textPos <= editor_->document()->textAt(row_).length()) {
			editor_->document()->undoStack()->push(new RemoveCommand(editor_->document(),
					this,
					editor_->analizerInstance_,
					row_,
					textPos - 1,
					1,
					false,
					row_,
					column_ - 1
				));
		} else if (row_ < editor_->document()->linesCount() && textPos > editor_->document()->textAt(row_).length()) {
			movePosition(QTextCursor::PreviousCell, TextCursor::MM_Move);
		} else {
			row_ --;
			column_ = hardIndents ? 2 * editor_->document()->indentAt(row_) : 0;
		}
	} else {
		// remove current line and set cursor to end of previous line
		if (row_ > 0) {
			if (row_ < editor_->document()->linesCount()) {

				// Check if previous char position is protected
				if (!teacherModeFlag_ && editor_->document()->isProtected(row_ - 1)) {
					return;
				}

				editor_->document()->undoStack()->push(new RemoveCommand(editor_->document(),
						this,
						editor_->analizerInstance_,
						row_ - 1,
						editor_->document()->textAt(row_ - 1).length(),
						1,
						false,
						row_ - 1,
						editor_->document()->textAt(row_ - 1).length() +
						(hardIndents ? 2 * editor_->document()->indentAt(row_ - 1) : 0)
					));
			} else {
				evaluateCommand(KeyCommand::MoveToPreviousChar);
			}
		}
	}

	visibleFlag_ = true;
	emit updateRequest();
	emit updateRequest(fromLineUpdate, toLineUpdate);

	emitPositionChanged();
}

void TextCursor::removeCurrentLine()
{
	if (!enabledFlag_) {
		return;
	}
	if (modifiesProtectedLiines()) {
		return;
	}
	if (hasSelection()) {
		removeSelectedText();
		emitPositionChanged();
		return;
	}

	if (row_ < editor_->document()->linesCount()) {
		int column = column_;
		movePosition(QTextCursor::EndOfBlock, MM_Move, 1);
		editor_->document()->undoStack()->push(new RemoveCommand(editor_->document(), this, editor_->analizerInstance_, row_, 0, editor_->document()->textAt(row_).length() + 1, true, row_, column_));
		column_ = column;
		emit updateRequest(-1, -1);
		emit updateRequest();
	}

	emitPositionChanged();
}

void TextCursor::removeLineTail()
{
	if (!enabledFlag_) {
		return;
	}
	if (modifiesProtectedLiines()) {
		return;
	}
	if (hasSelection()) {
		removeSelection();
	}

	if (row_ < editor_->document()->linesCount()) {

		int textPos = column_ - editor_->document()->indentAt(row_) * 2;
		if (textPos < editor_->document()->textAt(row_).length()) {
			editor_->document()->undoStack()->push(new RemoveCommand(editor_->document(), this, editor_->analizerInstance_, row_, textPos, editor_->document()->textAt(row_).length() - textPos, true, row_, column_));
			emit updateRequest(-1, -1);
			emit updateRequest();
		}
	}

	emitPositionChanged();
}

void TextCursor::removeCurrentChar()
{
	// TODO Undo-redo stack!
	if (!enabledFlag_) {
		return;
	}
	if (modifiesProtectedLiines() && !hasSelection()) {
		return;
	}
	if (hasSelection()) {
		removeSelectedText();
		emitPositionChanged();
		return;
	}
	if (hasRectSelection()) {
		removeSelectedBlock();
		emitPositionChanged();
		return;
	}
	visibleFlag_ = false;
	emit updateRequest();

	int fromLineUpdate = -1;
	int toLineUpdate = -1;

	const int indent = editor_->document()->indentAt(row_);
	int textPos = column_ - indent * 2;
	if (row_ >= editor_->document()->linesCount()) {
		return;
	}
	if (textPos >= editor_->document()->textAt(row_).length() && row_ >= editor_->document()->linesCount() - 1) {
		return;
	}
	if (textPos < 0) {
		column_ = 2 * editor_->document()->indentAt(row_);
		return;
	}

	// Check if deletes next protected line
	if (textPos >= editor_->document()->textAt(row_).length()) {
		if (row_ + 1 < editor_->document()->linesCount()) {
			if (editor_->document()->isProtected(row_ + 1)) {
				return;
			}
		}
	}

	editor_->document()->undoStack()->push(new RemoveCommand(editor_->document(), this, editor_->analizerInstance_, row_, textPos, 1, true, row_, column_));

	visibleFlag_ = true;
	emit updateRequest();
	emit updateRequest(fromLineUpdate, toLineUpdate);

	emitPositionChanged();
}


void TextCursor::removeSelectedText()
{
	if (!enabledFlag_) {
		return;
	}

	if (!hasSelection()) {
		return;
	}

	if (modifiesProtectedLiines()) {
		changeSelectionToExcludeProtectedLines();
		return;
	}

	bool hardIndents = editor_->analizer() &&
		Shared::AnalizerInterface::HardIndents == editor_->analizer()->plugin()->indentsBehaviour();

	// Find where to place cursor after deletion

	int cursorStartLine = -1;
	int cursorTextPos = -1;

	for (uint i = 0; i < editor_->document()->linesCount(); i++) {

		if (editor_->document()->lineEndSelectedAt(i) && cursorStartLine == -1) {
			cursorStartLine = i;
		}
		QList<bool> sm = editor_->document()->selectionMaskAt(i);
		for (int j = 0; j < sm.size(); j++) {
			if (sm[j] && cursorTextPos == -1) {
				cursorTextPos = j;
			}
			if (sm[j] && cursorStartLine == -1) {
				cursorStartLine = i;
			}
		}
		if (cursorStartLine != -1) {
			if (cursorTextPos == -1) {
				cursorTextPos = editor_->document()->textAt(i).length();
			}
			break;
		}
	}

	// Remove selected text inside lines

	int selectionLineStart = -1;
//	int selectionLineEnd = -1;

	int lineStart = -1;
	int posStart = -1;
	int count = 0;

	for (uint i = 0; i < editor_->document()->linesCount(); i++) {
		int start = -1;
		int end = -1;
		QList<bool> sm = editor_->document()->selectionMaskAt(i);
		for (int j = 0; j < sm.size(); j++) {
			bool v = sm[j];
			if (v) {
				if (selectionLineStart == -1) {
					selectionLineStart = i;
					lineStart = i;
					posStart = j;
				}
//				selectionLineEnd = qMax(selectionLineEnd, i);
				if (start == -1) {
					start = j;
				}
				end = qMax(j, end);
				count ++;
			}
		}
		if (editor_->document()->lineEndSelectedAt(i)) {
			if (selectionLineStart == -1) {
				selectionLineStart = i;
				lineStart = i;
				posStart = editor_->document()->textAt(i).length();
			}
			count ++;
//			selectionLineEnd = qMax(i + 1, selectionLineEnd);
		}
	}


	if (count > 0) {
		editor_->document()->undoStack()->push(new RemoveCommand(editor_->document(),
				this,
				editor_->analizerInstance_,
				lineStart,
				posStart,
				count,
				true,
				cursorStartLine,
				(hardIndents
					? editor_->document()->indentAt(row_) * 2
					: 0) + cursorTextPos
			));
	}

	// Move cursor

	removeSelection();

	emit updateRequest(-1, -1);
	emit updateRequest();

}

void TextCursor::removeSelectedBlock()
{
	if (!enabledFlag_) {
		return;
	}

	if (!hasRectSelection()) {
		return;
	}

	int lineStart = selectionRect_.top();
	int startPos = selectionRect_.left();

	editor_->document()->undoStack()->push(new RemoveBlockCommand(editor_->document(), this, editor_->analizerInstance_, selectionRect_));

	row_ = lineStart;
	column_ = startPos;
	selectionRect_ = QRect(-1, -1, 0, 0);
	emit updateRequest(-1, -1);
	emit updateRequest();

	emitPositionChanged();
}

bool TextCursor::hasSelection() const
{
	for (uint i = 0; i < editor_->document()->linesCount(); i++) {
		if (editor_->document()->lineEndSelectedAt(i)) {
			return true;
		}
		if (editor_->document()->selectionMaskAt(i).contains(true)) {
			return true;
		}
	}
	return false;
}


QString TextCursor::selectedText() const
{
	QString result;
	if (hasSelection()) {
		for (uint i = 0; i < editor_->document()->linesCount(); i++) {
			const QList<bool> sm = editor_->document()->selectionMaskAt(i);
			const QString text = editor_->document()->textAt(i);
//            Q_ASSERT(text.length()==sm.size());
			for (int j = 0; j < qMin(text.length(), sm.size()); j++) {
				if (sm[j]) {
					result += text[j];
				}
			}
			if (editor_->document()->lineEndSelectedAt(i) && i < editor_->document()->linesCount() - 1) {
				result += "\n";
			}
		}
	} else if (hasRectSelection()) {
		uint startLine = selectionRect_.top();
		uint endLine = qMin(editor_->document()->linesCount(), selectionRect_.bottom() + 1u);
		uint startPos = selectionRect_.left();
		uint endPos = selectionRect_.right();
		for (uint i = startLine; i < endLine; i++) {
			uint indent = editor_->document()->indentAt(i);
			uint textStart = qMax(0, int(startPos) - int(indent) * 2);
			uint textEnd = qMax(0, int(endPos) - int(indent) * 2);
			textStart = qMin(textStart, uint(editor_->document()->textAt(i).length()));
			textEnd = qMin(textEnd, uint(editor_->document()->textAt(i).length()));
			result += editor_->document()->textAt(i).mid(textStart, textEnd - textStart);
			if (i < endLine - 1) {
				result += "\n";
			}
		}
	}
	return result;
}

QStringList TextCursor::rectSelectionText() const
{
	QStringList result;
	if (hasRectSelection()) {
		uint startLine = selectionRect_.top();
		uint endLine = qMin(
				editor_->document()->linesCount(),
				selectionRect_.bottom() + 1u);
		int startPos = selectionRect_.left();
		int endPos = selectionRect_.right() + 1;
		for (uint i = startLine; i < endLine; i++) {
			int indent = editor_->document()->indentAt(i);
			int textStart = startPos - indent * 2;
			int textEnd = endPos - indent * 2;
			textStart = qMin(textStart, editor_->document()->textAt(i).length());
			textEnd = qMin(textEnd, editor_->document()->textAt(i).length());
			textStart = qMax(0, textStart);
			textEnd = qMax(0, textEnd);
			QString text = editor_->document()->textAt(i).mid(textStart, textEnd - textStart);
			int w = selectionRect_.width();
			result << text.leftJustified(w, ' ');
		}
	}
	return result;
}

void TextCursor::selectionBounds(int &fromRow, int &fromCol, int &toRow, int &toCol) const
{
	fromRow = fromCol = toRow = toCol = -1;
	if (!hasSelection()) {
		return;
	}
	for (uint i = 0; i < editor_->document()->linesCount(); i++) {
		if (editor_->document()->selectionMaskAt(i).contains(true) || editor_->document()->lineEndSelectedAt(i)) {
			if (fromRow == -1) {
				fromRow = i;
			}
		}
		if (!editor_->document()->lineEndSelectedAt(i)) {
			if (fromRow != -1) {
				toRow = i;
				break;
			}
		}
	}
	const QList<bool> first = editor_->document()->selectionMaskAt(fromRow);
	fromCol = first.indexOf(true);
	if (fromCol == -1) {
		fromCol = first.size();
	}
	fromCol += 2 * editor_->document()->indentAt(fromRow);

	if (toRow != -1) {
		const QList<bool> last = editor_->document()->selectionMaskAt(toRow);
		toCol = last.lastIndexOf(true);
		if (toCol == -1) {
			toCol = 0;
		}
		toCol += 2 * editor_->document()->indentAt(toRow);
	} else {
		toRow = editor_->document()->linesCount();
		toCol = 0;
	}
}

void TextCursor::removeSelection()
{
	editor_->document()->removeSelection();
	emit updateRequest(-1, -1);
	emitPositionChanged();

}

void TextCursor::undo()
{
//    int prevRow = i_row;
//    int prevLines = m_document->linesCount();
	if (enabledFlag_) {
		editor_->document()->undoStack()->undo();
	}
	int row = row_;
	int column = column_;
//    if (prevRow!=i_row || prevLines!=m_document->linesCount()) {
	editor_->document()->forceCompleteRecompilation(QPoint(column, row));
//    }
	row_ = row;
	column_ = column;
	// restore cursor position due it may be changed
	// after compilation request

	emit undoAvailable(enabledFlag_ && editor_->document()->undoStack()->canUndo());
	emit redoAvailable(enabledFlag_ && editor_->document()->undoStack()->canRedo());
}

void TextCursor::redo()
{
//    int prevRow = i_row;
//    int prevLines = m_document->linesCount();
	if (enabledFlag_) {
		editor_->document()->undoStack()->redo();
	}
//    if (prevRow!=i_row || prevLines!=m_document->linesCount()) {
	editor_->document()->forceCompleteRecompilation(QPoint(column(), row()));
//    }
	emit undoAvailable(enabledFlag_ && editor_->document()->undoStack()->canUndo());
	emit redoAvailable(enabledFlag_ && editor_->document()->undoStack()->canRedo());
}

void TextCursor::handleRedoChanged(bool v)
{
	emit redoAvailable(enabledFlag_ && v);
}

void TextCursor::handleUndoChanged(bool v)
{
	emit undoAvailable(enabledFlag_ && v);
}

void TextCursor::setEnabled(bool v)
{
	enabledFlag_ = v;
	emit updateRequest();
	emit undoAvailable(enabledFlag_ && editor_->document()->undoStack()->canUndo());
	emit redoAvailable(enabledFlag_ && editor_->document()->undoStack()->canRedo());
}

} // namespace Editor
