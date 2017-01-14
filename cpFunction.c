/*
  +----------------------------------------------------------------------+
  | common con pool                                                      |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.0 of the Apache license,    |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.apache.org/licenses/LICENSE-2.0.html                      |
  | If you did not receive a copy of the Apache2.0 license and are unable|
  | to obtain it through the world-wide-web, please send a note to       |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | original: Tianfeng Han    modify:Xinhua Guo  <woshiguo35@sina.com>   |
  +----------------------------------------------------------------------+
 */

#include "php_connect_pool.h"

static char bufr[SW_LOG_BUFFER_SIZE];
static char bufpid[SW_PID_BUFFER_SIZE];

FILE *pid_fn = NULL;
int cp_error;
FILE *cp_log_fn = NULL;

int cpLog_init(char *logfile)
{
    cp_log_fn = fopen(logfile, "a+");
    if (cp_log_fn == NULL)
    {
        return FAILURE;
    }
    if (setvbuf(cp_log_fn, bufr, _IOLBF, SW_LOG_BUFFER_SIZE) < 0)
    {
        return FAILURE;
    }
    return SUCCESS;
}

int pid_init()
{
    char pid_name[512] = {0};
    sprintf(pid_name, "%s.pid", PID_FILE_PATH);
    pid_fn = fopen(pid_name, "w+");
    if (pid_fn == NULL)
    {
        cpLog("create pid file error");
        return FAILURE;
    }
    if (setvbuf(pid_fn, bufpid, _IONBF, SW_PID_BUFFER_SIZE) < 0)
    {
        return FAILURE;
    }
    return SUCCESS;
}

int set_pid(int pid)
{
    fprintf(pid_fn, "%d\n", pid);
    return SUCCESS;
}

int cpWrite(int fd, void *buf, int count)
{
    int nwritten = 0, totlen = 0;
    while (totlen != count)
    {
        nwritten = write(fd, buf, count - totlen);
        if (nwritten > 0)
        {
            totlen += nwritten;
            buf += nwritten;
        }
        else if (nwritten == 0)
        {
            return totlen;
        }
        else
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN)
            {
                usleep(1);
                continue;
            }
            else
            {
                return -1;
            }
        }

    }
    return totlen;
}

int cpFifoRead(int pipe_fd_read, void *buf, int len)
{
    int n, total = 0;
    do
    {
        n = read(pipe_fd_read, buf + total, len);
        if (n > 0)
        {
            total += n;
            if (total == len)
            {
                break;
            }
        }
        else if (errno == EAGAIN)
        {//for async "If no process has the pipe open for writing, read() will return 0 to indicate end-of-file"
            //                        printf("worker fifo recive error %d,len %d\n", errno, n);
            usleep(1);
        }
        else
        {
            //                        printf("worker fifo recive error2 %d,len %d\n", errno, n);
        }
    } while ((n < 0 && errno == EINTR) || n > 0);
    return total;
}

int cpNetRead(int fd, void *buf, int len)
{
    int n, total = 0;
    do
    {
        n = recv(fd, buf + total, len, MSG_WAITALL);
        if (n > 0)
        {
            total += n;
            if (total == len)
            {
                break;
            }
        }
        else if (n == 0)
        {
            return 0;
        }
        //                else {
        //                    cpLog("worker recive error %d,len %d,%d\n", errno, n,fd);
        //                }
    } while ((n < 0 && errno == EINTR) || n > 0);
    return total;
}

void cpSettitle(char *title_name)
{

    //    assert(MAX_TITLE_LENGTH > strlen(title) + 5);
    int len = 0;
    char name[MAX_TITLE_LENGTH + 5] = {0};
    strcat(name, "pool_");
    if (strlen(title_name) >= MAX_TITLE_LENGTH)
    {
        len = MAX_TITLE_LENGTH;
    }
    else
    {
        len = strlen(title_name);
    }
    memcpy(name + 5, title_name, len);
    zval *zname;
    CP_MAKE_STD_ZVAL(zname);
    CP_ZVAL_STRING(zname, name, 0);

#if PHP_MAJOR_VERSION >= 7 || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 4)
    zval *retval;
    zval **args[1];
    args[0] = &zname;

    zval *function;
    CP_MAKE_STD_ZVAL(function);
    CP_ZVAL_STRING(function, "cli_set_process_title", 1);

    if (cp_call_user_function_ex(EG(function_table), NULL, function, &retval, 1, args, 0, NULL TSRMLS_CC) == FAILURE)
    {
        return;
    }
    cp_zval_ptr_dtor(&function);
    if (retval)
    {
        cp_zval_ptr_dtor(&retval);
    }
#else
    bzero(sapi_module.executable_location, 127);
    memcpy(sapi_module.executable_location, Z_STRVAL_P(zname), Z_STRLEN_P(zname));
#endif
}

void cpSetIsBlock(int sock, int block)
{
    int opts, ret;
    do
    {
        opts = fcntl(sock, F_GETFL);
    } while (opts < 0 && errno == EINTR);

    if (opts < 0)
    {
        cpLog("fcntl(%d, GETFL) failed.", sock);
    }

    if (!block)
    {
        opts = opts | O_NONBLOCK;
    }
    else
    {
        opts = opts & ~O_NONBLOCK;
    }

    do
    {
        ret = fcntl(sock, F_SETFL, opts);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0)
    {
        cpLog("fcntl(%d, SETFL, opts) failed.", sock);
    }
}

int cpSetTimeout(int sock, double timeout)
{
    int ret;
    struct timeval timeo;
    timeo.tv_sec = (int) timeout;
    timeo.tv_usec = (int) ((timeout - timeo.tv_sec) * 1000 * 1000);
    ret = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (void *) &timeo, sizeof (timeo));
    if (ret < 0)
    {
        return FAILURE;
    }
    ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (void *) &timeo, sizeof (timeo));
    if (ret < 0)
    {
        return FAILURE;
    }
    return SUCCESS;
}

int cpCreateFifo(char *file)
{
    int pipe_fd;
    int res;
    umask(0);
    if (access(file, F_OK) == -1)
    {
        res = mkfifo(file, 0666);
        if (res != 0 && errno != EEXIST)
        {//&&避免 worker和client一起创建导致的exist错误
            cpLog("Could not create fifo %s Error: %s[%d]", file, strerror(errno), errno);
            return -1;
        }
    }
    pipe_fd = open(file, CP_PIPE_MOD);
    if (pipe_fd == -1)
    {
        cpLog("Could not open fifo %s Error: %s[%d]", file, strerror(errno), errno);
        return -1;
    }
    return pipe_fd;
}

/**
 * clear all singal
 */
void swSingalNone()
{
    sigset_t mask;
    sigfillset(&mask);
    int ret = pthread_sigmask(SIG_BLOCK, &mask, NULL);
    if (ret < 0)
    {
        cpLog("pthread_sigmask fail: %s", strerror(ret));
    }
}

//zval * cpMD5(zval *arr)
//{//pass in array , out md5 zval
//    smart_str ser_data = {0};
//    cp_serialize(&ser_data, arr);
//
//    zval fun_name, **args[1], *retval, *str;
//    CP_ZVAL_STRING(&fun_name, "md5", 0);
//
//    CP_MAKE_STD_ZVAL(str);
//#if PHP_MAJOR_VERSION < 7
//    CP_ZVAL_STRINGL(str, ser_data.c, ser_data.len, 1);
//#else
//    zend_string *str_data = ser_data.s;
//    CP_ZVAL_STRINGL(str, str_data->val, str_data->len, 1);
//#endif
//    args[0] = &str;
//
//    if (cp_call_user_function_ex(CG(function_table), NULL, &fun_name, &retval, 1, args, 0, NULL TSRMLS_CC) != SUCCESS)
//    {
//        cp_zval_ptr_dtor(&str);
//        smart_str_free(&ser_data);
//        return NULL;
//    }
//    cp_zval_ptr_dtor(&str);
//    smart_str_free(&ser_data);
//    return retval;
//}

void cp_serialize(smart_str *ser_data, zval *array)
{
    php_serialize_data_t var_hash;
    PHP_VAR_SERIALIZE_INIT(var_hash);
#if PHP_MAJOR_VERSION < 7
    php_var_serialize(ser_data, &array, &var_hash TSRMLS_CC);
#else
    php_var_serialize(ser_data, array, &var_hash TSRMLS_CC);
#endif
    PHP_VAR_SERIALIZE_DESTROY(var_hash);

    //    gettimeofday(&end, NULL);
    //    int timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    //    printf("ser time: %d us\n", timeuse);
}

zval * cp_unserialize(char *data, int len)
{
    zval *unser_value;
    CP_ALLOC_INIT_ZVAL(unser_value);
    php_unserialize_data_t var_hash;
    PHP_VAR_UNSERIALIZE_INIT(var_hash);
    if (cp_php_var_unserialize(&unser_value, (const unsigned char **) &data, (unsigned char *) data + len - 1, &var_hash TSRMLS_CC) != 1)
    {
        //        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "unser data is corrupted");
    }
    PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
    return unser_value;
}

void cp_ser_and_setpro(zval *arr)
{
    smart_str ser_data = {0};
    cp_serialize(&ser_data, arr);
#if PHP_MAJOR_VERSION < 7
    memcpy(CPGL.ping_mem_addr + CP_PING_MD5_LEN + CP_PING_PID_LEN + CP_PING_DIS_LEN, ser_data.c, ser_data.len);
#else
    zend_string *str = ser_data.s;
    memcpy(CPGL.ping_mem_addr + CP_PING_MD5_LEN + CP_PING_PID_LEN + CP_PING_DIS_LEN, str->val, str->len);
#endif
    smart_str_free(&ser_data);
}

void cp_ser_and_setdis(zval *arr)
{
    smart_str ser_data = {0};
    cp_serialize(&ser_data, arr);
#if PHP_MAJOR_VERSION < 7
    memcpy(CPGL.ping_mem_addr + CP_PING_MD5_LEN + CP_PING_PID_LEN, ser_data.c, ser_data.len);
#else
    zend_string *str = ser_data.s;
    memcpy(CPGL.ping_mem_addr + CP_PING_MD5_LEN + CP_PING_PID_LEN, str->val, str->len);
#endif
    smart_str_free(&ser_data);
}

cpSignalFunc cpSignalSet(int sig, cpSignalFunc func, int restart, int mask)
{
    struct sigaction act, oact;
    act.sa_handler = func;
    if (mask)
    {
        sigfillset(&act.sa_mask);
    }
    else
    {
        sigemptyset(&act.sa_mask);
    }
    act.sa_flags = 0;
    //        act.sa_flags = SA_SIGINFO;
    if (sigaction(sig, &act, &oact) < 0)
    {
        return NULL;
    }
    return oact.sa_handler;
}

int cpQueueSignalSet(int sig, cpQueueFunc func)
{
    struct sigaction act, oact;
    sigemptyset(&act.sa_mask);

    act.sa_sigaction = func;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(sig, &act, &oact) < 0)
    {
        cpLog("sigaction error %d", errno);
        return FAILURE;
    }
    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, CP_SIG_EVENT);
    sigprocmask(SIG_BLOCK, &block, NULL);

    return SUCCESS;
}

#ifndef HAVE_CLOCK_GETTIME
#ifdef __MACH__

int clock_gettime(clock_id_t which_clock, struct timespec *t)
{
    // be more careful in a multithreaded environement
    if (!orwl_timestart)
    {
        mach_timebase_info_data_t tb = {0};
        mach_timebase_info(&tb);
        orwl_timebase = tb.numer;
        orwl_timebase /= tb.denom;
        orwl_timestart = mach_absolute_time();
    }
    double diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;
    t->tv_sec = diff * ORWL_NANO;
    t->tv_nsec = diff - (t->tv_sec * ORWL_GIGA);
    return 0;
}
#endif
#endif
