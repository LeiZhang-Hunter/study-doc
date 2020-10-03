#rig扩展源码分析

你可能会问rig是什么，如果是虎扑php-fpm重要的监控中间件，在日常监控上报数据指标中具有举足轻重的重要地位。

rig在php的使用中以扩展的形式融入到日常使用中，下面我们分析一下rig的实现过程。

如何分析一个php扩展是如何做的？那肯定是要首先看zend生命周期中的一些钩子函数中都做了什么

比如 分析 global_init、rinit、minit等等

首先看模块初始化函数minit做了什么

```
PHP_MINIT_FUNCTION (rig) {


	ZEND_INIT_MODULE_GLOBALS(rig, php_rig_init_globals, NULL);
	//data_register_hashtable();
	REGISTER_INI_ENTRIES();
	/* If you have INI entries, uncomment these lines
	*/
	if (RIG_G(enable)) {
        if (strcasecmp("cli", sapi_module.name) == 0 && cli_debug == 0) {
            return SUCCESS;
        }

        // 用户自定义函数执行器(php脚本定义的类、函数)
        ori_execute_ex = zend_execute_ex;
        zend_execute_ex = rig_execute_ex;

        // 内部函数执行器(c语言定义的类、函数)
//        ori_execute_internal = zend_execute_internal;
//        zend_execute_internal = rig_execute_internal;

		// bind curl
		zend_function *old_function;
		if ((old_function = zend_hash_str_find_ptr(CG(function_table), "curl_exec", sizeof("curl_exec") - 1)) != NULL) {
			orig_curl_exec = old_function->internal_function.handler;
			old_function->internal_function.handler = rig_curl_exec_handler;
		}


        if ((old_function = zend_hash_str_find_ptr(CG(function_table), "curl_setopt", sizeof("curl_setopt")-1)) != NULL) {
            orig_curl_setopt = old_function->internal_function.handler;
            old_function->internal_function.handler = rig_curl_setopt_handler;
        }

        // 批量设置URL和相应的选项
        if ((old_function = zend_hash_str_find_ptr(CG(function_table), "curl_setopt_array", sizeof("curl_setopt_array")-1)) != NULL) {
            orig_curl_setopt_array = old_function->internal_function.handler;
            old_function->internal_function.handler = rig_curl_setopt_array_handler;
        }

        //关闭 cURL 会话并且释放所有资源。cURL 句柄 ch 也会被删除。
        if ((old_function = zend_hash_str_find_ptr(CG(function_table), "curl_close", sizeof("curl_close")-1)) != NULL) {
            orig_curl_close = old_function->internal_function.handler;
            old_function->internal_function.handler = rig_curl_close_handler;
        }
	}

	return SUCCESS;
}
```

初始化了 php_rig_init_globals 这个全局的变量，如果说在全局范围内开启了rig，而且只能在fpm下运行，我们可以看到一个很关键的操作，hook掉了zend虚拟机的执行，hook掉了zend的核心执行函数 zend_execute

	// 用户自定义函数执行器(php脚本定义的类、函数)
        ori_execute_ex = zend_execute_ex;
        zend_execute_ex = rig_execute_ex;

那么我们还需要再看rig_execute_ex 里到底做了什么？其实这项技术我最早是在xhprof看到的，这已经不是什么新技术了

```
ZEND_API void rig_execute_ex(zend_execute_data *execute_data) {
    if (application_instance == 0 || rig_enable==0) {
        ori_execute_ex(execute_data);
        return;
    }

    zend_function *zf = execute_data->func;
    const char *class_name = (zf->common.scope != NULL && zf->common.scope->name != NULL) ? ZSTR_VAL(
            zf->common.scope->name) : NULL;
    const char *function_name = zf->common.function_name == NULL ? NULL : ZSTR_VAL(zf->common.function_name);


    // char *str =NULL;
    // spprintf(&str, 0, "php 拦截点：%s://%s", class_name, function_name);
    // test_log(str);


    if (class_name != NULL) {
        if (strcmp(class_name, "Monolog\\Logger") == 0 && ( strcmp(function_name, "info") == 0 || strcmp(function_name, "warn") == 0 || strcmp(function_name, "debug") == 0)) {
            //params
            zval *p = ZEND_CALL_ARG(execute_data, 1);
            zend_string *pstr=zval_get_string(p);
            if(pstr!=NULL && startsWith(ZSTR_VAL(pstr),log_metrics_prefix)==0){
                save_metrics_log(ZSTR_VAL(pstr));
            }
       } else if (strcmp(class_name, "Psr\\Log\\AbstractLogger") == 0 && ( strcmp(function_name, "info") == 0 || strcmp(function_name, "warn") == 0 || strcmp(function_name, "debug") == 0)) {
            // params
            zval *p = ZEND_CALL_ARG(execute_data, 1);
            zend_string *pstr = zval_get_string(p);
            if(pstr!=NULL && startsWith(ZSTR_VAL(pstr),log_metrics_prefix)==0){
                save_metrics_log(ZSTR_VAL(pstr));
            }
       }
    }
    ori_execute_ex(execute_data);
}
```

在这里获取到当前运行的类名和函数，如果判断是类名Monolog\\Logger，或者是info、warn、debug等函数名字，那么获取到调用传入的协议内容，然后调用save_metrics_log，也就是说一个最基本核心的思想就是每次zend虚拟机执行zend_execute_ex都回去判断是否有调用Monolog\\Logger或者Psr\\Log\\AbstractLogger，然后调用save_metrics_log进行数据上报。

好了我们看完了hook虚拟机的部分源码，再继续向下看

```
	// bind curl
	zend_function *old_function;
	if ((old_function = zend_hash_str_find_ptr(CG(function_table), "curl_exec", sizeof("curl_exec") - 1)) != NULL) {
		orig_curl_exec = old_function->internal_function.handler;
		old_function->internal_function.handler = rig_curl_exec_handler;
	}


        if ((old_function = zend_hash_str_find_ptr(CG(function_table), "curl_setopt", sizeof("curl_setopt")-1)) != NULL) {
            orig_curl_setopt = old_function->internal_function.handler;
            old_function->internal_function.handler = rig_curl_setopt_handler;
        }

        // 批量设置URL和相应的选项
        if ((old_function = zend_hash_str_find_ptr(CG(function_table), "curl_setopt_array", sizeof("curl_setopt_array")-1)) != NULL) {
            orig_curl_setopt_array = old_function->internal_function.handler;
            old_function->internal_function.handler = rig_curl_setopt_array_handler;
        }

        //关闭 cURL 会话并且释放所有资源。cURL 句柄 ch 也会被删除。
        if ((old_function = zend_hash_str_find_ptr(CG(function_table), "curl_close", sizeof("curl_close")-1)) != NULL) {
            orig_curl_close = old_function->internal_function.handler;
            old_function->internal_function.handler = rig_curl_close_handler;
        }
```

这里其实是hook runtime上的curl，当然这些技术也早就不是什么新技术了，当年在sff扩展中我也用到了这种技术。

先看第一处

```

		zend_function *old_function;
		if ((old_function = zend_hash_str_find_ptr(CG(function_table), "curl_exec", sizeof("curl_exec") - 1)) != NULL) {
			orig_curl_exec = old_function->internal_function.handler;
			old_function->internal_function.handler = rig_curl_exec_handler;
		}
```

太显而易见我就不说了 直接hook了 old_function->internal_function.handler，zend虚拟机有过了解的都知道zend虚拟机执行过程是一个大的while循环，从头开始挨个执行handler，我们主要应该分析 4个 hook里面做了什么 rig_curl_exec_handler、rig_curl_setopt_handler、rig_curl_setopt_array_handler、rig_curl_close_handler

首先要我们看rig_curl_exec_handler

```
void rig_curl_exec_handler(INTERNAL_FUNCTION_PARAMETERS)
{

    if(application_instance == 0 || rig_enable==0) {
        orig_curl_exec(INTERNAL_FUNCTION_PARAM_PASSTHRU);
        return;
    }

	zval  *zid;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &zid) == FAILURE) {
		return;
	}

	int is_send = 1;

    zval function_name,curlInfo;
    zval params[1];
    ZVAL_COPY(&params[0], zid);
    ZVAL_STRING(&function_name,  "curl_getinfo");
    call_user_function(CG(function_table), NULL, &function_name, &curlInfo, 1, params);
    zval_dtor(&function_name);
    zval_dtor(&params[0]);

    zval *z_url = zend_hash_str_find(Z_ARRVAL(curlInfo),  ZEND_STRL("url"));
    if(z_url==NULL || strlen(Z_STRVAL_P(z_url)) <= 0) {
        zval_dtor(&curlInfo);
        is_send = 0;
    }

    char *url_str = Z_STRVAL_P(z_url);
    php_url *url_info = NULL;
    if(is_send == 1) {
        url_info = php_url_parse(url_str);
        if(url_info->scheme == NULL || url_info->host == NULL) {
            zval_dtor(&curlInfo);
            php_url_free(url_info);
            is_send = 0;
        }
    }

    char *peer = NULL;
    char *full_url = NULL;


    if (is_send == 1) {

// for php7.3.0+
#if PHP_VERSION_ID >= 70300
        char *php_url_scheme = ZSTR_VAL(url_info->scheme);
        char *php_url_host = ZSTR_VAL(url_info->host);
        char *php_url_path = ZSTR_VAL(url_info->path);
        char *php_url_query = ZSTR_VAL(url_info->query);
#else
        char *php_url_scheme = url_info->scheme;
        char *php_url_host = url_info->host;
        char *php_url_path = url_info->path;
        char *php_url_query = url_info->query;
#endif

        int peer_port = 0;
        if (url_info->port) {
            peer_port = url_info->port;
        } else {
            if (strcasecmp("http", php_url_scheme) == 0) {
                peer_port = 80;
            } else {
                peer_port = 443;
            }
        }

        if (url_info->query) {
            if (url_info->path == NULL) {
                spprintf(&full_url, 0, "%s?%s", "/", php_url_query);
            } else {
                spprintf(&full_url, 0, "%s?%s", php_url_path, php_url_query);
            }
        } else {
            if (url_info->path == NULL) {
                spprintf(&full_url, 0, "%s", "/");
            } else {
                spprintf(&full_url, 0, "%s", php_url_path);
            }
        }

        spprintf(&peer, 0, "%s:%d", php_url_host, peer_port);
    }

    zval curl_upstream;
    array_init(&curl_upstream);

    add_assoc_long(&curl_upstream, "application_instance", application_instance);
   // add_assoc_stringl(&curl_upstream, "uuid", application_uuid, strlen(application_uuid));
    add_assoc_long(&curl_upstream, "pid", getppid());
    add_assoc_long(&curl_upstream, "application_id", application_id);
    add_assoc_string(&curl_upstream, "version", RIG_G(version));
    add_assoc_bool(&curl_upstream, "isEntry", 0);
	//SKY_ADD_ASSOC_ZVAL(&curl_upstream, "body");

    zval curl_upstream_body;
    array_init(&curl_upstream_body);

    char *l_millisecond;
    long millisecond;
    if(is_send == 1) {
        l_millisecond = get_millisecond();
        millisecond = zend_atol(l_millisecond, strlen(l_millisecond));
        efree(l_millisecond);
        add_assoc_long(&curl_upstream_body, "startTime", millisecond);
    }

	 orig_curl_exec(execute_data, return_value);
	 if(return_value!=NULL){
         zend_string *result_string;
         result_string = zval_get_string(return_value);
         if(result_string!=NULL){
             add_assoc_long(&curl_upstream_body, "responseSize", ZSTR_LEN(result_string));
             // zend_string_free(result_string);
         }
	 }

     zval *http_method = zend_hash_index_find(Z_ARRVAL_P(&RIG_G(curl_header)), Z_RES_HANDLE_P(zid));
     if( http_method == NULL){
        add_assoc_string(&curl_upstream_body, "method", "GET");
     }else{
        add_assoc_string(&curl_upstream_body, "method", Z_STRVAL_P(http_method));
     }

    if (is_send == 1) {
        zval function_name_1, curlInfo_1;
        zval params_1[1];
        ZVAL_COPY(&params_1[0], zid);
        ZVAL_STRING(&function_name_1, "curl_getinfo");
        call_user_function(CG(function_table), NULL, &function_name_1, &curlInfo_1, 1, params_1);
        zval_dtor(&params_1[0]);
        zval_dtor(&function_name_1);

        l_millisecond = get_millisecond();
        millisecond = zend_atol(l_millisecond, strlen(l_millisecond));
        efree(l_millisecond);

        zval *z_http_code;
        z_http_code = zend_hash_str_find(Z_ARRVAL(curlInfo_1), ZEND_STRL("http_code"));
        if(z_http_code!=NULL){
           add_assoc_long(&curl_upstream_body, "responseCode", Z_LVAL_P(z_http_code));
        }


        char *path = (char*)emalloc(strlen(full_url) + 5);
        bzero(path, strlen(full_url) + 5);

        int i;
        for(i = 0; i < strlen(full_url); i++) {
            if (full_url[i] == '?') {
                break;
            }
            path[i] = full_url[i];
        }
        path[i] = '\0';

        add_assoc_string(&curl_upstream_body, "path", path);
        efree(path);


        add_assoc_long(&curl_upstream_body, "endTime", millisecond);
        add_assoc_string(&curl_upstream_body, "peer", peer);
        add_assoc_zval(&curl_upstream, "body", &curl_upstream_body);

        write_log(rig_json_encode(&curl_upstream),1,1);

        zval *http_header = zend_hash_index_find(Z_ARRVAL_P(&RIG_G(curl_header)), Z_RES_HANDLE_P(zid));
        if (http_header != NULL) {
            zend_hash_index_del(Z_ARRVAL_P(&RIG_G(curl_header)), Z_RES_HANDLE_P(zid));
        }

        efree(peer);
        efree(full_url);

        php_url_free(url_info);
        zval_ptr_dtor(&curlInfo_1);
        zval_ptr_dtor(&curlInfo);

        zval_ptr_dtor(&curl_upstream);

    }
}
```
获取到参数 

```
	zval  *zid;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &zid) == FAILURE) {
		return;
	}

	int is_send = 1;

```

这个参数就是curl_exec的参数，就是一个curl资源类型句柄，然后拿到这个参数调用curl_getinfo去获取curl信息，然后将这些信息打包到一个curl_upstream中，压缩成json，进行write_log调用，在这里我们其实会有一个疑问save_metrics_log 和 writelog的区别，我们后面会继续看rig_curl_setopt_handler，当然为了保证程序正常执行不被打扰，还要调用orig_curl_exec(execute_data, return_value);

```
void rig_curl_setopt_handler(INTERNAL_FUNCTION_PARAMETERS) {
    if(application_instance == 0 || rig_enable==0) {
        orig_curl_setopt(INTERNAL_FUNCTION_PARAM_PASSTHRU);
        return;
    }

    zval *zid, *zvalue;
    zend_long options;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "rlz", &zid, &options, &zvalue) == FAILURE) {
        return;
    }


   if(zid !=NULL && options!=NULL){
        if(CURLOPT_CUSTOMREQUEST == options){
           if(zvalue!=NULL){
               add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), Z_STRVAL_P(zvalue));
           }
        }else{
            zend_long value = zvalue==NULL ? 0 :zval_get_long(zvalue);
                if(value == 0 ){
                    add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), "GET");
                }else{
                    if(CURLOPT_POST == options){
                        add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), "POST");
                    }else if(CURLOPT_HTTPGET == options){
                        add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), "GET");
                    }else if(CURLOPT_PUT == options){
                        add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), "PUT");
                    }else if(CURLOPT_HEADER == options){
                        add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), "HEADER");
                    }
                }
        }
    }

    orig_curl_setopt(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    return;
}
```

放到了 全局变量RIG_G(curl_header)中，至于放到这里有什么用呢？我们要继续看rig_curl_setopt_handler了，rig_curl_setopt_array_handler是 curl_setopt的hook函数，看一下他的功效


```
void rig_curl_setopt_array_handler(INTERNAL_FUNCTION_PARAMETERS) {

    if(application_instance == 0 || rig_enable==0) {
        orig_curl_setopt_array(INTERNAL_FUNCTION_PARAM_PASSTHRU);
        return;
    }

    zval *zid, *arr, *entry;
    zend_ulong option;
    zend_string *string_key;

    ZEND_PARSE_PARAMETERS_START(2, 2)
            Z_PARAM_RESOURCE(zid)
            Z_PARAM_ARRAY(arr)
    ZEND_PARSE_PARAMETERS_END();

  if( zend_hash_index_find(Z_ARRVAL_P(arr), CURLOPT_POST )!= NULL){
       add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), "POST");
    }else if( zend_hash_index_find(Z_ARRVAL_P(arr), CURLOPT_HTTPGET ) != NULL){
       add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), "GET");
    }else if( zend_hash_index_find(Z_ARRVAL_P(arr), CURLOPT_PUT) != NULL){
       add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), "PUT");
    }else if( zend_hash_index_find(Z_ARRVAL_P(arr), CURLOPT_HEADER) != NULL){
       add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), "HEADER");
    }else if( zend_hash_index_find(Z_ARRVAL_P(arr), CURLOPT_CUSTOMREQUEST) != NULL){
         zval *zvalue=zend_hash_index_find(Z_ARRVAL_P(arr), CURLOPT_CUSTOMREQUEST);
         if(zvalue!=NULL){
             add_index_string(&RIG_G(curl_header), Z_RES_HANDLE_P(zid), Z_STRVAL_P(zvalue));
         }

    }
    orig_curl_setopt_array(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
```
还是主要是将一些关键信息记录到RIG_G(curl_header)，hook主要是为了记录 http请求的方法get、post、put、delete等等。

RIG_G(curl_header)主要是为了在curl_exec的事时候也就是调用rig_curl_exec_handler时候做上报的 


我们再看 save_metrics_log

```
void save_metrics_log(char *log){
    write_log(log,2,0);

}
```

发现其实他就是writelog，那继续跟进去看writelog

```
static void write_log(char *text,int prefix, int isFree) {
    if (application_instance != 0  && rig_enable==1) {
        // to stream
        if(text == NULL || strlen(text) <= 0) {
            return;
        }

        struct sockaddr_un un;
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, RIG_G(sock_path));
        int fd;
        char *message = (char*) emalloc(strlen(text) + 10);
        bzero(message, strlen(text) + 10);

        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0) {
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
            int conn = connect(fd, (struct sockaddr *) &un, sizeof(un));

            if (conn >= 0) {
                sprintf(message, "%d%s\n", prefix,text);
                write(fd, message, strlen(message));
                //test_log(message);
            }
            close(fd);
        }
        efree(message);
        if(isFree==1){
            efree(text);
        }

    }

}
```

其实发现这就是建立一个unix套接字 send过去，其实我不大认同这种做法，因为每次调用writelog都会频繁创建销毁描述符，但是这有一个好处就是没有粘包的困扰，开发难度会下降很多

MINIT一系列分析完了，我们再分析一波RINIT，看看请求初始化的事后扩展做了什么？

只看比较核心的几行

```
	rig_register()
	
	
        request_init();

        php_output_handler *handler;
        handler = php_output_handler_create_internal("myext handler", sizeof("myext handler") -1, rig_output_handler, /* PHP_OUTPUT_HANDLER_DEFAULT_SIZE */ 2048, PHP_OUTPUT_HANDLER_STDFLAGS);
        php_output_handler_start(handler);
```

初始化请求，并且打开输出缓冲区，最后我们再看我们程序中节点上报的函数，其中有一个比较关键的函数rig_register，这个是用来注册rig的

```
static int rig_register( ) {

        int instance_id=0;
        struct sockaddr_un un;
        un.sun_family = AF_UNIX;
        // rig.sock_path=/var/run/rig-agent.sock 通讯
        strcpy(un.sun_path, RIG_G(sock_path));
        int fd;
        char message[4096];
        char return_message[4096];

        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0) {
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
            int conn = connect(fd, (struct sockaddr *) &un, sizeof(un));

            if (conn >= 0) {
                bzero(message, sizeof(message));
                 char *uri = get_page_request_uri();
                sprintf(message, "0{\"app_code\":\"%s\",\"pid\":%d,\"version\":\"%s\",\"php_version\":\"%s\",\"url\":\"%s\"}\n", RIG_G(app_code),
                        getppid(), RIG_G(version),PHP_VERSION,uri);
                write(fd, message, strlen(message));

                bzero(return_message, sizeof(return_message));
                read(fd, return_message, sizeof(return_message));

                 if (uri != NULL) {
                       efree(uri);
                  }

                char *ids[10] = {0};
                int i = 0;
               // C 库函数 char *strtok(char *str, const char *delim) 分解字符串 str 为一组字符串，delim 为分隔符。
                char *p = strtok(return_message, ",");

                while (p != NULL) {
                    ids[i++] = p;
                    p = strtok(NULL, ",");
                }

                if (ids[0] != NULL && ids[1] != NULL && ids[2] != NULL) {
                    application_id = atoi(ids[0]);
                    instance_id = atoi(ids[1]);
                   // strncpy(application_uuid, ids[2], sizeof application_uuid - 1);
                }
            }

            close(fd);
        }

        return instance_id;

}
```

埋点程序太简单了，直接从内存中，通过域套接字上报到rig

```
PHP_FUNCTION(rig_biz_metrics)
{

    zval *res=NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &res) == FAILURE) {
        return;
    }

   save_metrics_log(Z_STRVAL_P(res));

   RETURN_TRUE;
}
```


