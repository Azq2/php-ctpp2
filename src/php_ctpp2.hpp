#pragma once

#define PHP_CTPP2_VERSION  "0.1"
#define PHP_CTPP2_EXTNAME  "ctpp2"

#include "ctpp2.hpp"

extern "C" {
	#ifdef HAVE_CONFIG_H
		#include "config.h"
	#endif
	#include "php.h"
}

#include <unordered_map>
#include "zend_API.h"

#include "zend_interfaces.h"
#include "zend_modules.h"
#include "zend_exceptions.h"
#include "ext/standard/info.h"

#define CTPP2_CLASS_NAME "CTPP2"

static int le_ctpp_bytecode;

ZEND_BEGIN_MODULE_GLOBALS(ctpp2)
	long arg_stack_size;
	long code_stack_size;
	long steps_limit;
	long cache_bytecode;
	long max_functions;
	long debug_level;
ZEND_END_MODULE_GLOBALS(ctpp2)

#ifdef ZTS
    #define CTPP_G(v) TSRMG(ctpp2_globals_id, zend_ctpp2_globals *, v)
#else
    #define CTPP_G(v) (ctpp2_globals.v)
#endif

struct php_ctpp2_object {
	zend_object std; 
	CTPP2 *ctpp;
};

zend_class_entry *php_ctpp2_ce;
zend_object_handlers php_ctpp2_object_handlers;

PHP_MINIT_FUNCTION(ctpp2);
PHP_MSHUTDOWN_FUNCTION(ctpp2);
PHP_RINIT_FUNCTION(ctpp2);
PHP_RSHUTDOWN_FUNCTION(ctpp2);
PHP_MINFO_FUNCTION(ctpp2);

extern zend_module_entry ctpp2_module_entry;

PHP_METHOD(CTPP2, __construct);
PHP_METHOD(CTPP2, setIncludeDirs);
PHP_METHOD(CTPP2, parseTemplate);
PHP_METHOD(CTPP2, parseText);
PHP_METHOD(CTPP2, loadBytecode);
PHP_METHOD(CTPP2, loadBytecodeString);
PHP_METHOD(CTPP2, saveBytecode);
PHP_METHOD(CTPP2, dumpBytecode);
PHP_METHOD(CTPP2, freeBytecode);
PHP_METHOD(CTPP2, param);
PHP_METHOD(CTPP2, reset);
PHP_METHOD(CTPP2, json);
PHP_METHOD(CTPP2, output);
PHP_METHOD(CTPP2, display);
PHP_METHOD(CTPP2, dump);
PHP_METHOD(CTPP2, getLastError);
PHP_METHOD(CTPP2, bind);
PHP_METHOD(CTPP2, unbind);

static void php_ctpp2_free_storage(void *object TSRMLS_DC);
static zend_object_value php_ctpp2_create_handler(zend_class_entry *type TSRMLS_DC);
static void init_ctpp2_class();
static ZEND_RSRC_DTOR_FUNC(php_ctpp2_destroy_bytecode);
