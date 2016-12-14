<?php

define("PHPCP_TESTRUN", true);

require_once(dirname($_SERVER["PHP_SELF"])."/TestSuite.php");
require_once(dirname($_SERVER["PHP_SELF"])."/RedisTest.php");
require_once(dirname($_SERVER["PHP_SELF"])."/MemcachedTest.php");

/* 确保错误信息可以被正常输出到 stdout */
error_reporting(E_ALL);
ini_set("display_errors", "1");

$args = getopt("", array("host:", "class:", "test:", "nocolors"));
// $class = isset($args["class"]) ? strtolower($args["class"]) : "phpcp";
$class = isset($args["class"]) ? $args["class"] : "RedisTest";
$colorize = !isset($args["nocolors"]);
$filter = isset($args["test"]) ? $args["test"] : NULL;
$host = isset($args["host"]) ? $args["host"] : "127.0.0.1";

TestSuite::flagColorization($colorize);

echo "\n正在测试中，可能会花上一些时间，请耐心等待 :-)\n";
echo "测试类 ";
echo TestSuite::make_bold($class . ":\n");
exit(TestSuite::run($class, $filter, $host));

?>
