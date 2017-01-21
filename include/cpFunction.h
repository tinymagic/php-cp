/* 
 * File:   cpFunction.h
 * Author: gxhua
 *
 * Created on 2014年10月6日, 下午9:22
 */

#ifndef CONPFUNCTION_H
#define	CONPFUNCTION_H

#ifdef	__cplusplus
extern "C" {
#endif

#define CP_FIFO_NAME_LEN   200
#define CP_FIFO_NAME_PRE   "/var/run/cp/con_pool_c2w_pipe"
#define CP_MMAP_NAME_PRE   "/var/run/cp/con_pool_mmap"
#define SW_LOG_BUFFER_SIZE 1024
#define SW_PID_BUFFER_SIZE 100
#define SW_LOG_DATE_STRLEN  64
#define CP_LOG_FORMAT "[%s]\t%s\t\n"
#define FAILUREOR_MSG_SIZE 1024
#define MAX_TITLE_LENGTH   120
#define MAX_INI_LENGTH   1024

#define PID_FILE_PATH "/var/run/php_connection_pool"
#define cpLog(str,...)     \
                        do \
                            { \
                                char cp_error_str[FAILUREOR_MSG_SIZE];\
                                snprintf(cp_error_str,FAILUREOR_MSG_SIZE,"%s: "str,__func__,##__VA_ARGS__);\
                                char date_str[SW_LOG_DATE_STRLEN];\
                                time_t t;\
                                struct tm *p;\
                                t = time(NULL);\
                                p = localtime(&t);\
                                snprintf(date_str, SW_LOG_DATE_STRLEN, "%d-%02d-%02d %02d:%02d:%02d", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);\
                                fprintf(cp_log_fn, CP_LOG_FORMAT, date_str,cp_error_str); \
                                if(CPGC.daemonize == 0) fprintf(stdout, CP_LOG_FORMAT, date_str,cp_error_str); \
                           }while(0);

    typedef void (*cpSignalFunc)(int);
    typedef void (*cpQueueFunc) (int, siginfo_t *, void *);


    int cpLog_init(char *logfile);
    int pid_init();
    int set_pid(int pid);
    int cpWrite(int fd, void *buf, int count);
    int cpSetTimeout(int sock, double timeout);
    int cpQueueSignalSet(int sig, cpQueueFunc func);
    int cpNetRead(int fd, void *buf, int len);
    int cpCreateFifo(char *file);
    int cpFifoRead(int pipe_fd_read, void *buf, int len);

    void cp_ser_and_setpro(zval *arr);
    void cpSettitle(char *title);
    void cpSetIsBlock(int sock, int flag);
    void swSingalNone();

    //zval* cpGetConfig(char *filename);
    //    zval * cpMD5(zval *arr);

    cpSignalFunc cpSignalSet(int sig, cpSignalFunc func, int restart, int mask);

    static CPINLINE zval* cpGetConfig(char *filename) {
        zval *fun_name, **args[2], *file, *section, *retval;

        CP_MAKE_STD_ZVAL(file);
        CP_ZVAL_STRING(file, filename, 1);
        CP_MAKE_STD_ZVAL(section);
        ZVAL_BOOL(section, 1);

        args[0] = &file;
        args[1] = &section;

        CP_MAKE_STD_ZVAL(fun_name);
        CP_ZVAL_STRING(fun_name, "parse_ini_file", 0);

        if (cp_call_user_function_ex(EG(function_table), NULL, fun_name, &retval, 2, args, 0, NULL TSRMLS_CC) != SUCCESS) {
            cp_zval_ptr_dtor(&file);
            cp_zval_ptr_dtor(&section);
            return NULL;
        }
        cp_zval_ptr_dtor(&file);
        cp_zval_ptr_dtor(&section);
        //zend_print_zval_r(retval,0);
        return retval;
    }

#ifdef	__cplusplus
}
#endif

#endif	/* CONPFUNCTION_H */

