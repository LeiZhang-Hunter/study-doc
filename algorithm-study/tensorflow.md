	import tensorflow as tf
	import os
	os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'
	# tf.enable_eager_execution(
	#     config=None,
	#     device_policy=None,
	#     execution_mode=None
	# )
	x = tf.constant(2)
	y = tf.constant(1)
	sess = tf.Session()
	print(sess.run(tf.add(x, y)))#3
	
	
通用脚本入口:

	import tensorflow as tf
	import os
	os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'
	
	def main(argv):
	    print argv

	tf.app.run(
	    main=main,
	    argv=None
	)

autograph

可以把函数转化为字符串，还可以把python代码函数转换为TensorFlow 图构建代码。

	# _*_ coding:utf-8 _*_  //为了告诉python解释器，按照utf-8编码读取源代码，否则，你在源代码中写的中文输出可能会由乱码
	#coding=utf-8 //上一行代码也可以写成这种形式

	import tensorflow as tf
	import os


	os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'
	# tf.enable_eager_execution(
	#     config=None,
	#     device_policy=None,
	#     execution_mode=None
	# )


	def square_if_positive(x):
	  if x > 0:
	    x = x * x
	  else:
	    x = 0.0
	  return x

	#AutoGraph 会将大部分 Python 语言转换为等效的 TensorFlow 图构建代码。
	fun = tf.autograph.to_code(square_if_positive)

	fun = tf.autograph.to_graph(square_if_positive)

	print fun(5)
	
####变量

tf Variable 可以初始化一个变量，然后我们在变量上可以进行各种各样的操作，变量初始化没有值

	import tensorflow as tf

	import os

	os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'

	a = tf.Variable(2,name="scalar")

	b = tf.Variable([2,3],name="vector")

	c = tf.Variable([[0,1],[2,3]],name="matrix")

	w = tf.Variable(tf.zeros([784,10]))
	
####最简单的变量全局初始化方法

	import tensorflow as tf

	import os

	os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'

	a = tf.Variable(2,name="scalar")

	b = tf.Variable([2,3],name="vector")

	c = tf.Variable([[0,1],[2,3]],name="matrix")

	w = tf.Variable(tf.zeros([784,10]))

	init = tf.global_variables_initializer()

	with tf.Session() as sess:
	    sess.run(init)


####初始化一个变量子集

	import tensorflow as tf

	import os

	os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'

	a = tf.Variable(2,name="scalar")

	b = tf.Variable([2,3],name="vector")

	c = tf.Variable([[0,1],[2,3]],name="matrix")

	w = tf.Variable(tf.zeros([784,10]))


	init = tf.variables_initializer([a,b],name="initdb")
	with tf.Session() as sess:
	    sess.run(init)
	    
####初始化一个变量

	import tensorflow as tf

	import os

	os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'

	a = tf.Variable(2,name="scalar")

	b = tf.Variable([2,3],name="vector")

	c = tf.Variable([[0,1],[2,3]],name="matrix")

	w = tf.Variable(tf.zeros([784,10]))


	with tf.Session() as sess:
	    sess.run(w.initializer)

####查看变量的值：

	import tensorflow as tf

	import os

	os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'

	a = tf.Variable(2,name="scalar")

	b = tf.Variable([2,3],name="vector")

	c = tf.Variable([[0,1],[2,3]],name="matrix")

	w = tf.Variable(tf.zeros([784,10]))


	with tf.Session() as sess:
	    sess.run(w.initializer)
	    print w.eval()

变量一般在求梯度时候用	

####placeholder用于存放用于训练的数据