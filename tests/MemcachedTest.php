<?php

defined("PHPCP_TESTRUN") or die("使用 RunTest.php 运行测试!");

require_once(dirname($_SERVER["PHP_SELF"])."/TestSuite.php");

class MemcachedTest extends TestSuite {
    public $memcached;

    public function setup()
    {
        $this->memcached = new memcachedProxy();
    }

    public function test_class_exists()
    {
        $this->assertTrue(class_exists("memcachedProxy"));
    }

    public function test_method_exists()
    {
        $this->assertTrue(method_exists($this->memcached, "__call"));
    }

    public function test_add()
    {
        $ret = $this->memcached->set("phpcp", "hello world");
        $this->assertTrue($ret);
    }

    public function test_get()
    {
        $ret = $this->memcached->get("phpcp");
        $this->assertEquals($ret, "hello world");
    }
}

?>
