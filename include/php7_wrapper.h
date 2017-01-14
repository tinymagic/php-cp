#ifndef EXT_POOL_PHP7_WRAPPER_H_
#define EXT_POOL_PHP7_WRAPPER_H_

#if PHP_MAJOR_VERSION < 7
#include <ext/standard/php_smart_str.h>
typedef zend_rsrc_list_entry zend_resource;
#define CP_RETURN_STRING                      RETURN_STRING
#define CP_Z_ARRVAL_P                         Z_ARRVAL_P
#define CP_Z_ARRVAL_PP                        Z_ARRVAL_P
#define IS_TRUE                               1
#define cp_add_assoc_string                   add_assoc_string
#define cp_add_index_string                   add_index_string
#define cp_convert_to_string_ex                  convert_to_string_ex

static CPINLINE int cp_zend_hash_find(HashTable *ht, char *k, int len, void **v) {
    zval **tmp = NULL;
    if (zend_hash_find(ht, k, len, (void **) &tmp) == SUCCESS) {
        *v = *tmp;
        return SUCCESS;
    } else {
        *v = NULL;
        return FAILURE;
    }
}

static CPINLINE int cp_zend_hash_index_find(HashTable *ht, zend_ulong h, void **v) {
    zval **tmp = NULL;
    if (zend_hash_index_find(ht, h, (void **) &tmp) == SUCCESS) {
        *v = *tmp;
        return SUCCESS;
    } else {
        *v = NULL;
        return FAILURE;
    }
}

#define CP_INTERNAL_SEND_RAW(send_data,type)\
                                zval send_zval = {0};\
                                CP_ZVAL_STRING(&send_zval,send_data,0);\
                                CP_INTERNAL_SERIALIZE_SEND_MEM(&send_zval,type);

#define cp_zend_hash_del                      zend_hash_del
#define cp_zend_hash_update                   zend_hash_update
#define cp_zend_hash_add                      zend_hash_add
#define cp_zend_hash_index_update             zend_hash_index_update
#define cp_zend_hash_copy                     zend_hash_copy
#define cp_zend_hash_exists                   zend_hash_exists
#define cp_zend_hash_get_current_key(a,b,c,d)  zend_hash_get_current_key_ex(a,b,c,d,0,NULL)

#define cp_zend_read_property                  zend_read_property

#define CP_ZVAL_STRING                        ZVAL_STRING
#define CP_ZVAL_STRINGL                       ZVAL_STRINGL
#define CP_ZEND_FETCH_RESOURCE_NO_RETURN      ZEND_FETCH_RESOURCE_NO_RETURN
#define CP_ZEND_FETCH_RESOURCE                ZEND_FETCH_RESOURCE
#define CP_ZEND_REGISTER_RESOURCE             ZEND_REGISTER_RESOURCE
#define CP_MAKE_STD_ZVAL(p)                   MAKE_STD_ZVAL(p)
#define CP_ALLOC_INIT_ZVAL(p)                 ALLOC_INIT_ZVAL(p)
#define CP_RETVAL_STRINGL                     RETVAL_STRINGL
#define cp_smart_str                          smart_str
#define cp_php_var_unserialize                php_var_unserialize
#define cp_zend_is_callable                   zend_is_callable
#define cp_call_user_function_ex              call_user_function_ex
#define cp_add_assoc_stringl_ex               add_assoc_stringl_ex
#define cp_add_assoc_stringl                  add_assoc_stringl
#define cp_zval_ptr_dtor                      zval_ptr_dtor
#define cp_zval_add_ref                       zval_add_ref
#define cp_strndup(v,l)                       estrndup(Z_STRVAL_P(v),l)
#define CP_RETURN_STRINGL                     RETURN_STRINGL
#define cp_explode                            php_explode
#define cp_zend_register_internal_class_ex    zend_register_internal_class_ex
#define cp_zend_fetch_class(zval,type)        zend_fetch_class(Z_STRVAL_P(zval),Z_STRLEN_P(zval),type)

#define cp_zend_call_method_with_0_params     zend_call_method_with_0_params
#define cp_zend_call_method_with_1_params     zend_call_method_with_1_params
#define cp_zend_call_method_with_2_params     zend_call_method_with_2_params

#define ZVAL_DUP(z,v)                                 *z = *v;zval_copy_ctor(z);

typedef int zend_size_t;

#define CP_HASHTABLE_FOREACH_START(ht, entry)\
    zval **tmp = NULL;\
    for (zend_hash_internal_pointer_reset(ht);\
        zend_hash_has_more_elements(ht) == SUCCESS; \
        zend_hash_move_forward(ht)) {\
        if (zend_hash_get_current_data(ht, (void**)&tmp) == FAILURE) {\
            continue;\
        }\
        entry = *tmp;

#if defined(HASH_KEY_NON_EXISTANT) && !defined(HASH_KEY_NON_EXISTENT)
#define HASH_KEY_NON_EXISTENT HASH_KEY_NON_EXISTANT
#endif

#define CP_HASHTABLE_FOREACH_START2(ht, k, klen, ktype, entry)\
    zval **tmp = NULL; ulong idx;\
    for (zend_hash_internal_pointer_reset(ht); \
            (ktype = zend_hash_get_current_key_ex(ht, &k, &klen, &idx, 0, NULL)) != HASH_KEY_NON_EXISTENT; \
            zend_hash_move_forward(ht)\
        ) { \
        if(HASH_KEY_IS_LONG==ktype){\
            char t[20] = {0};\
            sprintf(t,"%d",idx);\
            k = t; klen = strlen(t)+1; \
        }\
    if (zend_hash_get_current_data(ht, (void**)&tmp) == FAILURE) {\
        continue;\
    }\
    entry = *tmp;\
    klen --;

#define CP_HASHTABLE_FOREACH_END() }

static CPINLINE int CP_Z_TYPE_P(zval *z) {
    if (Z_TYPE_P(z) == IS_BOOL) {
        if ((uint8_t) Z_BVAL_P(z) == 1) {
            return IS_TRUE;
        } else {
            return 0;
        }
    } else {
        return Z_TYPE_P(z);
    }
}
#define cp_php_var_serialize(a,b,c)       php_var_serialize(a,&b,c)
#define IS_TRUE    1
inline int CP_Z_TYPE_P(zval *z);
#define CP_Z_TYPE_PP(z)        CP_Z_TYPE_P(*z)
#define CP_SEND_EXCEPTION_ARGS(str) do{   zval *exception = EG(exception);\
                                zend_class_entry *ce_exception = Z_OBJCE_P(exception);\
                                EG(exception) = NULL;\
                                cp_zend_call_method_with_0_params(&exception, ce_exception, NULL, "__tostring", str);\
                                CP_INTERNAL_SERIALIZE_SEND_MEM(*str,CP_SIGEVENT_EXCEPTION);\
                                cp_zval_ptr_dtor(&exception);\
                        }while(0);

#define CP_EXCEPTION_ARGS(str) do{   zval *exception = EG(exception);\
                                zend_class_entry *ce_exception = Z_OBJCE_P(exception);\
                                EG(exception) = NULL;\
                                cp_zend_call_method_with_0_params(&exception, ce_exception, NULL, "__tostring", str);\
                                cp_zval_ptr_dtor(&exception);\
                        }while(0);

#define CP_TEST_RETURN_FALSE(flag) ({if(flag==CP_CONNECT_PING){ \
              if(EG(exception)){ \
                  zval *exception = EG(exception);\
                  zval_ptr_dtor(&exception); \
                  EG(exception) = NULL;\
               }\
              return CP_FALSE; \
          }});

static CPINLINE int cp_internal_call_user_function(zval *object, zval *fun, zval **ret_value, zval * args) {
    zval *m_args;
    int count = 0;
    if (cp_zend_hash_find(Z_ARRVAL_P(args), ZEND_STRS("args"), (void **) &m_args) == SUCCESS) {
        count = zend_hash_num_elements(Z_ARRVAL_P(m_args));
        zval **tmp_pass[count];
        int i = 0;
        for (zend_hash_internal_pointer_reset(Z_ARRVAL_P(m_args)); zend_hash_has_more_elements(Z_ARRVAL_P(m_args)) == SUCCESS; zend_hash_move_forward(Z_ARRVAL_P(m_args))) {
            zval **ppzval;
            zend_hash_get_current_data(Z_ARRVAL_P(m_args), (void**) &ppzval);
            tmp_pass[i] = ppzval;
            i++;
        }
        return call_user_function_ex(NULL, &object, fun, ret_value, count, tmp_pass, 0, NULL TSRMLS_CC);
    } else {
        return call_user_function_ex(NULL, &object, fun, ret_value, count, NULL, 0, NULL TSRMLS_CC);
    }
}

static CPINLINE int cp_zend_hash_find_ptr(HashTable *ht, zval *k, void **ret) {
    return cp_zend_hash_find(ht, Z_STRVAL_P(k), Z_STRLEN_P(k) + 1, ret);
}
#else
//------------下面是php7版本------------------------------------
#include <ext/standard/php_smart_string.h>
#include "zend_smart_str.h"

#define cp_php_var_serialize                php_var_serialize
typedef size_t zend_size_t;
#define CP_RETVAL_STRINGL(s, l,dup)         RETVAL_STRINGL(s,l)
#define ZEND_SET_SYMBOL(ht,str,arr) zend_hash_str_update(ht, str, sizeof(str)-1, arr);

static CPINLINE int Z_BVAL_P(zval *v) {
    if (Z_TYPE_P(v) == IS_TRUE) {
        return 1;
    } else {
        return 0;
    }
}
#define cp_zend_fetch_class(zval,type)                   zend_fetch_class(Z_STR_P(zval),type)

#define cp_add_assoc_stringl(__arg, __key, __str, __length, __duplicate) cp_add_assoc_stringl_ex(__arg, __key, strlen(__key)+1, __str, __length, __duplicate)

static CPINLINE int cp_add_assoc_stringl_ex(zval *arg, const char *key, size_t key_len, char *str, size_t length, int duplicate) {
    key_len--;
    return add_assoc_stringl_ex(arg, key, key_len, str, length);
}

#define CP_Z_ARRVAL_P(z)                          Z_ARRVAL_P(z)

#define CP_HASHTABLE_FOREACH_START(ht, _val) ZEND_HASH_FOREACH_VAL(ht, _val);  {


#define CP_HASHTABLE_FOREACH_START2(ht, k, klen, ktype, _val) zend_string *_foreach_key;\
    zend_ulong knum;\
     ZEND_HASH_FOREACH_KEY_VAL(ht, knum,_foreach_key, _val);\
    if (!_foreach_key) {\
        char t[20] = {0};\
        sprintf(t,"%d",knum);\
        k = t; klen = strlen(t); ktype = 0;}\
    else {k = _foreach_key->val, klen=_foreach_key->len; ktype = 1;} {



#define CP_HASHTABLE_FOREACH_END()                 } ZEND_HASH_FOREACH_END();

#define Z_ARRVAL_PP(s)                             Z_ARRVAL_P(*s)
#define cp_convert_to_string_ex(s)                    convert_to_string_ex(*s)
#define Z_BVAL_PP(s)                               Z_BVAL_P(*s)
#define CP_Z_TYPE_P                                Z_TYPE_P
#define CP_Z_TYPE_PP(s)                            CP_Z_TYPE_P(*s)
#define Z_STRVAL_PP(s)                             Z_STRVAL_P(*s)
#define Z_STRLEN_PP(s)                             Z_STRLEN_P(*s)
#define Z_LVAL_PP(v)                               Z_LVAL_P(*v)
#define cp_strndup(s,l)                            \
        ({zend_string *str = zend_string_copy(Z_STR_P(s));\
        str->val;})

#define cp_zval_add_ref(p)   Z_TRY_ADDREF_P(*p)
#define cp_zval_ptr_dtor(p)  zval_ptr_dtor(*p)

static CPINLINE int cp_call_user_function_ex(HashTable *function_table, zval** object_pp, zval *function_name, zval **retval_ptr_ptr, uint32_t param_count, zval ***params, int no_separation, HashTable* ymbol_table) {
    zval real_params[20];
    int i = 0, ret;
    for (; i < param_count; i++) {
        real_params[i] = **params[i];
    }
    zval phpng_retval;
    *retval_ptr_ptr = &phpng_retval;
    zval *object_p = (object_pp == NULL) ? NULL : *object_pp;
    ret = call_user_function_ex(function_table, object_p, function_name, &phpng_retval, param_count, real_params, no_separation, NULL);
    return ret;
}

#define cp_php_var_unserialize(rval, p, max, var_hash)\
php_var_unserialize(*rval, p, max, var_hash)

#define CP_MAKE_STD_ZVAL(p)    zval _stack_zval_##p; p = &(_stack_zval_##p)

#define CP_RETURN_STRINGL(z,l,t)                      \
               zval key;\
                ZVAL_STRING(&key, z);\
                RETURN_STR(Z_STR(key))

#define CP_ALLOC_INIT_ZVAL(p)        CP_MAKE_STD_ZVAL(p)
#define CP_ZEND_FETCH_RESOURCE_NO_RETURN(rsrc, rsrc_type, passed_id, default_id, resource_type_name, resource_type)        \
        (rsrc = (rsrc_type) zend_fetch_resource(Z_RES_P(*passed_id), resource_type_name, resource_type))
#define CP_ZEND_REGISTER_RESOURCE(return_value, result, le_result) ZVAL_RES(return_value,zend_register_resource(result, le_result))
#define CP_RETURN_STRING(val, duplicate)     RETURN_STRING(val)
#define cp_add_assoc_string(array, key, value, duplicate)   add_assoc_string(array, key, value)
#define cp_add_index_string(array, key, value, duplicate)   add_index_string(array, key, value)
#define cp_zend_hash_copy(target,source,pCopyConstructor,tmp,size) zend_hash_copy(target,source,pCopyConstructor)
#define cp_zend_register_internal_class_ex(entry,parent_ptr,str)    zend_register_internal_class_ex(entry,parent_ptr)

#define cp_zend_call_method_with_0_params(obj, ptr, what, method, retval)           zend_call_method_with_0_params(*obj,ptr,what,method,*retval)
#define cp_zend_call_method_with_1_params(obj, ptr, what, method, retval, v1)           zend_call_method_with_1_params(*obj,ptr,what,method,*retval,v1)
//#define cp_zend_call_method_with_2_params(obj,ptr,what,char,return,name,cb)     zend_call_method_with_2_params(*obj,ptr,what,char,*return,name,cb)
#define sw_zend_call_method_with_2_params(obj, ptr, what, method, retval, name, cb)     zend_call_method_with_2_params(*obj,ptr,what,method,*retval,name,cb)
#define CP_ZVAL_STRINGL(z, s, l, dup)         ZVAL_STRINGL(z, s, l)
#define CP_ZVAL_STRING(z,s,dup)               ZVAL_STRING(z,s)
#define cp_smart_str                          smart_string

static CPINLINE zval* cp_zend_read_property(zend_class_entry *class_ptr, zval *obj, char *s, int len, int silent) {
    zval rv;
    return zend_read_property(class_ptr, obj, s, len, silent, &rv);
}

static CPINLINE int cp_zend_is_callable(zval *cb, int a, char **name) {
    zend_string *key;
    int ret = zend_is_callable(cb, a, &key);
    char *tmp = (char *) emalloc(key->len);
    memcpy(tmp, key->val, key->len);
    *name = tmp;
    return ret;
}

static CPINLINE int cp_zend_hash_del(HashTable *ht, char *k, int len) {
    return zend_hash_str_del(ht, k, len - 1);
}

static CPINLINE int cp_zend_hash_add(HashTable *ht, char *k, int len, void *pData, int datasize, void **pDest) {
    zval **real_p = pData;
    return zend_hash_str_add(ht, k, len - 1, *real_p) ? SUCCESS : FAILURE;
}

static CPINLINE int cp_zend_hash_index_update(HashTable *ht, int key, void *pData, int datasize, void **pDest) {
    zval **real_p = pData;
    return zend_hash_index_update(ht, key, *real_p) ? SUCCESS : FAILURE;
}

static CPINLINE int cp_zend_hash_update(HashTable *ht, char *k, int len, void * val, int size, void *ptr) {
    return zend_hash_str_update(ht, k, len - 1, val) ? SUCCESS : FAILURE;
}

static CPINLINE int cp_zend_hash_get_current_key(HashTable *ht, char **key, uint32_t *keylen, ulong *num) {
    zend_string *_key_ptr;
    int type = zend_hash_get_current_key(ht, &_key_ptr, (zend_ulong*) num);
    *key = _key_ptr->val;
    *keylen = _key_ptr->len;
    return type;
}

static CPINLINE int cp_zend_hash_find(HashTable *ht, char *k, int len, void **v) {
    zval *value = zend_hash_str_find(ht, k, len - 1);

    if (value == NULL) {
        return FAILURE;
    } else {
        *v = (void *) value;
        return SUCCESS;
    }
}

static CPINLINE int cp_zend_hash_find_ptr(HashTable *ht, zval *k, void **ret) {
    *ret = zend_hash_find_ptr(ht, Z_STR_P(k));
//    zval_dtor(k);
    if (*ret == NULL) {
        return FAILURE;
    } else {
        return SUCCESS;
    }
}

static CPINLINE int cp_zend_hash_index_find(HashTable *ht, zend_ulong h, void **v) {
    zval *value = zend_hash_index_find(ht, h);

    if (value == NULL) {
        return FAILURE;
    } else {
        *v = (void *) value;
        return SUCCESS;
    }
}

static CPINLINE int cp_zend_hash_exists(HashTable *ht, char *k, int len) {
    zval key;
    ZVAL_STRING(&key, k);
    zval *value = zend_hash_str_find(ht, k, len - 1);

    if (value == NULL) {
        return FAILURE;
    } else {
        return SUCCESS;
    }
}

static CPINLINE void cp_explode(zval *delim, zval *desc, zval *ex_arr, zend_long limit) {
    const zend_string *str_delim;
    zend_string *str_desc;
    if (EXPECTED(Z_TYPE_P(delim) == IS_STRING)) {
        str_delim = Z_STR_P(delim);
    } else {
        str_delim = zval_get_string(delim);
    }

    if (EXPECTED(Z_TYPE_P(desc) == IS_STRING)) {
        str_desc = Z_STR_P(desc);
    } else {
        str_desc = zval_get_string(desc);
    }
    php_explode(str_delim, str_desc, ex_arr, limit);
}

static CPINLINE int cp_internal_call_user_function(zval *object, zval *fun, zval **ret_value, zval * args) {
    zval *m_args;
    zval real_params[20];
    if (!(*ret_value)) {
        zval phpng_retval;
        *ret_value = &phpng_retval;
    }
    if (cp_zend_hash_find(CP_Z_ARRVAL_P(args), ZEND_STRS("args"), (void **) &m_args) == SUCCESS) {
        int i = 0;
        zval *val;
        CP_HASHTABLE_FOREACH_START(CP_Z_ARRVAL_P(m_args), val)
        real_params[i] = *val;
        i++;
        CP_HASHTABLE_FOREACH_END()
        return call_user_function_ex(NULL, object, fun, *ret_value, i, real_params, 0, NULL TSRMLS_CC);
    } else {
        return call_user_function_ex(NULL, object, fun, *ret_value, 0, NULL, 0, NULL TSRMLS_CC);
    }
}

#define CP_INTERNAL_SEND_RAW(send_data,type)\
                                zval send_zval = {0};\
                                CP_ZVAL_STRING(&send_zval,send_data,0);\
                                CP_INTERNAL_SERIALIZE_SEND_MEM(&send_zval,type);\
                                zval_ptr_dtor(&send_zval);
#define CP_GET_EXCEPTION_STR

#define CP_EXCEPTION_ARGS(str) do{    zend_object *ex = EG(exception);\
                zval exception, tmp, rv,ret;\
                ZVAL_OBJ(&exception, ex);\
                zend_class_entry *ce_exception = Z_OBJCE(exception);\
                zend_call_method_with_0_params(&exception, ce_exception, NULL, "__tostring", &tmp);\
                *str = zend_read_property(ce_exception, &exception, "message", sizeof ("message") - 1, 1, &rv);\
	            ZVAL_STRING(&ret,Z_STRVAL_P(*str));\
                *str = &ret;\
	            zval_ptr_dtor(&exception);\
                zval_ptr_dtor(&tmp);\
                EG(exception) = NULL;\
                        }while(0);

#define CP_SEND_EXCEPTION_ARGS(str) do{   CP_EXCEPTION_ARGS(str);\
			    CP_INTERNAL_SERIALIZE_SEND_MEM(*str,CP_SIGEVENT_EXCEPTION);\
            }while(0);

#define CP_TEST_RETURN_FALSE(flag) ({if(flag==CP_CONNECT_PING){ \
              if(EG(exception)){ \
                  zend_object_std_dtor(EG(exception)); \
                  EG(exception) = NULL;\
               }\
              return CP_FALSE; \
          }});

#endif /* EXT_SWOOLE_PHP7_WRAPPER_H_ */
#endif
