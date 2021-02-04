# cctype

检查单个字符

isalnum 当字符是字母的时候是真的

```c
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << isalnum('9') << std::endl;
    return 0;
}
```

isalpha

当c是字母的时候为真

```c
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << isalpha('c') << std::endl;
    return 0;
}
```

iscntrl

当c是控制字符的时候为真

```c
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << iscntrl('\r') << std::endl;
    return 0;
}
```

isdigit
 如果参数是数字（0－9），函数返回true
```c
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << isdigit('4') << std::endl;
    return 0;
}

```

等等

isgraph()  如果参数是除空格之外的打印字符，函数返回true

```c
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << isgraph('@') << std::endl;
    return 0;
}

```

islower()  如果参数是小写字母，函数返回true
```
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << islower('a') << std::endl;
    return 0;
}

```

isprint()  如果参数是打印字符（包括空格），函数返回true
```c
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << islower('a') << std::endl;
    return 0;
}

```

ispunct()  如果参数是标点符号，函数返回true

```
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << ispunct('.') << std::endl;
    return 0;
}
```

isspace()  如果参数是标准空白字符，如空格、换行符、水平或垂直制表符，函数返回true
```c
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << isspace('\n') << std::endl;
    return 0;
}
```

isupper()  如果参数是大写字母，函数返回true
```c
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << isupper('A') << std::endl;
    return 0;
}
```

isxdigit() 如果参数是十六进制数字，即0－9、a－f、A－F，函数返回true
```c
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << isxdigit('a') << std::endl;
    return 0;
}
```

tolower()  如果参数是大写字符，返回其小写，否则返回该参数
```c
#include <cctype>
#include <iostream>
int main ()
{
    std::cout << tolower('A') << std::endl;
    return 0;
}
```
toupper()  如果参数是小写字符，返回其大写，否则返回该参数