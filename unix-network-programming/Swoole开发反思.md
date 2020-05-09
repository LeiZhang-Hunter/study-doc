#Swoole应用启动部分代码简例


1.首先我可以用php_sapi_name（）来获取php的运行模式，锁定再命令行状态下

2.可以用php的命令行参数，来做php 的 webservice的打开、关闭和重载以及帮助操作，为什么这么写，因为这样我们可以在操作或者关闭前做一些十分个性的操作，在这写一个十分简单的类包例子：

	class Server{
	
	    static $beforeDo;

		static $pidFile;

		public static function setPidFile()
		{
			self::$pidFile = $pidFile;
		}
	
	    public static function checkRunMode()
	    {
	        if(php_sapi_name() != 'cli')
	        {
	            exit("must be command mode");
	        }
	    }
	
	    public static function beforeUnder($beforeFunc)
	    {
	        self::$beforeDo = $beforeFunc;
	    }
	
	    public function runServer($startFunc)
	    {
	        global $argv;
	        if($argv[1] == 'stop')
	        {
	            call_user_func(self::$beforeDo);
	            self::$beforeDo;
	        }else if($argv[1] == 'start' || $argv[1] == '')
	        {
	            $startFunc();
	        }
	    }
	}
	
	Server::checkRunMode();
	Server::beforeUnder(function (){
	
	});
	Server::checkRunMode(function (){
	
	});