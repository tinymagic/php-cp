<?php

if (class_exists("PDO")):
    /**
     * pdoProxy stub
     * @stub
     */
    class pdoProxy extends PDO
    {
        /**
         * release current PDO connection to the pool
         * @return bool
         */
        public function release()
        {
            return true;
        }

        /**
         * force use the master connection
         *
         * only valid when using master-slave config
         * @return true
         */
        public function forceMaster()
        {
            return true;
        }

        /**
         * close the PDO connection
         * @return bool
         */
        public function close()
        {
            return true;
        }

        /**
         * 是否异步
         *
         * @param bool $async
         *
         * @return bool
         */
        public function setAsync($async)
        {
            unset($async);
            return true;
        }

        /**
         * 异步执行完成
         * @return bool|mixed
         */
        public function done()
        {
            return true;
        }
    }

    /**
     * Class pdo_connect_pool_PDOStatement
     * @stub
     */
    class pdo_connect_pool_PDOStatement extends PDOStatement implements Iterator
    {
        /**
         * 是否异步
         *
         * @param bool $async
         *
         * @return bool
         */
        public function setAsync($async)
        {
            unset($async);
            return true;
        }

        /**
         * release current PDO connection to the pool
         * @return bool
         */
        public function release()
        {
            return true;
        }

        /**
         * 异步执行完成
         * @return bool|mixed
         */
        public function done()
        {
            return true;
        }

        /**
         * move cursor the beginning
         */
        public function rewind()
        {
        }

        /**
         * move cursor to the next
         */
        public function next()
        {
        }

        /**
         * get current element
         * @return mixed
         */
        public function current()
        {
            return null;
        }


        /**
         * get current key
         * @return mixed
         */
        public function key()
        {
            return null;
        }

        /**
         * is valid
         * @return bool
         */
        public function valid()
        {
            return true;
        }
    }
endif;


if (class_exists("PDO")):
    class redisProxy extends Redis
    {
        /**
         * release current PDO connection to the pool
         * @return bool
         */
        public function release()
        {
            return true;
        }

        /**
         * force use the master connection
         *
         * only valid when using master-slave config
         * @return true
         */
        public function forceMaster()
        {
            return true;
        }

        /**
         * close the PDO connection
         * @return bool
         */
        public function close()
        {
            return true;
        }

        /**
         * 是否异步
         *
         * @param bool $async
         *
         * @return bool
         */
        public function setAsync($async)
        {
            unset($async);
            return true;
        }

        /**
         * 异步执行完成
         * @return bool|mixed
         */
        public function done()
        {
            return true;
        }
    }
endif;
