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
   | Author: Xinhua Guo  <woshiguo35@sina.com>                            |
   +----------------------------------------------------------------------+
 */
#include "php_connect_pool.h"

int static cpListen();
void static cpSignalHandle(int sig);
void static cpSignalInit(void);
int static cpReactor_client_close(int fd);
static int cpReactor_client_receive(int fd);
static int cpReactor_client_release(int fd);
static int cpReactor_start(int sock);

void cpServer_init_common(zval *conf)
{
    zval *v;
    //daemonize，守护进程化
    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("daemonize"), (void **) &v) == SUCCESS)
    {
        convert_to_long(v);
        CPGC.daemonize = (int) Z_LVAL_P(v);
    }

    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("recycle_num"), (void **) &v) == SUCCESS)
        //if (cp_zend_hash_find(Z_ARRVAL_P(conf), "recycle_num", strlen("recycle_num"), (void **) &v) == SUCCESS)
    {
        convert_to_long(v);
        CPGC.recycle_num = (int) Z_LVAL_P(v);
    }
    //error_file
    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("log_file"), (void **) &v) == SUCCESS)
        //if (cp_zend_hash_find(Z_ARRVAL_P(conf), "log_file", strlen("log_file"), (void **) &v) == SUCCESS)
    {
        memcpy(CPGC.log_file, Z_STRVAL_P(v), Z_STRLEN_P(v));
    }

    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("max_read_len"), (void **) &v) == SUCCESS)
    {
        convert_to_long(v);
        CPGC.max_read_len = (int) Z_LVAL_P(v);
    }
    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("port"), (void **) &v) == SUCCESS)
    {//todo check null
        convert_to_long(v);
        CPGC.port = (int) Z_LVAL_P(v);
    }

    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("idel_time"), (void **) &v) == SUCCESS)
    {
        convert_to_long(v);
        CPGC.idel_time = (int) Z_LVAL_P(v);
    }

    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("ping_time"), (void **) &v) == SUCCESS)
    {
        convert_to_long(v);
        CPGC.ping_time = (int) Z_LVAL_P(v);
    }

    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("ser_fail_hits"), (void **) &v) == SUCCESS)
    {
        convert_to_long(v);
        CPGC.ser_fail_hits = (int) Z_LVAL_P(v);
    }

    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("max_fail_num"), (void **) &v) == SUCCESS)
    {
        convert_to_long(v);
        CPGC.max_fail_num = (int) Z_LVAL_P(v);
    }

    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("max_data_size_to_log"), (void **) &v) == SUCCESS)
    {
        convert_to_long(v);
        CPGC.max_data_size_to_log = (int) Z_LVAL_P(v);
    }

    if (cp_zend_hash_find(Z_ARRVAL_P(conf), ZEND_STRS("max_hold_time_to_log"), (void **) &v) == SUCCESS)
    {
        convert_to_long(v);
        CPGC.max_hold_time_to_log = (int) Z_LVAL_P(v);
    }
}

void cpKillClient()
{
    int i;
    for (i = 0; i <= CP_MAX_FDS; i++)
    {
        cpConnection *conn = &(CPGS->conlist[i]);
        if (conn->fpm_pid)
        {
            kill(conn->fpm_pid, 9);
            //  printf("kill %d\n",conn->fpm_pid);
        }
    }
}

static void cpServer_init_lock()
{
    int i = 0;
    for (; i < CP_GROUP_NUM - 1; i++)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        if (pthread_mutex_init(&CPGS->G[i].mutex_lock, &attr) < 0)
        {
            cpLog("pthread_mutex_init error!. Error: %s [%d]", strerror(errno), errno);
        }
        CPGS->G[i].lock = cpMutexLock;
        CPGS->G[i].unLock = cpMutexUnLock;
        CPGS->G[i].tryLock = cpMutexTryLock;
    }
}

int cpServer_init(zval *conf, char *ini_file)
{
    int sock = 0;
    size_t group_num = 0;
    cpShareMemory shm = {0};
    zval *config;
    char *name;
    uint32_t klen;
    int ktype;
    HashTable *_ht = Z_ARRVAL_P(conf);

    bzero(&CPGL, sizeof (cpServerG));
    CPGC.backlog = CP_BACKLOG;
    //    CPGC.reactor_num = CP_CPU_NUM;
    CPGC.reactor_num = 1;
    CPGC.timeout_sec = CP_REACTOR_TIMEO_SEC;
    CPGC.timeout_usec = CP_REACTOR_TIMEO_USEC;
    CPGC.max_conn = CP_MAX_FDS;
    CPGC.max_request = CP_MAX_REQUEST;
    CPGC.idel_time = CP_IDEL_TIME;
    CPGC.ping_time = CP_PING_SLEEP;
    CPGC.recycle_num = CP_RECYCLE_NUM;
    CPGC.max_read_len = CP_DEF_MAX_READ_LEN;
    CPGC.ser_fail_hits = 1;
    CPGC.max_fail_num = 2;
    CPGC.port = CP_PORT_PDO;
    CPGC.max_data_size_to_log = 0;
    CPGC.max_hold_time_to_log = 0;

    if ((sock = cpListen()) < 0)
    {
        printf("listen[1] fail\n");
        return FAILURE;
    }

    if ((cp_create_mmap_dir()) < 0)
    {
        php_printf("mkdir[1] fail\n");
        return FAILURE;
    }

    shm.size = sizeof (cpServerGS);
    strncpy(shm.mmap_name, CP_SERVER_MMAP_FILE, strlen(CP_SERVER_MMAP_FILE));
    if (cp_create_mmap_file(&shm) == 0)
    {
        CPGS = (cpServerGS*) cp_mmap_calloc_with_file(&shm);
        cpKillClient();
        bzero(CPGS, shm.size);
        if (CPGS == NULL)
        {
            php_printf("calloc[1] fail\n");
            return FAILURE;
        }
    }
    else
    {
        php_printf("calloc[1] fail\n");
        return FAILURE;
    }
    strcpy(CPGC.ini_file, ini_file);

    CP_HASHTABLE_FOREACH_START2(_ht, name, klen, ktype, config)
    {
        if (strcmp(name, "common") == 0)
        {//common config
            cpServer_init_common(config);
        }
        else
        {
            zval *v;
            strcpy(CPGS->G[group_num].name, name);
            if (cp_zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("pool_min"), (void **) &v) == SUCCESS)
            {
                convert_to_long(v);
                CPGS->G[group_num].worker_num = CPGS->G[group_num].worker_min = Z_LVAL_P(v);
            }
            if (cp_zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("pool_max"), (void **) &v) == SUCCESS)
            {
                convert_to_long(v);
                CPGS->G[group_num].worker_max = Z_LVAL_P(v);
            }
            CPGS->group_num++;
            group_num++;
        }

    }
    CP_HASHTABLE_FOREACH_END();

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (pthread_mutex_init(&CPGS->mutex_lock, &attr) < 0)
    {
        cpLog("pthread_mutex_init error!. Error: %s [%d]", strerror(errno), errno);
        return FAILURE;
    }

    CPGS->default_min = CP_DEF_MIN_NUM;
    CPGS->default_max = CP_DEF_MAX_NUM;
    CPGS->max_buffer_len = CPGC.max_read_len;
    CPGS->max_data_size_to_log = CPGC.max_data_size_to_log;
    CPGS->max_hold_time_to_log = CPGC.max_hold_time_to_log;
    strcpy(CPGS->log_file, CPGC.log_file);

    cpServer_init_lock();
    return sock;

}

int cpServer_create()
{
    if (CPGC.reactor_num < 1 || CPGC.max_read_len >= CP_MAX_READ_LEN)
    {
        php_printf("reactor_num < 1 or max_read_len >%d\n", CP_MAX_READ_LEN);
        return FAILURE;
    }

    if (CPGC.ser_fail_hits < 1 || CPGC.max_fail_num < 1)
    {
        php_printf("ping server conf error\n");
        return FAILURE;
    }

    cpLog_init(CPGC.log_file);
    CPGS->reactor_threads = (cpThread*) cp_mmap_calloc(CPGC.reactor_num * sizeof (cpThread));
    if (CPGS->reactor_threads == NULL)
    {
        cpLog("calloc[1] fail");
        return FAILURE;
    }

    CPGS->ping_workers = (cpWorker*) cp_mmap_calloc(sizeof (cpWorker));
    if (CPGS->ping_workers == NULL)
    {
        cpLog("[Main] calloc[ping_workers] fail");
        return FAILURE;
    }

    CPGS->running = 1;

    return SUCCESS;
}

int cpServer_start(int sock)
{
    int w, pid, ret, g;
    if (CPGC.daemonize > 0)
    {
        if (daemon(0, 0) < 0)
        {
            return FAILURE;
        }
    }

    CPGS->master_pid = getpid();
    CPGL.process_type = CP_PROCESS_MASTER;

    pid = fork();
    switch (pid)
    {
            //创建manager进程
        case 0:
            for (g = 0; g < CPGS->group_num; g++)
            {
                for (w = 0; w < CPGS->G[g].worker_min; w++)
                {
                    //alloc了max个 但是只启动min个
                    ret = cpCreate_worker_mem(w, g);
                    pid = cpFork_one_worker(w, g);
                    if (pid < 0 || ret < 0)
                    {
                        cpLog("Fork worker process fail");
                        return FAILURE;
                    }
                    else
                    {
                        CPGS->G[g].workers[w].pid = pid;
                        CPGS->G[g].workers_status[w] = CP_WORKER_IDLE;
                    }
                }
            }
            //数据库坏连接检测恢复进程
            //            ret = cpCreate_ping_worker_mem();
            //            ping_pid = cpFork_ping_worker();
            //            if (ping_pid < 0 || ret < 0)
            //            {
            //                cpLog("Fork ping  process fail");
            //                return FAILURE;
            //            }
            //            CPGS->ping_workers->pid = ping_pid;

            //标识为管理进程
            CPGL.process_type = CP_PROCESS_MANAGER;
            ret = cpWorker_manager_loop();
            exit(ret);
            break;
            //主进程
        default:
            CPGS->manager_pid = pid;
            break;
        case -1:
        {
            cpLog("fork manager process fail");
            return FAILURE;
        }
    }

    cpSignalInit();
    if (cpReactor_start(sock) < 0)
    {
        cpLog("Reactor_start[1] fail");
        return FAILURE;
    }
    return SUCCESS;
}

static int cpServer_master_onAccept(int fd)
{
    struct sockaddr_in client_addr;
    uint32_t client_addrlen = sizeof (client_addr);
    int conn_fd, c_pti = 0, i;

    for (i = 0; i < CP_ACCEPT_MAX_COUNT; i++)
    {
        //accept得到连接套接字
        conn_fd = accept(fd, (struct sockaddr *) &client_addr, &client_addrlen);
        if (conn_fd < 0)
        {
            switch (errno)
            {
                case EAGAIN:
                    return SUCCESS;
                case EINTR:
                    continue;
                default:
                    cpLog("accept fail. Error: %s[%d]", strerror(errno), errno);
                    return SUCCESS;
            }
        }
        //连接过多
        if (CPGS->connect_count >= CPGC.max_conn)
        {
            cpLog("too many connection,please reduce you worker num");
            close(conn_fd);
            return SUCCESS;
        }

        int flag = 1;
        setsockopt(conn_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof (flag));
#if (defined SO_KEEPALIVE) && (defined TCP_KEEPIDLE)  
        int keepalive = 1;
        int keep_idle = CP_TCP_KEEPCOUNT;
        int keep_interval = CP_TCP_KEEPIDLE;
        int keep_count = CP_TCP_KEEPINTERVAL;

        setsockopt(conn_fd, SOL_SOCKET, SO_KEEPALIVE, (void *) &keepalive, sizeof (keepalive));
        setsockopt(conn_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) &keep_idle, sizeof (keep_idle));
        setsockopt(conn_fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *) &keep_interval, sizeof (keep_interval));
        setsockopt(conn_fd, IPPROTO_TCP, TCP_KEEPCNT, (void *) &keep_count, sizeof (keep_count));
#endif

        if (CPGC.reactor_num > 1)
        {
            int i, event_num = CPGS->reactor_threads[0].event_num;
            CPGS->reactor_next_i = 0;
            for (i = 1; i < CPGC.reactor_num; i++)
            {
                if (CPGS->reactor_threads[i].event_num < event_num)
                {
                    CPGS->reactor_next_i = i;
                    event_num = CPGS->reactor_threads[i].event_num;
                }
            }
            c_pti = CPGS->reactor_next_i;
        }


        cpConnection *conn = &(CPGS->conlist[conn_fd]);
        if (conn)
        {//不能在add后做,线程安全,防止添加到reactor后马上就读到数据,这时候下面new_connect还没执行。
            conn->release = CP_FD_RELEASED;
        }
        if (cpEpoll_add(CPGS->reactor_threads[c_pti].epfd, conn_fd, EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLPRI) < 0)
        {
            cpLog("[Master]add event fail Errno=%d|FD=%d", errno, conn_fd);
            close(conn_fd);
            return SUCCESS;
        }
        else
        {
            CPGS->reactor_threads[c_pti].event_num++;
            conn->fd = conn_fd;
            conn->pth_id = c_pti;
            CPGS->connect_count++;
        };

        continue;
    }
    return SUCCESS;
}

//void cpPrint_queue(cpGroup *G)
//{
//    int current_fd = G->first_wait_id;
//    while (current_fd)
//    {
//        cpConnection* conn = &(CPGS->conlist[current_fd]);
//        printf("fd is %d\n", conn->fd);
//        current_fd = conn->next_wait_id;
//    }
//}

static int cpReactor_client_release(int fd)
{
    cpConnection *conn = &(CPGS->conlist[fd]);
    cpGroup *G = &CPGS->G[conn->group_id];

    if (G->lock(G) == 0)
    {
        if (conn->release == CP_FD_NRELEASED)
        {//防止too many cons&&重复release
            if (G->first_wait_id && conn->worker_index <= G->worker_max)
            {//wait is not null&&use queue&&use reload to reduce max maybe trigger this
                int wait_pid = cpPopWaitQueue(G, conn);
                if (kill(wait_pid, SIGRTMIN) < 0)
                {
                    CPGS->G[conn->group_id].workers_status[conn->worker_index] = CP_WORKER_IDLE;
                    cpLog("send sig 2 %d error. Error: %s [%d]", wait_pid, strerror(errno), errno);
                }
                G->unLock(G);
            }
            else
            {
                CPGS->G[conn->group_id].workers_status[conn->worker_index] = CP_WORKER_IDLE;
                G->unLock(G);
            }
        }
        else if (conn->release == CP_FD_WAITING)
        {
            cpLog("The fd %d is closed and remove from the queue but no conn dispatch , maybe have slow query", fd);
            cpConnection *pre = NULL;
            int current_fd = G->first_wait_id;
            while (current_fd)
            {
                conn = &(CPGS->conlist[current_fd]);
                if (conn->fd == fd)
                {
                    if (fd == G->first_wait_id)
                    {
                        if (conn->next_wait_id)
                        {
                            G->first_wait_id = conn->next_wait_id;
                            conn->next_wait_id = 0;
                        }
                        else
                        {//only one
                            G->first_wait_id = G->last_wait_id = 0;
                        }
                    }
                    else if (fd == G->last_wait_id)
                    {
                        pre->next_wait_id = 0;
                        G->last_wait_id = pre->fd;
                    }
                    else
                    {
                        pre->next_wait_id = conn->next_wait_id;
                        conn->next_wait_id = 0;
                    }
                    break;
                }
                pre = conn;
                current_fd = conn->next_wait_id;
            }
            G->unLock(G);
        }
        else
        {
            G->unLock(G);
        }
    }

    return SUCCESS;

}

static int cpReactor_client_close(int fd)
{//长连接 相当于mshutdown
    cpReactor_client_release(fd);
    cpConnection *conn = &(CPGS->conlist[fd]);
    conn->fpm_pid = 0;
    //关闭连接
    cpEpoll_del(CPGS->reactor_threads[conn->pth_id].epfd, fd);
    (CPGS->reactor_threads[conn->pth_id].event_num <= 0) ? CPGS->reactor_threads[conn->pth_id].event_num = 0 : CPGS->reactor_threads[conn->pth_id].event_num--;
    CPGS->connect_count--;

    return SUCCESS;
}

static int cpReactor_client_receive(int fd)
{
    int event_size = sizeof (cpTcpEvent), n, ret = -1;
    char data[event_size];
    //非ET模式会持续通知
    n = cpNetRead(fd, data, event_size);
    if (n > 0)
    {
        cpTcpEvent *event = (cpTcpEvent*) data;
        switch (event->type)
        {
            case CP_TCPEVENT_ADD:
                ret = kill(CPGS->manager_pid, SIGRTMIN);
                if (ret < 0)
                {//TODO 
                    cpLog("send sig error. Error: %s [%d]", strerror(errno), errno);
                }
                break;
            case CP_TCPEVENT_GETFD:
            {
                cpMasterInfo info;
                info.server_fd = fd;
                CPGS->conlist[fd].fpm_pid = event->data;
                ret = cpWrite(fd, &info, sizeof (info));
                break;
            }
            default:
                cpLog("wrong type");
                break;
        }
        return ret;

    }
    else if (n == 0)
    {
close_fd:
        return cpReactor_client_close(fd);
    }
    else
    {//需要检测errno来区分是EAGAIN还是ECONNRESET
        if (errno == EAGAIN)
        {
            return SUCCESS;
        }
        else if (errno == ECONNRESET)
        {
            goto close_fd;
        }
        else
        {
            cpLog("Read from socket[%d] fail. Error: %s [%d]", fd, strerror(errno), errno);
            return SUCCESS;
        }
    }
    return SUCCESS;
}

int static cpReactor_thread_loop(int *id)
{

    struct timeval timeo;
    timeo.tv_sec = CP_REACTOR_TIMEO_SEC;
    timeo.tv_usec = CP_REACTOR_TIMEO_USEC;

    swSingalNone();

    int epfd = epoll_create(512); //这个参数没用
    CPGS->reactor_threads[*id].epfd = epfd;

    epoll_wait_handle handles[CP_MAX_EVENT];
    handles[EPOLLIN] = cpReactor_client_receive;
    handles[EPOLLPRI] = cpReactor_client_release;
    handles[EPOLL_CLOSE] = cpReactor_client_close;

    cpEpoll_wait(handles, &timeo, epfd);

    free(id);
    pthread_exit(0);
    return SUCCESS;
}

int static cpReactor_start(int sock)
{
    int i;
    int accept_epfd = epoll_create(512); //这个参数没用
    if (cpEpoll_add(accept_epfd, sock, EPOLLIN) < 0)
    {
        return FAILURE;
    };

    pid_init();
    set_pid(CPGS->master_pid);

    struct timeval timeo;
    timeo.tv_sec = CP_REACTOR_TIMEO_SEC;
    timeo.tv_usec = CP_REACTOR_TIMEO_USEC;
    pthread_t pidt;
    for (i = 0; i < CPGC.reactor_num; i++)
    {
        int *index = (int*) malloc(sizeof (int));
        *index = i;
        if (pthread_create(&pidt, NULL, (void * (*)(void *)) cpReactor_thread_loop, (void *) index) < 0)
        {
            cpLog("pthread_create[tcp_reactor] fail");
        }
        pthread_detach(pidt);
        CPGS->reactor_threads[i].thread_id = pidt;
    }
    epoll_wait_handle handles[CP_MAX_EVENT];
    handles[EPOLLIN] = cpServer_master_onAccept;

    //    usleep(50000);
    sleep(1);
    cpLog("start  success");
    return cpEpoll_wait(handles, &timeo, accept_epfd);
}

int static cpListen()
{
    int sock;
    int option;
    int ret;

    struct sockaddr_in addr_in4;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("swSocket_listen: Create socket fail.Errno=%d\n", errno);
        return FAILURE;
    }
    option = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof (int));

    bzero(&addr_in4, sizeof (addr_in4));
    inet_pton(AF_INET, "127.0.0.1", &(addr_in4.sin_addr));
    addr_in4.sin_port = htons(CPGC.port);
    addr_in4.sin_family = AF_INET;
    ret = bind(sock, (struct sockaddr *) &addr_in4, sizeof (addr_in4));

    if (ret < 0)
    {
        printf("Bind fail.port=%d. Error: %s [%d]\n", CPGC.port, strerror(errno), errno);
        return FAILURE;
    }
    //开始监听套接字
    ret = listen(sock, CPGC.backlog);
    if (ret < 0)
    {
        printf("Listen fail.port=%d. Error: %s [%d]\n", CPGC.port, strerror(errno), errno);
        return FAILURE;
    }
    cpSetIsBlock(sock, 0);

    if (sock < 0)
    {
        return FAILURE;
    }
    int bufsize = CP_UNSOCK_BUFSIZE;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof (bufsize));
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof (bufsize));
    return sock;
}

static void cpSignalHandle(int sig)
{
    switch (sig)
    {
        case SIGTERM:
            cpLog("stop pool server");
            CPGS->running = 0;
            int i, j = 0, ret;
            for (; j < CPGS->group_num; j++)
            {
                cpGroup *G = &CPGS->G[j];
                for (i = 0; i < G->worker_num; i++)
                {
                    ret = kill(G->workers[i].pid, SIGKILL);
                    if (ret == -1)
                    {
                        cpLog("kill failed, id=%d. Error: %s [%d]", i, strerror(errno), errno);
                    }
                }
            }
            //            ret = kill(CPGS->ping_workers->pid, SIGKILL);
            //            if (ret == -1)
            //            {
            //                cpLog("kill ping worker failed, id=%d. Error: %s [%d]", i, strerror(errno), errno);
            //            }
            exit(1);
            break;
        case SIGUSR1:
            cpLog("reload pool server");
            ret = kill(CPGS->manager_pid, SIGUSR1);
            if (ret == -1)
            {
                cpLog("reload failed, Error: %s [%d]", strerror(errno), errno);
            }
            break;
        default:
            break;
    }
}

int cpMutexLock(cpGroup *G)
{
    if (pthread_mutex_lock(&G->mutex_lock) != 0)
    {
        cpLog("pthread_mutex_lock. Error: %s [%d]", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int cpMutexUnLock(cpGroup *G)
{
    if (pthread_mutex_unlock(&G->mutex_lock) != 0)
    {
        cpLog("pthread_mutex_unlock. Error: %s [%d]", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int cpMutexTryLock(cpGroup *G)
{
    if (pthread_mutex_trylock(&G->mutex_lock) != 0)
    {
        cpLog("pthread_mutex_trylock. Error: %s [%d]", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int cpPopWaitQueue(cpGroup *G, cpConnection *conn)
{
    cpConnection *wait_conn = &CPGS->conlist[G->first_wait_id]; //等待队列的连接
    wait_conn->worker_id = conn->worker_id;
    wait_conn->worker_index = conn->worker_index;
    wait_conn->group_id = conn->group_id;
    wait_conn->release = CP_FD_NRELEASED;
    if (wait_conn->next_wait_id)
    {
        G->first_wait_id = wait_conn->next_wait_id;
        wait_conn->next_wait_id = 0;
    }
    else
    {
        G->first_wait_id = G->last_wait_id = 0;
    }
    int wait_pid = wait_conn->wait_fpm_pid;
    wait_conn->wait_fpm_pid = 0;
    CPGS->G[wait_conn->group_id].workers[wait_conn->worker_index].CPid = wait_pid;
    return wait_pid;
}

void static cpSignalInit(void)
{
    cpSignalSet(SIGHUP, SIG_IGN, 1, 0);
    cpSignalSet(SIGPIPE, SIG_IGN, 1, 0);
    cpSignalSet(SIGUSR1, cpSignalHandle, 1, 0);
    cpSignalSet(SIGUSR2, SIG_IGN, 1, 0);
    cpSignalSet(SIGTERM, cpSignalHandle, 1, 0);
}
