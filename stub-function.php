<?php

/**
 * 运行服务
 *
 * @param string $conf
 * @return bool|null
 */
function pool_server_create($conf)
{
    unset($conf);
    return true;
}

/**
 * 查看服务状态
 *
 * @param int $pid 服务进程id 可从pid文件获取
 */
function pool_server_status($pid)
{
}

/**
 * 重新加载服务配置
 *
 * @param int $pid 服务进程id 可从pid文件获取
 * @return bool|null
 */
function pool_server_reload($pid)
{
    unset($pid);
    return true;
}

/**
 * 关闭服务
 *
 * @param int $pid 服务进程id 可从pid文件获取
 * @return bool|null
 */
function pool_server_shutdown($pid)
{
    unset($pid);
    return true;
}

/**
 * 获取版本号
 * @return string
 */
function pool_server_version()
{
    return "";
}
