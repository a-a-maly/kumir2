#include "vm.hpp"

namespace VM
{

void KumirVM::do_specialcall(uint16_t alg)
{
	if (stacksMutex_) {
		stacksMutex_->lock();
	}
	int argsCount = valuesStack_.pop().toInt();
	if (stacksMutex_) {
		stacksMutex_->unlock();
	}
	// Special calls
	if (alg == 0x00) {
		// Input
		if (stacksMutex_) {
			stacksMutex_->lock();
		}
		bool fileIO = false;
		int varsCount = argsCount;
		Kumir::FileType fileReference;
		std::deque<Variable> references;
		for (int i = 0; i < varsCount; i++) {
			const Variable &ref = valuesStack_.pop();
			if (ref.baseType() == VT_record
				&& ref.recordClassAsciiName() == std::string("file")) {
				fileIO = true;
				if (!ref.isValid()) {
					error_ = Kumir::Core::fromUtf8("Нет значения у ключа файла");
					return;
				}
				const Record fileReferenceRecord = ref.toRecord();
				fileReference = fromRecordValue(fileReferenceRecord);
			} else {
				references.push_back(ref);
			}
		}
		if (stacksMutex_) {
			stacksMutex_->unlock();
		}
		bool hasInput = false;
		if (consoleInputBuffer_ && !fileIO) {
			fileReference.setType(Kumir::FileType::Console);
		}
		if (input_ && !fileIO && !Kumir::Files::overloadedStdIn() && !consoleInputBuffer_) {
			// input functor works like input operator here
			hasInput = (*input_)(references, &error_);
		} else {
			hasInput = true;
			if (stacksMutex_) {
				stacksMutex_->lock();
			}

			for (int i = 0; i < (int)references.size(); i++) {
				if (references.at(i).baseType() == VT_int) {
					int value = Kumir::IO::readInteger(fileReference, !fileIO);
					references.at(i).setValue(AnyValue(value));
				} else if (references.at(i).baseType() == VT_real) {
					real value = Kumir::IO::readReal(fileReference, !fileIO);
					references.at(i).setValue(AnyValue(value));
				} else if (references.at(i).baseType() == VT_bool) {
					bool value = Kumir::IO::readBool(fileReference, !fileIO);
					references.at(i).setValue(AnyValue(value));
				} else if (references.at(i).baseType() == VT_char &&
					references.at(i).isConstant() &&
					references.at(i).value().toChar() == Char('\n')
				) {
					// Skip everything to next line
					Kumir::IO::readLine(fileReference, !fileIO);
				} else if (references.at(i).baseType() == VT_char) {
					Char value = Kumir::IO::readChar(fileReference, !fileIO);
					references.at(i).setValue(AnyValue(value));
				} else if (references.at(i).baseType() == VT_string) {
					const String &value = Kumir::IO::readLine(fileReference, !fileIO);
					references.at(i).setValue(AnyValue(value));
				}
				if (Kumir::Core::getError().length() > 0 && error_.length() == 0) {
					error_ = Kumir::Core::getError();
				}
				if (error_.length() > 0) {
					break;
				}
			}
			if (stacksMutex_) {
				stacksMutex_->unlock();
			}
		}
		const int lineNo = contextsStack_.top().lineNo;
		if (lineNo != -1 && debugHandler_ && hasInput &&
			!blindMode_ &&
			contextsStack_.top().type != EL_BELOWMAIN &&
			error_.empty()
		) {
			String margin;
			margin.reserve(100);
			for (int i = 0; i < (int)references.size(); i++) {
				const Variable &var = references.at(i);
				if (var.isConstant()) {
					continue;
				}
				String svalue;
				if (var.baseType() == VT_char) {
					svalue.push_back('\'');
					svalue.push_back(var.value().toChar());
					svalue.push_back('\'');
				} else if (var.baseType() == VT_string) {
					svalue.push_back('"');
					svalue.append(var.value().toString());
					svalue.push_back('"');
				} else if (var.baseType() == VT_record) {
					String localError;
					svalue = (*customTypeToString_)(var, &localError);

				} else {
					svalue = var.value().toString();
				}
				if (svalue.length() == 0) {
					continue;
				}
				if (i > 0) {
					margin.push_back(',');
					margin.push_back(' ');
				}
				margin += var.fullReferenceName();
				margin.push_back('=');
				margin += svalue;
			}
			if (contextsStack_.top().moduleContextNo == 0) {
				debugHandler_->appendTextToMargin(lineNo, margin);
			}
		}
	}
	if (alg == 0x01) {
		// Output
		if (stacksMutex_) {
			stacksMutex_->lock();
		}
		bool fileIO = false;
		Kumir::FileType fileReference;
		if (argsCount % 3) {
			fileIO = true;
			const Variable fileVar = valuesStack_.pop();
			if (!fileVar.isValid()) {
				error_ = Kumir::Core::fromUtf8("Нет значения у ключа файла");
				return;
			}
			const Record fileReferenceRecord = fileVar.toRecord();
			fileReference = fromRecordValue(fileReferenceRecord);

		}
		int varsCount = argsCount / 3;
		std::deque< std::pair<int, int> > formats;
		std::deque<Variable> values;
		for (int i = 0; i < varsCount; i++) {
			std::pair<int, int> format;
			format.second = valuesStack_.pop().toInt();
			format.first = valuesStack_.pop().toInt();
			formats.push_front(format);
			const Variable &ref = valuesStack_.pop();
			values.push_front(ref);
		}
		if (stacksMutex_) {
			stacksMutex_->unlock();
		}
		if (output_ && !fileIO && !Kumir::Files::overloadedStdOut()) {
			// output functor works like output operator here
			(*output_)(values, formats, &error_);
		} else {
			for (int i = 0; i < varsCount; i++) {
				if (values.at(i).baseType() == VT_int) {
					Kumir::IO::writeInteger(formats[i].first, values.at(i).toInt(), fileReference, !fileIO);
				} else if (values.at(i).baseType() == VT_real) {
					Kumir::IO::writeReal(formats[i].first, formats[i].second, values.at(i).toReal(), fileReference, !fileIO);
				} else if (values.at(i).baseType() == VT_bool) {
					Kumir::IO::writeBool(formats[i].first, values.at(i).toBool(), fileReference, !fileIO);
				} else if (values.at(i).baseType() == VT_char) {
					Kumir::IO::writeChar(formats[i].first, values.at(i).toChar(), fileReference, !fileIO);
				} else if (values.at(i).baseType() == VT_string) {
					Kumir::IO::writeString(formats[i].first, values.at(i).toString(), fileReference, !fileIO);
				}
				if (Kumir::Core::getError().length() > 0 && error_.length() == 0) {
					error_ = Kumir::Core::getError();
				}
				if (error_.length() > 0) {
					break;
				}
			}
		}
	} else if (alg == 0x04) {
		// Get char from string
		if (stacksMutex_) {
			stacksMutex_->lock();
		}

		Variable second = valuesStack_.pop();
		int index = second.value().toInt();

		Variable first = valuesStack_.pop();
		//const String &s = first.value().toString();
		const String &s = first.toStringRef();

		error_ = Kumir::Core::getError();
		if (error_.length() == 0) {
			if (index < 1 || index > (int) s.length()) {
				error_ = Kumir::Core::fromUtf8("Индекс символа больше длины строки");
			} else {
				Char result = s[index - 1];
				Variable r(result);
				valuesStack_.push(r);
			}
		}

		if (stacksMutex_) {
			stacksMutex_->unlock();
		}
	} else if (alg == 0x05) {
		// Set char in string
		if (stacksMutex_) {
			stacksMutex_->lock();
		}
		Variable third = valuesStack_.pop();
		Variable second = valuesStack_.pop();
		Variable first = valuesStack_.pop();
		int index = third.value().toInt();
		String source = second.value().toString();
		Char ch = first.value().toChar();
		error_ = Kumir::Core::getError();
		if (error_.length() == 0) {
			if (index < 1) {
				error_ = Kumir::Core::fromUtf8("Индекс символа меньше 1");
			} else if (index > (int)source.length()) {
				error_ = Kumir::Core::fromUtf8("Индекс символа больше длины строки");
			} else {
				source[index - 1] = ch;
				Variable r(source);
				valuesStack_.push(r);
			}
		}
		if (stacksMutex_) {
			stacksMutex_->unlock();
		}
	} else if (alg == 0x06) {
		// Get slice from string
		if (stacksMutex_) {
			stacksMutex_->lock();
		}
		Variable third = valuesStack_.pop();
		Variable second = valuesStack_.pop();
		Variable first = valuesStack_.pop();
		int start = second.value().toInt();
		int end   = third.value().toInt();
		const String &s = first.toStringRef();
		error_ = Kumir::Core::getError();
		if (error_.length() == 0) {
			if (start < 1 || start > (int)s.length()) {
				error_ = Kumir::Core::fromUtf8("Левая граница вырезки за пределами строки");
			} else if (end < start) {
				String empty;
				valuesStack_.push(Variable(empty));
			} else if (end < 1 || end > (int) s.length()) {
				error_ = Kumir::Core::fromUtf8("Правая граница вырезки за пределами строки");
			} else {
				String result = s.substr(start - 1, end - start + 1);
				Variable r(result);
				valuesStack_.push(r);
			}
		}
		if (stacksMutex_) {
			stacksMutex_->unlock();
		}
	} else if (alg == 0x07) {
		// Set slice in string
		if (stacksMutex_) {
			stacksMutex_->lock();
		}
		Variable fourth = valuesStack_.pop();
		Variable third = valuesStack_.pop();
		Variable second = valuesStack_.pop();
		Variable first = valuesStack_.pop();
		int end = fourth.value().toInt();
		int start = third.value().toInt();
		String source = second.value().toString();
		String ch = first.value().toString();
		error_ = Kumir::Core::getError();

		if (error_.length() == 0) {
			if (end < start && start == 0) {
				source = ch + source;
				Variable r(source);
				valuesStack_.push(r);
			} else if (start > (int) source.length()) {
				error_ = Kumir::Core::fromUtf8("Левая граница вырезки за пределами строки");
			} else if (end < start && start > 0) {
				source.insert(start, ch);
				Variable r(source);
				valuesStack_.push(r);
			} else if (start == (int) source.length() + 1 && end <= start) {
				source.append(ch);
				Variable r(source);
				valuesStack_.push(r);
			} else if (end < 1 || end > (int) source.length()) {
				error_ = Kumir::Core::fromUtf8("Правая граница вырезки за пределами строки");
			} else if (start < 1 || start > (int) source.length()) {
				error_ = Kumir::Core::fromUtf8("Левая граница вырезки за пределами строки");
			} else if (end < start) {
				error_ = Kumir::Core::fromUtf8("Ошибка в границах вырезки");
			} else {
				source = source.substr(0, start - 1) + ch + source.substr(end);
				Variable r(source);
				valuesStack_.push(r);
			}
		}

		if (stacksMutex_) {
			stacksMutex_->unlock();
		}
	} else if (alg == 0xBB01) {
		if (stacksMutex_) {
			stacksMutex_->lock();
		}
		// Input argument
		int localId = argsCount; // Already removed from stack
		Variable ref = contextsStack_.top().locals[localId].toReference();
		if (stacksMutex_) {
			stacksMutex_->unlock();
		}
		(*getMainArgument_)(ref, &error_);
	} else if (alg == 0xBB02) {
		if (stacksMutex_) {
			stacksMutex_->lock();
		}
		// Output argument or return value
		int localId = argsCount; // Already removed from stack
		Variable ref = contextsStack_.top().locals[localId].toReference();
		if (stacksMutex_) {
			stacksMutex_->unlock();
		}
		(*returnMainValue_)(ref, &error_);

	}
}

}

