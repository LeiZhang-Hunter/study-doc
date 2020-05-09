先编译头文件 生成object文件，再进行连接生成二进制文件

如果有8个.c文件和三个.h文件

我们要写一个makefile说明书写规则。

1.如果这个工程没有编译过，那么我们要把所有的.c文件编译并且连接

2.如果这个工程的某几个c文件被修改，我们只编译被修改的c文件，并且链接目标文件

3.如果这个工程头文件被更改，那么我们需要编译引用这几个头文件的c文件，并且链接程序。

只要我们makefile写的好，我们只用一个make命令就可以完成，make命令会自动智能的根据当前文件的修改状况来确定那些文件需要编译，然后自己编译所需要的文件和目标程序。

3.1makefile的规则

在makefile之前我们先看一下游戏规则

```
target ...:prerequisites...
	command
	
```

target可以是一个目标文件，也可以是一个object file，也可以是可执行文件。还可以是一个标签

prerequisites 是要生成target所需要的文件或者目标。

command也就是make需要执行的命令（任意的shell指令）。


这是一个文件的依赖关系，也就是说target这一个或者多个的目标文件依赖于prerequisites中的文件，其生成规则定义在command中。说白一点，prerequisites中如果有一个以上的文件比target要新，command命令就会被执行。这就是makefile的规则也是核心。

反斜杠\是换行的意思 ，目标文件 target包含：执行文件（target）包含：执行文件edit。如果要删除执行文件和所有中间文件只需要make clean就可以。

在makefile中，目标文件（target）包含：执行文件edit和中间目标文件(*.o),依赖文件就是冒号后面的那些.c和.h文件 。每一个.o文件都有一组依赖文件，而这些.o文件又是二进制文件 的依赖文件，依赖关系说明了目标文件是那些文件生成的，换言之就是哪些文件更新的。

在定义好target以后，后续的一行定义了make的指令，make会比较target文件和prerequisites文件的修改日期，如果 prerequisites 文件的日期要比 targets 文件的日期要新,或者 target 不存在的话,那么, make 就会执行后续定义的命令。

这里说明clean不是一个文件是一个动作名称

make不会自动执行标签后的命令，这个方法十分有用，我们可以在makefile中定义不用的命令，比如打包备份等等。

####makefile是如何工作的？

make以后会找makefile

会找到edit文件，并把这个文件作为最终目标

如果edit不存在，或者edit后面的依赖.o文件更新时间变动，他会执行后面的edit后面的命令生成edit

make会先编译连接.h和.c文件并且最终生成.o文件，最后生成二进制文件

####3.4 makefile中使用变量
定义 用变量名 + = 号来定义

使用 $()

	obj = web.o

	test_make:$(obj)

		cc -o test_make $(obj)

	web.o:web.c
		cc -c web.c


	clean:
		rm test_make web.o
		
####3.5 让make来自动推导结果

make 比如说一个a.o他就会自动把一个a.c加入导依赖关系中，于是我们不需要写的那么复杂

	obj = web.o

	test_make:$(obj)

		cc -o test_make $(obj)

	web.o:  


	clean:
		rm test_make web.o
		
####3.6 另类的makefile
由于好多.o文件都 需要大量的相同的.h所以我们可以做简化

	objects = main.o kbd.o command.o display.o \
	insert.o search.o files.o utils.o
	edit : $(objects)
	cc -o edit $(objects)
	$(objects) : defs.h
	kbd.o command.o files.o : command.h
	display.o insert.o search.o files.o : buffer.h
	.PHONY : clean
	clean :
	rm edit $(objects)

####3.7清楚目标文件规则

	clean:
		rm edit $(obj)
		
更稳健的方法是

	.PHONY : clean
	clean :
		-rm edit $(objects)


前面说过, .PHONY 意思表示 clean 是一个 “ 伪目标 ” ,。而在 rm 命令前面加了一个小减号的意思就是,也许某些文件出现问题,但不要管,继续做后面的事。当然, clean 的规则不要放在文件的开头,不然,这就会变成 make 的默认目标,相信谁也不愿意这样。不成文的规矩是 ——“clean 从来都是放在文件的最后 ”

我们希望，只要输入”make clean“后，”rm *.o temp“命令就会执行。但是，当当前目录中存在一个和指定目标重名的文件时，例如clean文件，结果就不是我们想要的了。输入”make clean“后，“rm *.o temp” 命令一定不会被执行。

解决的办法是:将目标clean定义成伪目标就成了。无论当前目录下是否存在“clean”这个文件，输入“make clean”后，“rm *.o temp”命令都会被执行。


###4.makefile总述

makefile里主要有5个东西：显示规则，隐晦规则，变量定义，文件指示和注释。

####4.2 makefile的文件名

默认情况下make会在当前目录下按以下顺序找寻文件“GUNmakefile”、“makefile”、“Makefile”的文件，找到了解释这个文件。这三个文件中最好使用Makefile这个文件名，因为这个文件名第一个字符为大写，这样比较显目。大多数linux 都支持makefile和“Makefile”


####4.3 引用其他makefile

在Makefile使用include 关键字可以把别的Makefile文件包含进来，这很像c语言李的#include，include语法是：

	include <filename>
	
filename可以是当前操作系统Shell的文件模式

include 前面可以有一些空字符，但是绝对不能是tab开始。include和filename可以用一个或者多个空格隔开。	举个例子，有这样几个makefile：a.mk、b.mk、c.mk,还有一个文件叫foo.make,以及一个变量$(bar),也包含了e.mk和f.mk。

	include foo.make *.mk $(bar)
	
等价于

	include foo.make a.mk b.mk c.mk e.mk f.mk
	
make 执行开始的时候，会把找寻include所指出的其他的makefile，并把他的内容放在当前位置。就好像c c++的include 一样。如果文件都没有指定绝对路径或者是相对路径的话，make会首先在当前目录下查找：

1.如果make执行时候 有 -I或者是“--include-dir”参数，那么make就会在这个参数指定的目录下寻找。

2.如果是目录<prefix>/include (一般是：/usr/local/bin或者/usr/include)存在的话，make也会去找。

如果找不到文件的话 make会生成一条警告信息，但不会立即出错，会继续加载文件，make载入完成后会再次重新尝试载入，如果还是不行 会报出致命错误。如果你不想理会无法独处的文件可以在include 前面加一个-号。

	-include filename
	
####4.4 环境变量MAKEFILES

如果你在环境中设置了环境变量，那么make会把这个环境变量做一个类似make的动作，我们不建议使用MAKEFILES这个环境变量。


####4.5make的工作方式

GUN的make工作步骤按照以下执行方式进行

1.读入所有的Makefile

2.读入被include其他的makefile

3.初始化文件中的变量

4.推到隐晦规则，并分析所有规则

5.为所有文件创建依赖关系

6.根据依赖关系决定哪些目标文件重新生成

7。执行生成命令

1-5是第一个步骤 6-7是第二个步骤

第一阶段如果变量被使用了，那么make会把其展开在使用位置。但是make不会马上展开，而是使用拖延战术，如果出现在依赖关系的规则中，只有当变量被使用了才会决定展开