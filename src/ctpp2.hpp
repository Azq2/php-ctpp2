#pragma once

extern "C" {
	#ifdef HAVE_CONFIG_H
		#include "config.h"
	#endif
	#include "php.h"
}

#include <CDT.hpp>
#include <CTPP2Exception.hpp>
#include <CTPP2FileSourceLoader.hpp>
#include <CTPP2Parser.hpp>
#include <CTPP2ParserException.hpp>
#include <CTPP2StringOutputCollector.hpp>
#include <CTPP2SyscallFactory.hpp>
#include <CTPP2VM.hpp>
#include <CTPP2VMDebugInfo.hpp>
#include <CTPP2VMDumper.hpp>
#include <CTPP2VMException.hpp>
#include <CTPP2VMStackException.hpp>
#include <CTPP2VMExecutable.hpp>
#include <CTPP2VMMemoryCore.hpp>
#include <CTPP2VMOpcodeCollector.hpp>
#include <CTPP2VMSTDLib.hpp>
#include <CTPP2ErrorCodes.h>
#include <CTPP2Error.hpp>
#include <CTPP2SourceLoader.hpp>
#include <CTPP2Logger.hpp>
#include <CTPP2StringIconvOutputCollector.hpp>
#include <CTPP2JSONParser.hpp>
#include <CTPP2HashTable.hpp>

#define cpp_free(a) if (a) { delete a; a = NULL; }
#define c_free(a) if (a) { ::free(a); a = NULL; }
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define cslen(str) str, (sizeof(str) - 1)
#define lencs(str) (sizeof(str) - 1), str

#define cdlen(str) str, (sizeof(str))
#define lencd(str) (sizeof(str)), str

class CTPP2;

// CTPPPHPSyscallHandler
class CTPPPHPSyscallHandler: public CTPP::SyscallHandler {
	public:
		friend class STDLibInitializer;
		CTPPPHPSyscallHandler(const char *func_name, zval *perl_sub);
		~CTPPPHPSyscallHandler() throw();
		INT_32 Handler(CTPP::CDT *args, const UINT_32 args_n, CTPP::CDT &ret, CTPP::Logger &logger);
		CCHAR_P GetName() const;
	protected:
		zval *callback;
		char *name;
};

// CTPPPerlLogger
class CTPPPerlLogger: public CTPP::Logger {
	public:
		~CTPPPerlLogger() throw();
	private:
		INT_32 WriteLog(const UINT_32 priority, CCHAR_P str, const UINT_32 len);
};

// CTPPPHPVarOutputCollector
class CTPPPHPVarOutputCollector: public CTPP::OutputCollector {
	public:
		CTPPPHPVarOutputCollector(zval *var);
		~CTPPPHPVarOutputCollector() throw();
		INT_32 Collect(const void *raw, const UINT_32 len);
	protected:
		zval *var;
};

// CTPPPHPVarOutputCollector
class CTPPPHPOutputCollector: public CTPP::OutputCollector {
	public:
		CTPPPHPOutputCollector();
		~CTPPPHPOutputCollector() throw();
		INT_32 Collect(const void *raw, const UINT_32 len);
	protected:
		zval *var;
};

// Загрузчик текстового исходного кода для CTPP2 (по указателю, без копирования)
class CTPP2RefTextSourceLoader: public CTPP::CTPP2SourceLoader {
	protected:
		const char *text;
		size_t length;
		CTPP::CTPP2FileSourceLoader file_loader;
	public:
		CTPP2RefTextSourceLoader(const char *txt, size_t len);
		CTPP2RefTextSourceLoader(zval *txt);
		CCHAR_P GetTemplate(UINT_32 &size);
		
		// Костыли для <TMPL_include>
		INT_32 LoadTemplate(CCHAR_P filename);
		void SetIncludeDirs(const STLW::vector<STLW::string> &include_dirs);
		CTPP2SourceLoader *Clone();
		
		~CTPP2RefTextSourceLoader() throw();
};

// Bytecode
class Bytecode {
	protected:
		CTPP::VMMemoryCore *mem;
		CTPP::VMExecutable *exe;
		unsigned int exe_size;
		
		void _compiler(CTPP::CTPP2SourceLoader &loader, const char *name);
	public:
		friend class CTPP2;
		
		enum SourceType {
			T_TEXT_SOURCE = 0, 
			T_SOURCE = 1, 
			T_BYTECODE = 2, 
			T_TEXT_BYTECODE = 3
		};
		
		inline const CTPP::VMMemoryCore *getCode() {
			return mem;
		}
		
		inline bool check() {
			return mem != NULL;
		}
		
		Bytecode(STLW::vector<STLW::string> &inc_dirs, zval *text, const char *filename, SourceType type);
		const char *data() {
			return (const char *) exe;
		}
		const unsigned int length() {
			return exe_size;
		}
		int save(const char *filename);
		void free();
		~Bytecode();
};

// CTPP2
class CTPP2 {
	protected:
		typedef STLW::map<STLW::string, CTPP::SyscallHandler *> UserSyscallList;
		CTPP::CTPPError error;
		CTPP::CDT *params;
		CTPP::VM *vm;
		CTPP::SyscallFactory *syscalls;
		struct {
			bool convert;
			STLW::string src;
			STLW::string dst;
		} charset;
		STLW::vector<STLW::string> include_dirs;
		STLW::map<STLW::string, CTPP::SyscallHandler *> user_syscalls;
		
		friend class Bytecode;
	public:
		typedef CTPP::SyscallHandler *(*FnHandler)();
		
		static const FnHandler functions[];
		
		CTPP2(unsigned int arg_stack_size, unsigned int code_stack_size, unsigned int steps_limit, unsigned int max_functions, 
			STLW::string src_charset, STLW::string dst_charset);
		int param(zval *var);
		int json(const char *json, unsigned int length);
		int loadUDF(const char *filename);
		static void php2cdt(zval *var, CTPP::CDT *param);
		static zval *cdt2php(const CTPP::CDT *cdt);
		static void json2cdt(const char *json, unsigned int length, CTPP::CDT *cdt);
		CTPP2 *reset();
		Bytecode *parse(zval *text, const char *filename, Bytecode::SourceType type);
		void output(zval *var, Bytecode *bytecode, const char *src_enc, const char *dst_enc);
		void getLastError(zval *var);
		void dumpParams(zval *var);
		void bind(const char *name, zval *func);
		void bind(CTPP::SyscallHandler *sys);
		void unbind(const char *name);
		CTPP2 *setIncludeDirs(STLW::vector<STLW::string> &include_dirs);
		CTPP2 *setIncludeDirs(::HashTable *include_dirs);
		~CTPP2();
};
