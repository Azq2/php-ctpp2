#include "ctpp2.hpp"
using namespace CTPP;

// CTPPPHPSyscallHandler
CTPPPHPSyscallHandler::CTPPPHPSyscallHandler(const char *func_name, zval *sub) {
	name = strdup(func_name);
	callback = sub;
	Z_ADDREF_P(callback);
}

INT_32 CTPPPHPSyscallHandler::Handler(CDT *args, const UINT_32 args_n, CDT &ret, Logger &logger) {
	zval retval, *params = (zval *) ecalloc(args_n, sizeof(zval));
	
	for (unsigned int i = args_n, j = 0; i-- > 0; ) {
		CTPP2::cdt2php(&params[j], &args[i]);
		Z_TRY_ADDREF_P(&params[j]);
		++j;
	}
	
	if (call_user_function(CG(function_table), NULL, callback, &retval, args_n, params TSRMLS_CC) != SUCCESS) {
		php_error(E_ERROR, "CTPPPHPSyscallHandler: Some bullshit when callings PHP function. ");
	} else {
		CTPP2::php2cdt(&retval, &ret);
	}
	
	return 0;
}
CCHAR_P CTPPPHPSyscallHandler::GetName() const {
	return name;
}
CTPPPHPSyscallHandler::~CTPPPHPSyscallHandler() throw() {
	free(name);
	if (callback)
		Z_DELREF_P(callback);
}

// CTPPPerlLogger
CTPPPerlLogger::~CTPPPerlLogger() throw() { }
INT_32 CTPPPerlLogger::WriteLog(const UINT_32 priority, CCHAR_P str, const UINT_32 len) {
	php_error(E_WARNING, "ERROR: %.*s", len, str);
	return 0;
}

// CTPP2RefTextSourceLoader
CTPP2RefTextSourceLoader::CTPP2RefTextSourceLoader(const char *txt, size_t len) {
	text = txt;
	length = len;
}
CCHAR_P CTPP2RefTextSourceLoader::GetTemplate(UINT_32 &size) {
	size = length;
	return text;
}
INT_32 CTPP2RefTextSourceLoader::LoadTemplate(CCHAR_P filename) {
	return file_loader.LoadTemplate(filename);
}
void CTPP2RefTextSourceLoader::SetIncludeDirs(const STLW::vector<STLW::string> &include_dirs) {
	file_loader.SetIncludeDirs(include_dirs);
}
CTPP2SourceLoader *CTPP2RefTextSourceLoader::Clone() {
	// Возвращает лоадер для include()
	return file_loader.Clone();
}
CTPP2RefTextSourceLoader::~CTPP2RefTextSourceLoader() throw() { }

// CTPPPHPOutputCollector
CTPPPHPOutputCollector::CTPPPHPOutputCollector() { }
INT_32 CTPPPHPOutputCollector::Collect(const void *raw, const UINT_32 len) {
	php_write((void *)raw, len TSRMLS_CC);
	return 0;
}
CTPPPHPOutputCollector::~CTPPPHPOutputCollector() throw() { }

// CTPPPHPVarOutputCollector
CTPPPHPVarOutputCollector::CTPPPHPVarOutputCollector(zval *v) {
	this->var = v;
	ZVAL_STRINGL(v, "", 0);
}
INT_32 CTPPPHPVarOutputCollector::Collect(const void *raw, const UINT_32 len) {
	if (len) {
		zend_string *str = Z_STR_P(this->var);
		str = zend_string_extend(str, str->len + len, 0);
		memcpy(str->val + str->len - len, raw, len);
		Z_STR_P(this->var) = str;
	}
	return 0;
}
CTPPPHPVarOutputCollector::~CTPPPHPVarOutputCollector() throw() { }

// CTPP2
CTPP2::CTPP2(unsigned int arg_stack_size, unsigned int code_stack_size, unsigned int steps_limit, unsigned int max_functions, 
		STLW::string src_charset, STLW::string dst_charset) {
	try {
		params = new CDT(CDT::HASH_VAL);
		syscalls = new SyscallFactory(max_functions);
		STDLibInitializer::InitLibrary(*syscalls);
		vm = new VM(syscalls, arg_stack_size, code_stack_size, steps_limit);
		
		int i = 0;
		while (functions[i])
			bind(functions[i++]());
		
		if (src_charset.size() && dst_charset.size()) {
			charset.src = src_charset;
			charset.dst = dst_charset;
			charset.convert = true;
		} else {
			// Конвертирование не требуется
			charset.convert = false;
		}
	} catch (...) {
		cpp_free(params);
		if (syscalls) {
			STDLibInitializer::DestroyLibrary(*syscalls);
			cpp_free(syscalls);
		}
		cpp_free(vm);
	}
}

void CTPP2::bind(SyscallHandler *sys) {
	SyscallHandler *h = syscalls->GetHandlerByName(sys->GetName());
	if (h) {
		throw CTPPLogicError("Cannot redefine function!");
	} else {
		syscalls->RegisterHandler(sys);
		user_syscalls[sys->GetName()] = sys;
	}
}

void CTPP2::bind(const char *name, zval *func) {
	SyscallHandler *h = syscalls->GetHandlerByName(name);
	if (h) {
		php_error(E_ERROR, "CTPP2: Cannot redefine udf %s", name);
	} else {
		CTPPPHPSyscallHandler *handler = new CTPPPHPSyscallHandler(name, func);
		syscalls->RegisterHandler(handler);
		user_syscalls[name] = handler;
	}
}

void CTPP2::unbind(const char *name) {
	SyscallHandler *h = syscalls->GetHandlerByName(name);
	if (!h) {
		php_error(E_ERROR, "CTPP2: Cannot unbind unknown udf %s", name);
	} else {
		UserSyscallList::iterator it;
		if ((it = user_syscalls.find(name)) != user_syscalls.end()) {
			delete h;
			user_syscalls.erase(it);
		}
		syscalls->RemoveHandler(name);
	}
}

int CTPP2::loadUDF(const char *filename) {
	php_error(E_ERROR, "CTPP2: // Nothing todo. =)\n");
}

Bytecode *CTPP2::parse(const char *filename, const char *data, size_t size, Bytecode::SourceType type) {
	try {
		return new Bytecode(include_dirs, filename, data, size, type);
	} catch (CTPPParserSyntaxError &e) {
		error = CTPPError(filename, e.what(), CTPP_COMPILER_ERROR | CTPP_SYNTAX_ERROR, e.GetLine(), e.GetLinePos(), 0);
	} catch (CTPPParserOperatorsMismatch& e) {
		error = CTPPError(filename, STLW::string("Expected ") + e.Expected() + ", but found " + e.Found(), CTPP_COMPILER_ERROR | CTPP_SYNTAX_ERROR, e.GetLine(), e.GetLinePos(), 0);
	} catch (CTPPUnixException &e) {
		error = CTPPError(filename, e.what(), CTPP_COMPILER_ERROR | CTPP_UNIX_ERROR, 0, 0, 0);
	} catch (CDTRangeException &e) {
		error = CTPPError(filename, e.what(), CTPP_COMPILER_ERROR | CTPP_RANGE_ERROR, 0, 0, 0);
	} catch (CDTAccessException &e) {
		error = CTPPError(filename, e.what(), CTPP_COMPILER_ERROR | CTPP_ACCESS_ERROR, 0, 0, 0);
	} catch (CDTTypeCastException &e) {
		error = CTPPError(filename, e.what(), CTPP_COMPILER_ERROR | CTPP_TYPE_CAST_ERROR, 0, 0, 0);
	} catch (CTPPLogicError &e) {
		error = CTPPError(filename, e.what(), CTPP_COMPILER_ERROR | CTPP_LOGIC_ERROR, 0, 0, 0);
	} catch (CTPPException &e) {
		error = CTPPError(filename, e.what(), CTPP_COMPILER_ERROR | CTPP_UNKNOWN_ERROR, 0, 0, 0);
	} catch (STLW::exception &e) {
		error = CTPPError(filename, e.what(), CTPP_COMPILER_ERROR | STL_UNKNOWN_ERROR, 0, 0, 0);
	} catch (...) {
		error = CTPPError(filename, "Unknown Error", CTPP_COMPILER_ERROR | STL_UNKNOWN_ERROR, 0, 0, 0);
	}
	
	php_error(E_WARNING, "CTPP2: In file %s at line %d, pos %d: %s (error code 0x%08X)", 
		error.template_name.c_str(), error.line, error.pos, error.error_descr.c_str(), error.error_code);
	return NULL;
}

CTPP2 *CTPP2::setIncludeDirs(::HashTable *hash) {
	size_t i = 0;
	zend_string *key;
	ulong num_key;
	zval *val;
	if (hash) {
		ZEND_HASH_FOREACH_KEY_VAL(hash, num_key, key, val) {
			if (Z_TYPE_P(val) != IS_STRING) {
				php_error(E_WARNING, "CTPP2: Not string at array index %ld", i);
			} else {
				this->include_dirs.push_back(STLW::string(Z_STRVAL_P(val), Z_STRLEN_P(val)));
			}
			++i;
		} ZEND_HASH_FOREACH_END();
	}
	return this;
}

void CTPP2::output(zval *out, Bytecode *bytecode, const char *src_enc, const char *dst_enc) {
	unsigned int IP = 0;
	
	if (!bytecode || !bytecode->check()) {
		error = CTPPError("", "Invalid Bytecode", CTPP_VM_ERROR | STL_UNKNOWN_ERROR, 0, 0, IP);
	} else {
		try {
			if (charset.convert || (src_enc && dst_enc)) {
				STLW::string src_charset, dst_charset;
				if (src_enc && dst_enc) {
					src_charset = STLW::string(src_enc);
					dst_charset = STLW::string(dst_enc);
				} else {
					src_charset = charset.src;
					dst_charset = charset.dst;
				}
				
				STLW::string result;
				CTPPPerlLogger logger;
				StringIconvOutputCollector output_collector(result, src_charset, dst_charset, 3);
				vm->Init(bytecode->getCode(), &output_collector, &logger);
				vm->Run(bytecode->getCode(), &output_collector, IP, *params, &logger);
				vm->Reset();
				
				ZVAL_STRINGL(out, result.data(), result.length());
				return;
			} else {
				CTPPPerlLogger logger;
				if (out) {
				//	STLW::string result;
				//	StringOutputCollector output_collector(result);
				//	vm->Init(bytecode->getCode(), &output_collector, &logger);
				//	vm->Run(bytecode->getCode(), &output_collector, IP, *params, &logger);
				//	ZVAL_STRINGL(out, result.data(), result.length());
					
					CTPPPHPVarOutputCollector output_collector(out);
					vm->Init(bytecode->mem, &output_collector, &logger);
					vm->Run(bytecode->mem, &output_collector, IP, *params, &logger);
				} else {
					CTPPPHPOutputCollector output_collector;
					vm->Init(bytecode->mem, &output_collector, &logger);
					vm->Run(bytecode->mem, &output_collector, IP, *params, &logger);
				}
				vm->Reset();
				return;
			}
		} catch (ZeroDivision &e) {
			error = CTPPError(e.GetSourceName(), e.what(), CTPP_VM_ERROR | CTPP_ZERO_DIVISION_ERROR, VMDebugInfo(e.GetDebugInfo()).GetLine(), 
				VMDebugInfo(e.GetDebugInfo()).GetLinePos(), e.GetIP());
		} catch (ExecutionLimitReached &e) {
			error = CTPPError(e.GetSourceName(), e.what(), CTPP_VM_ERROR | CTPP_EXECUTION_LIMIT_REACHED_ERROR, VMDebugInfo(e.GetDebugInfo()).GetLine(), 
				VMDebugInfo(e.GetDebugInfo()).GetLinePos(), e.GetIP());
		} catch (CodeSegmentOverrun &e) {
			error = CTPPError(e.GetSourceName(), e.what(), CTPP_VM_ERROR | CTPP_CODE_SEGMENT_OVERRUN_ERROR, VMDebugInfo(e.GetDebugInfo()).GetLine(), 
				VMDebugInfo(e.GetDebugInfo()).GetLinePos(), e.GetIP());
		} catch (InvalidSyscall &e) {
			if (e.GetIP() != 0) {
				error = CTPPError(e.GetSourceName(), e.what(), CTPP_VM_ERROR | CTPP_INVALID_SYSCALL_ERROR, VMDebugInfo(e.GetDebugInfo()).GetLine(), 
					VMDebugInfo(e.GetDebugInfo()).GetLinePos(), e.GetIP());
			} else {
				error = CTPPError(e.GetSourceName(), STLW::string("Unsupported syscall: \"") + e.what() + "\"", CTPP_VM_ERROR | CTPP_INVALID_SYSCALL_ERROR, 
					VMDebugInfo(e.GetDebugInfo()).GetLine(), VMDebugInfo(e.GetDebugInfo()).GetLinePos(), e.GetIP());
			}
		} catch (IllegalOpcode &e) {
			error = CTPPError(e.GetSourceName(), e.what(), CTPP_VM_ERROR | CTPP_ILLEGAL_OPCODE_ERROR, VMDebugInfo(e.GetDebugInfo()).GetLine(), 
				VMDebugInfo(e.GetDebugInfo()).GetLinePos(), e.GetIP());
		} catch (StackOverflow &e) {
			error = CTPPError(e.GetSourceName(), e.what(), CTPP_VM_ERROR | CTPP_STACK_OVERFLOW_ERROR, VMDebugInfo(e.GetDebugInfo()).GetLine(), 
				VMDebugInfo(e.GetDebugInfo()).GetLinePos(), e.GetIP());
		} catch (StackUnderflow &e) {
			error = CTPPError(e.GetSourceName(), e.what(), CTPP_VM_ERROR | CTPP_STACK_UNDERFLOW_ERROR, VMDebugInfo(e.GetDebugInfo()).GetLine(),
				VMDebugInfo(e.GetDebugInfo()).GetLinePos(), e.GetIP());
		} catch (VMException &e) {
			error = CTPPError(e.GetSourceName(), e.what(), CTPP_VM_ERROR | CTPP_VM_GENERIC_ERROR, VMDebugInfo(e.GetDebugInfo()).GetLine(),
				VMDebugInfo(e.GetDebugInfo()).GetLinePos(), e.GetIP());
		} catch (CTPPUnixException &e) {
			error = CTPPError("", e.what(), CTPP_VM_ERROR | CTPP_UNIX_ERROR, 0, 0, IP);
		} catch (CDTRangeException &e) {
			error = CTPPError("", e.what(), CTPP_VM_ERROR | CTPP_RANGE_ERROR, 0, 0, IP);
		} catch (CDTAccessException &e) {
			error = CTPPError("", e.what(), CTPP_VM_ERROR | CTPP_ACCESS_ERROR, 0, 0, IP);
		} catch (CDTTypeCastException &e) {
			error = CTPPError("", e.what(), CTPP_VM_ERROR | CTPP_TYPE_CAST_ERROR, 0, 0, IP);
		} catch (CTPPLogicError &e) {
			error = CTPPError("", e.what(), CTPP_VM_ERROR | CTPP_LOGIC_ERROR, 0, 0, IP);
		} catch(CTPPCharsetRecodeException &e) {
			error = CTPPError("", e.what(), CTPP_VM_ERROR | CTPP_CHARSET_RECODE_ERROR, 0, 0, 0);
		} catch (CTPPException &e) {
			error = CTPPError("", e.what(), CTPP_VM_ERROR | CTPP_UNKNOWN_ERROR, 0, 0, IP);
		} catch (STLW::exception &e) {
			error = CTPPError("", e.what(), CTPP_VM_ERROR | STL_UNKNOWN_ERROR, 0, 0, IP);
		} catch (...) {
			error = CTPPError("", "Unknown Error", CTPP_VM_ERROR | STL_UNKNOWN_ERROR, 0, 0, IP);
		}
	}
	vm->Reset();
	
	if (error.line > 0) {
		php_error(E_WARNING, "CTPP2::output(): %s (error code 0x%08X); IP: 0x%08X, file %s line %d pos %d", error.error_descr.c_str(),
			error.error_code, error.ip, error.template_name.c_str(), error.line, error.pos);
	} else {
		php_error(E_WARNING, "CTPP2::output(): %s (error code 0x%08X); IP: 0x%08X", error.error_descr.c_str(), error.error_code, error.ip);
	}
	
	if (out)
		ZVAL_BOOL(out, 0);
}

CTPP2::~CTPP2() {
	for (UserSyscallList::iterator it = user_syscalls.begin(), end = user_syscalls.end(); it != end; ++it)
		delete it->second;
	if (vm)
		delete vm;
	if (params)
		delete params;
	if (syscalls) {
		STDLibInitializer::DestroyLibrary(*syscalls);
		delete syscalls;
	}
}

CTPP2 *CTPP2::reset() {
	delete params;
	params = new CDT(CDT::HASH_VAL);
	return this;
}

int CTPP2::param(zval *var) {
	php2cdt(var, params);
	return  0;
}

int CTPP2::json(const char *json, unsigned int length) {
	try {
		json2cdt(json, length, params);
		return -1;
	} catch (CTPPParserSyntaxError &e) {
		error = CTPPError("{inline json}", e.what(), CTPP_COMPILER_ERROR | CTPP_SYNTAX_ERROR, e.GetLine(), e.GetLinePos(), 0);
	} catch (...) {
		error = CTPPError("{inline json}", "Unknown JSON Error", CTPP_COMPILER_ERROR | STL_UNKNOWN_ERROR, 0, 0, 0);
	}
	if (error.line > 0) {
		php_error(E_WARNING, "CTPP2::json_param(): %s (error code 0x%08X)", error.error_descr.c_str(), error.error_code);
	} else {
		php_error(E_WARNING, "CTPP2::json_param(): %s (error code 0x%08X) at line %d pos %d", 
			error.error_descr.c_str(), error.error_code, error.line, error.pos);
	}
	return  0;
}

void CTPP2::json2cdt(const char *json, unsigned int length, CDT *cdt) {
	CTPP2JSONParser jparser(*cdt);
	jparser.Parse(json, json + length);
}

void CTPP2::cdt2php(zval *ret, const CDT *cdt) {
	switch (cdt->GetType()) {
		case CDT::INT_VAL:
			ZVAL_LONG(ret, cdt->GetInt());
		break;
		
		case CDT::REAL_VAL:
			ZVAL_DOUBLE(ret, cdt->GetInt());
		break;
		
		case CDT::STRING_REAL_VAL:
		case CDT::STRING_INT_VAL:
		case CDT::STRING_VAL:
			ZVAL_STRINGL(ret, cdt->GetString().c_str(), cdt->GetString().size());
		break;
		
		case CDT::ARRAY_VAL:
		{
			array_init(ret);
			unsigned int array_size = cdt->Size();
			for (unsigned i = 0; i < array_size; ++i) {
				zval val;
				cdt2php(&val, &(*cdt)[i]);
				add_index_zval(ret, i, &val);
			}
		}
		break;
		
		case CDT::HASH_VAL:
		{
			array_init(ret);
			for (CDT::ConstIterator it = cdt->Begin(), end = cdt->End(); it != end; ++it) {
				zval val;
				cdt2php(&val, &it->second);
				add_assoc_zval_ex(ret, it->first.c_str(), it->first.size() + 1, &val);
			}
		}
		break;
		
		default:
			ZVAL_NULL(ret);
		break;
	}
}

void CTPP2::php2cdt(zval *var, CDT *cdt) {
	char buffer[33];
	if (!var)
		return;
	
	switch(Z_TYPE_P(var)) {
		case IS_LONG:
			cdt->operator=(INT_64(Z_LVAL_P(var)));
		break;
		
		case IS_DOUBLE:
			cdt->operator=(W_FLOAT(Z_DVAL_P(var)));
		break;
		
		case IS_STRING:
			cdt->operator=(STLW::string(Z_STRVAL_P(var), Z_STRLEN_P(var)));
		break;

#ifdef IS_REFERENCE
		case IS_REFERENCE:
			php2cdt(Z_REFVAL_P(var), cdt);
		break;
#endif

#ifdef IS_BOOL
		case IS_BOOL:
			cdt->operator=(Z_LVAL_P(var) != 0);
		break;
#else
		case IS_TRUE:
			cdt->operator=(1);
		break;
		
		case IS_FALSE:
			cdt->operator=(0);
		break;
#endif

		case IS_NULL:
			// Nothing
		break;
		
		default:
			if (Z_TYPE_P(var) == IS_OBJECT && zend_is_callable(var, 0, NULL TSRMLS_CC)) {
				zval retval;
				if (call_user_function(CG(function_table), NULL, var, &retval, 0, NULL TSRMLS_CC) != SUCCESS) {
					php_error(E_WARNING, "php2cdt: Something bullshit when calling PHP function. ");
				} else {
					CTPP2::php2cdt(&retval, cdt);
				}
			} else {
				// Пытаемся сконвертить в строку
				convert_to_string(var);
				if (Z_TYPE_P(var) == IS_STRING)
					cdt->operator = (STLW::string(Z_STRVAL_P(var), Z_STRLEN_P(var)));
			}
		break;
		
		case IS_ARRAY:
#ifdef IS_CONSTANT_ARRAY
		case IS_CONSTANT_ARRAY:
#endif
		{
			::HashTable *hash = Z_ARRVAL_P(var);
			ulong index, last_index = 0;
			
			CDT::eValType type = cdt->GetType();
			bool hash_inited = (type == CTPP::CDT::HASH_VAL || type == CTPP::CDT::ARRAY_VAL);
			
			zval *val; zend_string *key;
			ZEND_HASH_FOREACH_KEY_VAL(hash, index, key, val) {
				CTPP::CDT tmp;
				if (!hash_inited) {
					// Если строковый ключ или первый элемент не 0, значит это хэш
					type = key || index != 0 ? CTPP::CDT::HASH_VAL : CTPP::CDT::ARRAY_VAL;
					cdt->operator = (CTPP::CDT(type));
					hash_inited = true;
				} else if ((key || index - last_index != 1) && type == CTPP::CDT::ARRAY_VAL) {
					// Если текущий тип массив, но встретили строковый ключ или индекс не по порядку
					// Конвертим массив в хэш
					CTPP::CDT tmp_hash(type = CTPP::CDT::HASH_VAL);
					size_t array_size = cdt->Size();
					for (size_t i = 0; i < array_size; ++i) {
						int length = snprintf(buffer, sizeof(buffer), "%lu", i);
						tmp_hash[STLW::string(buffer, length)] = cdt->operator[](index);
					}
					cdt->operator = (tmp_hash);
				}
				last_index = index;
				
				if (type == CTPP::CDT::HASH_VAL) {
					php2cdt(val, &tmp);
					if (key) {
						cdt->operator[](STLW::string(key->val, key->len)) = tmp;
					} else {
						int length = snprintf(buffer, sizeof(buffer), "%lu", index);
						cdt->operator[](STLW::string(buffer, length)) = tmp;
					}
				} else {
					php2cdt(val, &tmp);
					cdt->operator[](index) = tmp;
				}
			} ZEND_HASH_FOREACH_END();
		}
		break;
	}
}

// Bytecode
Bytecode::Bytecode(STLW::vector<STLW::string> &inc_dirs, const char *filename, const char *data, size_t size, SourceType type) {
	exe = NULL; mem = NULL;
	
	if (type == T_TEXT_SOURCE) {
		CTPP2RefTextSourceLoader loader(data, size);
		loader.SetIncludeDirs(inc_dirs);
		_compiler(loader, filename);
	} else if (type == T_SOURCE) {
		CTPP2FileSourceLoader loader;
		loader.SetIncludeDirs(inc_dirs);
		loader.LoadTemplate(filename);
		_compiler(loader, filename);
	} else if (type == T_BYTECODE) {
		struct stat st;
		if (stat(filename, &st) != 0) {
			throw CTPPUnixException("No such file", errno);
		} else {
			if (st.st_size < 5)
				throw CTPPLogicError("Empty file");
			
			FILE *fp = fopen(filename, "r");
			if (!fp)
				throw CTPPUnixException("fopen", errno);
			
			exe = (VMExecutable *) malloc(st.st_size);
			int readed = fread(exe, st.st_size, 1, fp);
			if (readed != 1) {
				c_free(exe);
				fclose(fp);
				throw CTPPLogicError("File read error (truncated)");
			}
			fclose(fp);
			
			if (exe->magic[0] == 'C' && exe->magic[1] == 'T' && exe->magic[2] == 'P' && exe->magic[3] == 'P') {
				mem = new VMMemoryCore(exe);
			} else {
				c_free(exe);
				throw CTPPLogicError("Not an CTPP bytecode file!");
			}
		}
	} else if (type == T_TEXT_BYTECODE) {
		if (size < 5)
			throw CTPPLogicError("File read error (truncated)");
		exe = (VMExecutable *) malloc(size);
		memcpy(exe, data, size);
		
		if (exe->magic[0] == 'C' && exe->magic[1] == 'T' && exe->magic[2] == 'P' && exe->magic[3] == 'P') {
			mem = new VMMemoryCore(exe);
		} else {
			c_free(exe);
			throw CTPPLogicError("Not an CTPP bytecode!");
		}
	}
}

// Bytecode
void Bytecode::_compiler(CTPP2SourceLoader &loader, const char *name) {
	// Компилятор
	VMOpcodeCollector  vm_op_collector;
	StaticText         syscalls;
	StaticData         static_data;
	StaticText         static_text;
	CTPP::HashTable    hash_table;
	CTPP2Compiler compiler(vm_op_collector, syscalls, static_data, static_text, hash_table);
	
	// Создаём парсер и компилим
	CTPP2Parser parser(&loader, &compiler, name);
	parser.Compile();
	
	// Сам код шаблона
	unsigned int code_size = 0;
	const VMInstruction * vm_instructions = vm_op_collector.GetCode(code_size);
	
	// Дампим в байткод
	VMDumper dumper(code_size, vm_instructions, syscalls, static_data, static_text, hash_table);
	const VMExecutable *bytecode = dumper.GetExecutable(exe_size);
	
	// Аллочим память
	exe = (VMExecutable *) malloc(exe_size);
	memcpy(exe, bytecode, exe_size);
	mem = new VMMemoryCore(exe);
}
void Bytecode::free() {
	c_free(exe);
	cpp_free(mem);
}
Bytecode::~Bytecode() {
	free();
}
