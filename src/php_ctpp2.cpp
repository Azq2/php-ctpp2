#include "php_ctpp2.hpp"

ZEND_DECLARE_MODULE_GLOBALS(ctpp2);

static zend_function_entry functions[] = {
	PHP_FE_END
};

static const zend_module_dep ctpp2_deps[] = {
	{NULL, NULL, NULL}
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

static void php_ctpp2_free_storage(void *object TSRMLS_DC) {
	php_ctpp2_object *obj = (php_ctpp2_object *) object; 
	if (obj->ctpp) {
		delete obj->ctpp;
		obj->ctpp = NULL;
	}
	zend_hash_destroy(obj->std.properties);
	FREE_HASHTABLE(obj->std.properties);
	efree(obj);
}

static zend_object_value php_ctpp2_create_handler(zend_class_entry *type TSRMLS_DC) {
	zend_object_value retval;
	
	php_ctpp2_object *obj = (php_ctpp2_object *) emalloc(sizeof(php_ctpp2_object));
	memset(obj, 0, sizeof(php_ctpp2_object));
	obj->std.ce = type; 
	
	ALLOC_HASHTABLE(obj->std.properties);
	zend_hash_init(obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	
	object_properties_init(&(obj->std), php_ctpp2_ce);
	
	retval.handle   = zend_objects_store_put(obj, NULL, php_ctpp2_free_storage, NULL TSRMLS_CC);
	retval.handlers = &php_ctpp2_object_handlers;
	return retval;
}

static void init_ctpp2_class() {
	zend_class_entry ce;
	
	INIT_CLASS_ENTRY(ce, CTPP2_CLASS_NAME, php_ctpp2_methods);
	php_ctpp2_ce = zend_register_internal_class(&ce TSRMLS_CC);
	php_ctpp2_ce->create_object = php_ctpp2_create_handler;
	memcpy(&php_ctpp2_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	php_ctpp2_object_handlers.clone_obj = NULL;
	php_ctpp2_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;
}

// Конструктор
PHP_METHOD(CTPP2, __construct) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
	const char *charset_src = "", *charset_dst = "";
	
	unsigned int arg_stack_size = CTPP_G(arg_stack_size), 
		code_stack_size = CTPP_G(code_stack_size), 
		steps_limit = CTPP_G(steps_limit), 
		max_functions = CTPP_G(max_functions);
	
	zval **entry, *args = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|a", &args) != SUCCESS)
		WRONG_PARAM_COUNT; 
	
	if (args) {
		::HashTable *hash = Z_ARRVAL_P(args);
		HashPosition pos;
			
		char *key;
		uint klen;
		ulong index;
		
		zend_hash_internal_pointer_reset_ex(hash, &pos);
		while (zend_hash_get_current_data_ex(hash, (void **)&entry, &pos) == SUCCESS) {
			if (zend_hash_get_current_key_ex(hash, &key, &klen, &index, 0, &pos) == HASH_KEY_IS_STRING && Z_TYPE_PP(entry)) {
				if (!strcmp(key, "arg_stack_size")) {
					if (Z_TYPE_PP(entry) != IS_LONG)
						convert_to_long(*entry);
					arg_stack_size = Z_LVAL_PP(entry);
				} else if (!strcmp(key, "code_stack_size")) {
					if (Z_TYPE_PP(entry) != IS_LONG)
						convert_to_long(*entry);
					code_stack_size = Z_LVAL_PP(entry);
				} else if (!strcmp(key, "steps_limit")) {
					if (Z_TYPE_PP(entry) != IS_LONG)
						convert_to_long(*entry);
					steps_limit = Z_LVAL_PP(entry);
				} else if (!strcmp(key, "max_functions")) {
					if (Z_TYPE_PP(entry) != IS_LONG)
						convert_to_long(*entry);
					max_functions = Z_LVAL_PP(entry);
				} else if (!strcmp(key, "charset_dst")) {
					if (Z_TYPE_PP(entry) != IS_STRING)
						convert_to_string(*entry);
					charset_dst = Z_STRVAL_PP(entry);
				} else if (!strcmp(key, "charset_src")) {
					if (Z_TYPE_PP(entry) != IS_STRING)
						convert_to_string(*entry);
					charset_src = Z_STRVAL_PP(entry);
				}
				zend_hash_move_forward_ex(hash, &pos);
			}
		}
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
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	zval *dirs;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &dirs) != SUCCESS)
		WRONG_PARAM_COUNT;
	obj->ctpp->setIncludeDirs(Z_ARRVAL_P(dirs));
	RETURN_ZVAL(object, true, false);
}

// Распарсить шаблон из строки
PHP_METHOD(CTPP2, parseText) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
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
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
	const char *filename; int filename_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = obj->ctpp->parse(NULL, filename, Bytecode::T_SOURCE);
	if (b) {
		ZEND_REGISTER_RESOURCE(return_value, b, le_ctpp_bytecode);
	} else {
		RETURN_NULL();
	}
}

// Загрузить байткод из файла
PHP_METHOD(CTPP2, loadBytecode) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
	const char *filename; int filename_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = obj->ctpp->parse(NULL, filename, Bytecode::T_BYTECODE);
	if (b) {
		ZEND_REGISTER_RESOURCE(return_value, b, le_ctpp_bytecode);
	} else {
		RETURN_NULL();
	}
}

// Загрузить байткод из строки
PHP_METHOD(CTPP2, loadBytecodeString) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
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
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
	int charset_src_len, charset_dst_len;
	const char *charset_src = NULL, *charset_dst = NULL;
	
	zval *bytecode;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|ss", &bytecode, &charset_src, &charset_src_len, &charset_dst, &charset_dst_len) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = NULL;
	ZEND_FETCH_RESOURCE(b, Bytecode *, &bytecode, -1, (char *) "CTPP_BYTECODE", le_ctpp_bytecode);
	obj->ctpp->output(return_value, b, charset_src, charset_dst);
}

// Отрендерить шаблон и вывести в поток
PHP_METHOD(CTPP2, display) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
	int charset_src_len, charset_dst_len;
	const char *charset_src = NULL, *charset_dst = NULL;
	
	zval *bytecode;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|ss", &bytecode, &charset_src, &charset_src_len, &charset_dst, &charset_dst_len) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = NULL;
	ZEND_FETCH_RESOURCE(b, Bytecode *, &bytecode, -1, (char *) "CTPP_BYTECODE", le_ctpp_bytecode);
	obj->ctpp->output(NULL, b, charset_src, charset_dst);
	RETURN_ZVAL(object, true, false);
}

// Задампить текущие переменные
PHP_METHOD(CTPP2, dump) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	obj->ctpp->dumpParams(return_value);
}

// Задампить последнюю ошибку
PHP_METHOD(CTPP2, getLastError) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	obj->ctpp->getLastError(return_value);
}

// Задать параметры из JSON строки
PHP_METHOD(CTPP2, json) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	const char *json; int json_len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &json, &json_len) != SUCCESS)
		WRONG_PARAM_COUNT;
	obj->ctpp->json(json, json_len);
	RETURN_ZVAL(object, true, false);
}

// Задать параметры из хэша
PHP_METHOD(CTPP2, param) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	zval *params;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &params) != SUCCESS)
		WRONG_PARAM_COUNT;
	obj->ctpp->param(params);
	RETURN_ZVAL(object, true, false);
}

// Сбросить текущие параметры
PHP_METHOD(CTPP2, reset) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	obj->ctpp->reset();
	RETURN_ZVAL(object, true, false);
}

// Забиндить функцию в CTPP2
PHP_METHOD(CTPP2, bind) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
	zval *callback;
	int function_name_len;
	const char *function_name;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &function_name, &function_name_len, &callback) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	if (!zend_is_callable(callback, 0, NULL TSRMLS_CC)) {
		php_error(E_WARNING, "CTPP2: is not callable!");
	} else {
		obj->ctpp->bind(function_name, callback);
	}
	RETURN_ZVAL(object, true, false);
}

// Удалить функцию в CTPP2
PHP_METHOD(CTPP2, unbind) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	int function_name_len;
	const char *function_name;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &function_name, &function_name_len) != SUCCESS)
		WRONG_PARAM_COUNT;
	obj->ctpp->unbind(function_name);
	RETURN_ZVAL(object, true, false);
}

// Задампить байткод в строку
PHP_METHOD(CTPP2, saveBytecode) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
	zval *bytecode;
	const char *path; int path_len;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &bytecode, &path, &path_len) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = NULL;
	ZEND_FETCH_RESOURCE(b, Bytecode *, &bytecode, -1, (char *) "CTPP_BYTECODE", le_ctpp_bytecode);
	if (b && b->check()) {
		if (php_check_open_basedir(path TSRMLS_CC)) {
			php_error(E_WARNING, "CTPP2::%s(): open_basedir restriction.", get_active_function_name(TSRMLS_C));
			RETURN_BOOL(0);
		} else {
			b->save(path);
			RETURN_BOOL(1);
		}
	} else {
		php_error(E_WARNING, "CTPP2::%s(): invalid bytecode!", get_active_function_name(TSRMLS_C));
		RETURN_BOOL(0);
	}
}

// Сохранить байткод в файл
PHP_METHOD(CTPP2, dumpBytecode) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
	zval *bytecode;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &bytecode) != SUCCESS)
		WRONG_PARAM_COUNT;
	
	Bytecode *b = NULL;
	ZEND_FETCH_RESOURCE(b, Bytecode *, &bytecode, -1, (char *) "CTPP_BYTECODE", le_ctpp_bytecode);
	if (b && b->check()) {
		RETURN_STRINGL(b->data(), b->length(), 1);
	} else {
		php_error(E_WARNING, "CTPP2: invalid bytecode!");
		RETURN_NULL();
	}
}

// Уничтожить байткод
PHP_METHOD(CTPP2, freeBytecode) {
	zval *object = getThis();
	php_ctpp2_object *obj = (php_ctpp2_object *) zend_object_store_get_object(object TSRMLS_CC);
	
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
	if (rsrc->ptr != NULL) {
		Bytecode *b = (Bytecode *) (rsrc->ptr);
		delete b;
		rsrc->ptr = NULL;
	}
}