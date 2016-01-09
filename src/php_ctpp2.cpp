#include "php_ctpp2.hpp"

ZEND_DECLARE_MODULE_GLOBALS(ctpp2);

static zend_function_entry functions[] = {
	PHP_FE_END
};

static const zend_module_dep ctpp2_deps[] = {
	ZEND_MOD_END
};

zend_module_entry ctpp2_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
        STANDARD_MODULE_HEADER_EX, NULL, 
        (zend_module_dep*) ctpp2_deps, 
#endif
        PHP_CTPP2_EXTNAME, 
        functions, 
        PHP_MINIT(ctpp2), 
        PHP_MSHUTDOWN(ctpp2), 
        PHP_RINIT(ctpp2), 
        PHP_RSHUTDOWN(ctpp2), 
        PHP_MINFO(ctpp2), 
#if ZEND_MODULE_API_NO >= 20010901
        PHP_CTPP2_VERSION, 
#endif
        STANDARD_MODULE_PROPERTIES
};

extern "C" {
	#ifdef COMPILE_DL_CTPP2
		ZEND_GET_MODULE(ctpp2)
	#endif
}

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("ctpp.arg_stack_size",       "10240",   PHP_INI_ALL, OnUpdateLong, arg_stack_size,  zend_ctpp2_globals, ctpp2_globals)
	STD_PHP_INI_ENTRY("ctpp.code_stack_size",      "10240",   PHP_INI_ALL, OnUpdateLong, code_stack_size, zend_ctpp2_globals, ctpp2_globals)
	STD_PHP_INI_ENTRY("ctpp.steps_limit",          "1048576", PHP_INI_ALL, OnUpdateLong, steps_limit,     zend_ctpp2_globals, ctpp2_globals)
	STD_PHP_INI_ENTRY("ctpp.debug_level",          "0",       PHP_INI_ALL, OnUpdateLong, debug_level,     zend_ctpp2_globals, ctpp2_globals)
	STD_PHP_INI_ENTRY("ctpp.max_functions",        "1024",    PHP_INI_ALL, OnUpdateLong, max_functions,   zend_ctpp2_globals, ctpp2_globals)
PHP_INI_END()

PHP_MINIT_FUNCTION(ctpp2) {
	REGISTER_INI_ENTRIES();
	
	// bytecode resource
	le_ctpp_bytecode = zend_register_list_destructors_ex(php_ctpp2_destroy_bytecode, NULL, (char *) "CTPP_BYTECODE", module_number);
	
	// CTPP2 class
	init_ctpp2_class();
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(ctpp2) {
	return SUCCESS;
}

PHP_RINIT_FUNCTION(ctpp2) {
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(ctpp2) {
	return SUCCESS;
}

PHP_MINFO_FUNCTION(ctpp2) {
	php_info_print_table_start();
	php_info_print_table_header(2, "CTPP2", "enabled");
	php_info_print_table_end();
}

// CTPP2 Impl
static zend_function_entry php_ctpp2_methods[] = {
	PHP_ME(CTPP2, __construct,			NULL,	ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(CTPP2, setIncludeDirs,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, parseTemplate,		NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, parseText,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, saveBytecode,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, dumpBytecode,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, freeBytecode,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, loadBytecode,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, loadBytecodeString,	NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, param,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, reset,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, json,					NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, output,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, display,				NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, dump,					NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, getLastError,			NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, bind,					NULL,	ZEND_ACC_PUBLIC)
	PHP_ME(CTPP2, unbind,				NULL,	ZEND_ACC_PUBLIC)
	PHP_FE_END
};

static void php_ctpp2_free_storage(zend_object *object TSRMLS_DC) {
	php_ctpp2_object *obj = Z_FETCH_CUSTOM_OBJ(php_ctpp2_object, object);
	zend_object_std_dtor(&obj->std);
	if (obj->ctpp) {
		delete obj->ctpp;
		obj->ctpp = NULL;
	}
}

static zend_object *php_ctpp2_create_handler(zend_class_entry *ce TSRMLS_DC) {
	php_ctpp2_object *intern = (php_ctpp2_object *) ecalloc(1, sizeof(struct php_ctpp2_object) + zend_object_properties_size(ce));
	zend_object_std_init(&intern->std, ce);
	object_properties_init(&intern->std, ce);
	intern->ctpp = NULL;
	intern->std.handlers = &php_ctpp2_object_handlers;
	return &intern->std;
}

static void init_ctpp2_class() {
	zend_class_entry ce;
	memcpy(&php_ctpp2_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	INIT_CLASS_ENTRY(ce, CTPP2_CLASS_NAME, php_ctpp2_methods);
	ce.create_object = php_ctpp2_create_handler;
	php_ctpp2_object_handlers.offset = XtOffsetOf(php_ctpp2_object, std);
	php_ctpp2_object_handlers.free_obj = php_ctpp2_free_storage;
	php_ctpp2_object_handlers.clone_obj = NULL;
	php_ctpp2_ce = zend_register_internal_class(&ce);
}

// Конструктор
PHP_METHOD(CTPP2, __construct) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	const char *charset_src = "", *charset_dst = "";
	
	unsigned int arg_stack_size = CTPP_G(arg_stack_size), 
		code_stack_size = CTPP_G(code_stack_size), 
		steps_limit = CTPP_G(steps_limit), 
		max_functions = CTPP_G(max_functions);
	
	zval *args = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &args) != SUCCESS)
		WRONG_PARAM_COUNT; 
	
	if (args&&0) {
		::HashTable *hash = Z_ARRVAL_P(args);
		
		ulong num_key; zend_string *key; zval *val;
		ZEND_HASH_FOREACH_KEY_VAL(hash, num_key, key, val) {
			if (key) {
				if (!strcmp(key->val, "arg_stack_size")) {
					if (Z_TYPE_P(val) != IS_LONG)
						convert_to_long(val);
					arg_stack_size = Z_LVAL_P(val);
				} else if (!strcmp(key->val, "code_stack_size")) {
					if (Z_TYPE_P(val) != IS_LONG)
						convert_to_long(val);
					code_stack_size = Z_LVAL_P(val);
				} else if (!strcmp(key->val, "steps_limit")) {
					if (Z_TYPE_P(val) != IS_LONG)
						convert_to_long(val);
					steps_limit = Z_LVAL_P(val);
				} else if (!strcmp(key->val, "max_functions")) {
					if (Z_TYPE_P(val) != IS_LONG)
						convert_to_long(val);
					max_functions = Z_LVAL_P(val);
				} else if (!strcmp(key->val, "charset_dst")) {
					if (Z_TYPE_P(val) != IS_STRING)
						convert_to_string(val);
					charset_dst = Z_STRVAL_P(val);
				} else if (!strcmp(key->val, "charset_src")) {
					if (Z_TYPE_P(val) != IS_STRING)
						convert_to_string(val);
					charset_src = Z_STRVAL_P(val);
				}
			}
		} ZEND_HASH_FOREACH_END();
	}
		
	obj->ctpp = new CTPP2(
		arg_stack_size, 
		code_stack_size, 
		steps_limit, 
		max_functions, 
		charset_src, 
		charset_dst
	);
}

// Задать директории поиска шаблонов
PHP_METHOD(CTPP2, setIncludeDirs) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	zval *dirs;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &dirs) != SUCCESS)
		WRONG_PARAM_COUNT;
	obj->ctpp->setIncludeDirs(Z_ARRVAL_P(dirs));
	RETURN_ZVAL(object, true, false);
}

// Распарсить шаблон из строки
PHP_METHOD(CTPP2, parseText) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	zval *source;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &source) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = obj->ctpp->parse(source, "direct source", Bytecode::T_TEXT_SOURCE);
	if (b) {
		ZEND_REGISTER_RESOURCE(return_value, b, le_ctpp_bytecode);
	} else {
		RETURN_NULL();
	}
}

// Распарсить шаблон из файла
PHP_METHOD(CTPP2, parseTemplate) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	zend_string *filename;;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &filename) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = obj->ctpp->parse(NULL, filename->val, Bytecode::T_SOURCE);
	if (b) {
		ZEND_REGISTER_RESOURCE(return_value, b, le_ctpp_bytecode);
	} else {
		RETURN_NULL();
	}
}

// Загрузить байткод из файла
PHP_METHOD(CTPP2, loadBytecode) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	zend_string *filename;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &filename) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = obj->ctpp->parse(NULL, filename->val, Bytecode::T_BYTECODE);
	if (b) {
		ZEND_REGISTER_RESOURCE(return_value, b, le_ctpp_bytecode);
	} else {
		RETURN_NULL();
	}
}

// Загрузить байткод из строки
PHP_METHOD(CTPP2, loadBytecodeString) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	zval *source;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &source) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = obj->ctpp->parse(source, "direct bytecode", Bytecode::T_TEXT_BYTECODE);
	if (b) {
		ZEND_REGISTER_RESOURCE(return_value, b, le_ctpp_bytecode);
	} else {
		RETURN_NULL();
	}
}

// Отрендерить шаблон и вернуть в виде строки
PHP_METHOD(CTPP2, output) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	zend_string *charset_src = NULL, *charset_dst = NULL;
	zval *bytecode;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|SS", &bytecode, &charset_src, &charset_dst) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = NULL;
	ZEND_FETCH_RESOURCE(b, Bytecode *, &bytecode, -1, (char *) "CTPP_BYTECODE", le_ctpp_bytecode);
	obj->ctpp->output(return_value, b, charset_src ? charset_src->val : NULL, charset_dst ? charset_dst->val : NULL);
}

// Отрендерить шаблон и вывести в поток
PHP_METHOD(CTPP2, display) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	zend_string *charset_src = NULL, *charset_dst = NULL;
	zval *bytecode;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|SS", &bytecode, &charset_src, &charset_dst) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = NULL;
	ZEND_FETCH_RESOURCE(b, Bytecode *, &bytecode, -1, (char *) "CTPP_BYTECODE", le_ctpp_bytecode);
	obj->ctpp->output(NULL, b, charset_src ? charset_src->val : NULL, charset_dst ? charset_dst->val : NULL);
}

// Задампить текущие переменные
PHP_METHOD(CTPP2, dump) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	obj->ctpp->dumpParams(return_value);
}

// Задампить последнюю ошибку
PHP_METHOD(CTPP2, getLastError) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	obj->ctpp->getLastError(return_value);
}

// Задать параметры из JSON строки
PHP_METHOD(CTPP2, json) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	zend_string *json;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &json) != SUCCESS)
		WRONG_PARAM_COUNT;
	obj->ctpp->json(json->val, json->len);
	RETURN_ZVAL(object, true, false);
}

// Задать параметры из хэша
PHP_METHOD(CTPP2, param) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	zval *params;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &params) != SUCCESS)
		WRONG_PARAM_COUNT;
	obj->ctpp->param(params);
	RETURN_ZVAL(object, true, false);
}

// Сбросить текущие параметры
PHP_METHOD(CTPP2, reset) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	obj->ctpp->reset();
	RETURN_ZVAL(object, true, false);
}

// Забиндить функцию в CTPP2
PHP_METHOD(CTPP2, bind) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	zval *callback;
	zend_string *function_name;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Sz", &function_name, &callback) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	if (!zend_is_callable(callback, 0, NULL TSRMLS_CC)) {
		php_error(E_WARNING, "CTPP2: is not callable!");
	} else {
		obj->ctpp->bind(function_name->val, callback);
	}
	RETURN_ZVAL(object, true, false);
}

// Удалить функцию в CTPP2
PHP_METHOD(CTPP2, unbind) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	zend_string *function_name;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &function_name) != SUCCESS)
		WRONG_PARAM_COUNT;
	obj->ctpp->unbind(function_name->val);
	RETURN_ZVAL(object, true, false);
}

// Задампить байткод в строку
PHP_METHOD(CTPP2, saveBytecode) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	zval *bytecode;
	zend_string *path;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rS", &bytecode, &path) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = NULL;
	ZEND_FETCH_RESOURCE(b, Bytecode *, &bytecode, -1, (char *) "CTPP_BYTECODE", le_ctpp_bytecode);
	if (b && b->check()) {
		if (php_check_open_basedir(path->val TSRMLS_CC)) {
			php_error(E_WARNING, "CTPP2::%s(): open_basedir restriction.", get_active_function_name(TSRMLS_C));
			RETURN_BOOL(0);
		} else {
			b->save(path->val);
			RETURN_BOOL(1);
		}
	} else {
		php_error(E_WARNING, "CTPP2::%s(): invalid bytecode!", get_active_function_name(TSRMLS_C));
		RETURN_BOOL(0);
	}
}

// Сохранить байткод в файл
PHP_METHOD(CTPP2, dumpBytecode) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	zval *bytecode;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &bytecode) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = NULL;
	ZEND_FETCH_RESOURCE(b, Bytecode *, &bytecode, -1, (char *) "CTPP_BYTECODE", le_ctpp_bytecode);
	if (b && b->check()) {
		RETURN_STRINGL(b->data(), b->length());
	} else {
		php_error(E_WARNING, "CTPP2: invalid bytecode!");
		RETURN_NULL();
	}
}

// Уничтожить байткод
PHP_METHOD(CTPP2, freeBytecode) {
	Z_METHOD_PARAMS(php_ctpp2_object);
	
	zval *bytecode;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &bytecode) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = NULL;
	ZEND_FETCH_RESOURCE(b, Bytecode *, &bytecode, -1, (char *) "CTPP_BYTECODE", le_ctpp_bytecode);
	if (b) {
		b->free();
	} else {
		php_error(E_WARNING, "CTPP2: invalid bytecode!");
		RETURN_NULL();
	}
}

// resource
static ZEND_RSRC_DTOR_FUNC(php_ctpp2_destroy_bytecode) {
	if (res->ptr != NULL) {
		Bytecode *b = (Bytecode *) (res->ptr);
		delete b;
		res->ptr = NULL;
	}
}