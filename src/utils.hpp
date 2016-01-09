#pragma once

#define Z_FETCH_CUSTOM_OBJ(T, obj) ((T *)((char *)(obj) - XtOffsetOf(T, std)))
#define Z_CUSTOM_OBJ(T, obj) Z_FETCH_CUSTOM_OBJ(T, Z_OBJ(obj))
#define Z_CUSTOM_OBJ_P(T, obj) Z_FETCH_CUSTOM_OBJ(T, Z_OBJ_P(obj))
#define Z_METHOD_PARAMS(T) \
	zval *object = getThis(); \
	T *obj = Z_CUSTOM_OBJ_P(T, object);
#define ZEND_REGISTER_RESOURCE(return_value, object, id) \
	ZVAL_RES(return_value, zend_register_resource(object, id));
#define ZEND_FETCH_RESOURCE(obj, T, res, default_id, name, id) \
	obj = ((T) zend_fetch_resource_ex((*(res)), name, id));
#define RETURN_PHP_RESOURCE(b, le) { \
	void *__b = (void *) (b); \
	if (__b) { \
		ZEND_REGISTER_RESOURCE(return_value, __b, le); \
		return; \
	} else { \
		RETURN_NULL(); \
	} \
}
