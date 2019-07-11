**nohup**: Xshell连接服务器执行程序前加nohup可以避免断开连接程序被杀掉问题

**top**: 查看进程

**ps -u 用户名**: 查看用户的所有进程

**pwd**: 显示当前目录

**lls**：列出当前目录所有文件

**xshell sftp使用**：
	
	1）get：从远程服务器上下载一个文件存放到本地
	>> lcd d:\            #表示切换到本地的d盘下
	>> get ./test.sql　　 #这样就将当前文件下载本地的d盘下

	2）put：将本地的文件上传到远程服务器上
	>> put               #在windows下弹出选择文件的窗口
	
	3）lcd：先通过lcd切换到本地那个目录下
	>> lcd c:\            #表示切换到本地的c盘下

**netstat -tulpn:** 查看本地服务器端口映射

**grep -nr pattern**：查询当前目录下包含pattern的所有文件并显示行号

**gzip -dc myfile.gz | grep abc**：grep gz文件

**压缩\解压命令大全：**

- gzip [Options] file1 file2 file3：

	`-c //将输出写至标准输出，并保持原文件不变`
	
	`-d //进行解压操作`

	`-v //输出压缩/解压的文件名和压缩比等信息`

eg: 

    gzip exp1.txt exp2.txt　　　　 //分别将exp1.txt和exp2.txt压缩，且不保留原文件, 若要保留需加-c并结合>定向到目标文件
    gzip -dv exp1.gz　　　　　　 //将exp1.gz解压，并显示压缩比等信息。
	gzip -cd exp1.gz > exp.1　　  //将exp1.gz解压的结果放置在文件exp.1中，并且原压缩文件exp1.gz不会消失

- tar [Options] file_archive　　//注意tar的第一参数必须为命令选项，即不能直接接待处理文件，可以将多个文件打包到一个压缩包

常用命令参数：

	//指定tar进行的操作，以下三个选项不能出现在同一条命令中
	-c　　　　　　　　//创建一个新的打包文件(archive)	
	-x　　　　　　　　//对打包文件(archive)进行解压操作
	-t　　　　　　　　//查看打包文件(archive)的内容,主要是构成打包文件(archive)的文件名
	//指定支持的压缩/解压方式，操作取决于前面的参数，若为创建(-c),则进行压缩，若为解压(-x),则进行解压，不加下列参数时，则为单纯的打包操作
	-z　　　　　　　　//使用gzip进行压缩/解压，一般使用.tar.gz后缀
	-j　　　　　　　　//使用bzip2进行压缩/解压，一般使用.tar.bz2后缀

	//指定tar指令使用的文件，若没有压缩操作，则以.tar作为后缀
	-f filename　　 //-f后面接操作使用的文件，用空格隔开，且中间不能有其他参数，推荐放在参数集最后或单独作为参数

	//其他辅助选项
	-v　　　　　　　　//详细显示正在处理的文件名
	-p(小写)　　　　 //保留文件的权限和属性，在备份文件时较有用
	-P(大写)　　　　 //保留原文件的绝对路径，即不会拿掉文件路径开始的根目录，则在还原时会覆盖对应路径上的内容
	--exclude=file //排除不进行打包的文件

eg: 

	tar xvzf mongodb-linux-x86_64-2.4.6.tgz  /解压gzip文件
	tar -xvjf　etc.tar.bz2　//解压bzip2文件
	tar -cvjf etc.tar.bz2 /etc　　//-c为创建一个打包文件，相应的-f后面接创建的文件的名称，使用了.tar.bz2后缀，-j标志使用bzip2压缩，最后面为具体的操作对象/etc目录

- unzip [Options] file[.zip]

eg: `unzip aaa.zip `

**du -sh *: ** 查看目录空间

**dpkg -s package_name：** 检查package是否install

**shell脚本将命令返回值传递给变量：**

	var=`command` 或者 var=$(command)， eg: 
	ret=$(dpkg -s jq | grep "install ok installed")
	echo $ret

**sed：** 文本替换， sed '/s1/s2', eg: echo "aa" | sed 's/\"//g'
