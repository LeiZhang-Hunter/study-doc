#用apcu实现共享内存

	<?php
	
	
	
	/*
	 * 本程序于2018-12-20 12:39:33
	 * 由陈浩波编写完成
	 * 任何人使用时请保留该声明
	 */
	
	class getset {
	    private $test = 'ss';
	    public function __construct() {
	    }
	
	    public function __get($name) {
	        $apcu_key = __FILE__ . __CLASS__ . $name;
	        if (apcu_exists($apcu_key)) {
	            $this->$name = apcu_fetch($apcu_key);
	        }
	        return $this->$name;
	    }
	    public function __set($name, $value) {
	        $apcu_key = __FILE__ . __CLASS__ . $name;
	        $this->$name = $value;
	        apcu_store($apcu_key, $value);
	        return $name;
	    }
	}
	
	$test = new getset();
	
	$i = 100;
	
	$pid = pcntl_fork();
	//父进程和子进程都会执行下面代码
	if ($pid == -1) {
	    //错误处理：创建子进程失败时返回-1.
	    die('could not fork');
	} else if ($pid) {
	    //父进程会得到子进程号，所以这里是父进程执行的逻辑
	    while ($i--) {
	        $test->test = mt_rand(1, 10000);
	        echo "我是你爹, 我有", $test->test, "元钱!\n";
	        usleep(150);
	    }
	    posix_kill($pid, 5);
	    pcntl_wait($status); //等待子进程中断，防止子进程成为僵尸进程。
	} else {
	    $i *= 2;
	    //子进程得到的$pid为0, 所以这里是子进程执行的逻辑。
	    while ($i--) {
	//        $test->test = mt_rand(1, 10000);
	        echo "我是儿子, 我爹有", $test->test, "元钱!\n";
	        usleep(50);
	    }
	}
	// */
	
	echo "打完收工\n";
	
	
